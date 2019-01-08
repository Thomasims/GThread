#include "GThread.h"
#include "GThreadChannel.h"
#include "Notifier.h"

unsigned int GThread::count = 0;
map<unsigned int, GThread*> GThread::detached;
mutex GThread::detachedmtx;

GThread::GThread() {
	m_attached = true;
	m_id = count++;
	m_topnotifierid = 0;

	m_thread = new thread( ThreadMain, this );
	m_thread->detach();
}

GThread::~GThread() {
	delete m_thread;
}

void GThread::DetachLua() {
	m_attached = false;
	m_codecvar.notify_all();

	lock_guard<mutex> lck( detachedmtx );
	detached[m_id] = this;
}

void GThread::ReattachLua( bool update ) {
	m_attached = true;

	if ( update ) {
		lock_guard<mutex> lck( detachedmtx );
		detached.erase( m_id );
	}
}

void GThread::Run( string code ) {
	lock_guard<mutex> lck( m_codemtx );
	m_codequeue.push( code );
	m_codecvar.notify_all();
}

int log( lua_State* state ) {
	size_t size = 0;
	const char* text = luaL_checklstring( state, 1, &size );
	string output( text, size );
	std::cout << "log: " << text << std::endl;
	return 0;
}

int onerror( lua_State* state ) {
	size_t size = 0;
	const char* text = luaL_checklstring( state, -1, &size );
	std::cout << "error: " << text << std::endl;
	string output( text, size );
	return 0;
}

lua_State* newstate() {
	lua_State* state = luaL_newstate();

	luaopen_base( state );
	luaopen_bit( state );
	luaopen_debug( state );
	luaopen_jit( state );
	luaopen_math( state );
	luaopen_os( state );
	//luaopen_package( state );
	luaopen_string( state );
	luaopen_table( state );
	luaopen_coroutine( state );

	lua_pushcfunction( state, log );
	lua_setglobal( state, "log" );

	return state;
}

struct string_source {
	string* code;
	bool done;
};

const char* string_reader( lua_State* state, void* data, size_t* size ) {
	struct string_source* source = (struct string_source*) data;
	if ( source->done ) return NULL;
	source->done = true;

	const string& code = *source->code;
	*size = code.length();
	return code.c_str();
}

void GThread::ThreadMain( GThread* handle ) {
	lua_State* state = newstate();
	luaopen_engine( state, handle );

	{
		unique_lock<mutex> lck( handle->m_codemtx );
		while ( !handle->m_codequeue.empty() || handle->m_attached ) {
			if ( handle->m_codequeue.empty() ) {
				handle->m_codecvar.wait( lck );
			}
			else {
				string code = handle->m_codequeue.front();
				handle->m_codequeue.pop();

				handle->m_killed = false;

				struct string_source source { &code, false };

				int ret = lua_load( state, string_reader, &source, "GThread" ) || lua_pcall( state, 0, 0, NULL );

				if ( ret )
					onerror( state );
			}
		}
	}

	lua_close( state );

	{
		lock_guard<mutex> lck( detachedmtx );
		detached.erase( handle->m_id );
	}

	delete handle;
}


lua_Integer GThread::Wait( lua_State* state, const lua_Integer* refs, size_t n ) {
	unique_lock<mutex> lck( m_notifiersmtx );

	int ret = 0;
	chrono::system_clock::time_point until = chrono::system_clock::now() + chrono::hours( 1 );

	while ( !m_killed ) {
		for ( unsigned int i = 0; i < n; i++ ) {
			Notifier* notifier = m_notifiers[refs[i]];
			if ( notifier->ShouldResume( &until ) ) {
				lua_pushinteger( state, refs[i] );
				return notifier->PushReturnValues( state ) + 1;
			}
		}
		m_notifierscvar.wait_until( lck, until );
	}

	return 0; // TODO: Explore the possibility of throwing an exception instead
}

void GThread::WakeUp() {
	m_notifierscvar.notify_all();
}

lua_Integer GThread::SetupNotifier( Notifier* notifier ) {
	lock_guard<mutex> lck( m_notifiersmtx );

	lua_Integer id = m_topnotifierid++;
	m_notifiers[id] = notifier;

	return id;
}

void GThread::RemoveNotifier( lua_Integer id ) {
	lock_guard<mutex> lck( m_notifiersmtx );
	m_notifiers.erase( id );
}

void GThread::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThread" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );
		
		luaD_setcfunction( state, "__gc", _gc );
		luaD_setcfunction( state, "Run", Run );
		luaD_setcfunction( state, "OpenChannel", OpenChannel );
		luaD_setcfunction( state, "AttachChannel", AttachChannel );
		luaD_setcfunction( state, "Kill", Kill );
	}
	lua_pop( state, 1 );
}

int GThread::PushGThread( lua_State* state, GThread* thread ) {
	GThreadHandle* handle = (GThreadHandle*) lua_newuserdata( state, sizeof(GThreadHandle) );
	handle->thread = thread;
	luaL_getmetatable( state, "GThread" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThread::Create( lua_State* state ) {
	return PushGThread( state, new GThread() );
}

int GThread::GetDetached( lua_State* state ) {
	lock_guard<mutex> lck( detachedmtx );
	lua_newtable( state );
	{
		int i = 1;
		for ( auto& el : detached ) {
			el.second->ReattachLua( false );
			lua_pushnumber( state, i );
			PushGThread( state, el.second );
			lua_settable( state, -3 );
		}
		detached.clear();
	}
	return 1;
}

int GThread::_gc( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->thread ) return luaL_error( state, "Invalid GThread" );
	handle->thread->DetachLua();
	handle->thread = NULL;
	return 0;
}

int GThread::Run( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->thread ) return luaL_error( state, "Invalid GThread" );

	size_t size;
	const char* code = luaL_checklstring( state, 2, &size );

	handle->thread->Run( string( code, size ) );
	return 0;
}

int GThread::OpenChannel( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->thread ) return luaL_error( state, "Invalid GThread" );
	return 0;
}

int GThread::AttachChannel( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->thread ) return luaL_error( state, "Invalid GThread" );
	return 0;
}

int GThread::Kill( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->thread ) return luaL_error( state, "Invalid GThread" );

	handle->thread->m_killed = true;
	handle->thread->WakeUp();

	return 0;
}
