#include "GThread.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

using namespace std;

unsigned int GThread::count = 0;
map<unsigned int, GThread*> GThread::detached;
mutex GThread::detachedmtx;

GThread::GThread() {
	m_killed = false;
	m_attached = true;
	m_id = count++;
	m_topnotifierid = 0;

	m_timingid = SetupNotifier( (Notifier*) &m_timing, this );

	m_thread = new thread( ThreadMain, this );
}

GThread::~GThread() {
	m_thread->detach();
	delete m_thread;
	for ( const auto& res : m_channels ) {
		if ( res.second.outgoing )
			res.second.outgoing->UnRef();
		if ( res.second.incoming )
			res.second.incoming->UnRef();
	}
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

void GThread::Terminate() {
#ifdef _WIN32
#pragma warning(suppress: 6258)
	::TerminateThread(m_thread->native_handle(), 1);
#else
	pthread_cancel(m_thread->native_handle());
#endif
}

int log( lua_State* state ) {
	int top = lua_gettop( state );
	for( int i = 1; i <= top; i++ ) {
		const char* text = lua_tolstring( state, i, NULL );
		if( text )
			std::cout << "log: " << text << std::endl;
	}
	return 0;
}

int onerror( lua_State* state ) {
	const char* text = lua_tolstring( state, -1, NULL );
	std::cout << "error: " << text << std::endl;
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
	lua_setglobal( state, "print" );

	return state;
}

void GThread::ThreadMain( GThread* handle ) {
#ifndef _WIN32
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
#endif

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

				int ret = luaL_loadbuffer( state, code.data(), code.length(), "GThread" ) || lua_pcall( state, 0, 0, NULL );

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


lua_Integer GThread::Wait( lua_State* state, set<lua_Integer> refs ) {
	unique_lock<mutex> lck( m_notifiersmtx );

	int ret = 0;
	chrono::system_clock::time_point until = chrono::system_clock::now() + chrono::hours( 1 );

	set<lua_Integer> newrefs;
	for ( lua_Integer ref : refs ) { // This is absolute trash, rethink this whole notifiers thing
		auto& it = m_notifiers.find( ref );
		if ( it == end( m_notifiers ) )
			continue;
		NotifierInstance notifierins = it->second;
		if ( notifierins.notifier )
			newrefs.insert( ref );
		else
			newrefs.insert( reinterpret_cast<lua_Integer>(notifierins.data) );
	}

	while ( !m_killed ) {
		for ( lua_Integer ref : newrefs ) {
			auto& it = m_notifiers.find( ref );
			if ( it == end( m_notifiers ) )
				continue;
			NotifierInstance notifierins = it->second;
			if ( notifierins.notifier->ShouldResume( &until, notifierins.data ) ) {
				lua_pushinteger( state, ref );
				return notifierins.notifier->PushReturnValues( state, notifierins.data ) + 1;
			}
		}
		m_notifierscvar.wait_until( lck, until );
	}

	return 0;
}

void GThread::WakeUp() {
	m_notifierscvar.notify_all();
}

lua_Integer GThread::SetupNotifier( Notifier* notifier, void* data ) {
	lock_guard<mutex> lck( m_notifiersmtx );

	lua_Integer id = m_topnotifierid++;
	m_notifiers[id] = { notifier, data };

	return id;
}

void GThread::RemoveNotifier( lua_Integer id, bool nolock ) {
	if( nolock ) {
		m_notifiers.erase( id );
	} else {
		lock_guard<mutex> lck( m_notifiersmtx );
		m_notifiers.erase( id );
	}
}

lua_Integer GThread::CreateTimer( std::chrono::system_clock::time_point when ) {
	lua_Integer id = SetupNotifier( nullptr, reinterpret_cast<void*>(m_timingid) );
	m_timing.CreatePoint( when, id );
	return id;
}

DoubleChannel GThread::OpenChannels( string name ) {
	auto& res = m_channels.find(name);
	if (res == end(m_channels)) {
		GThreadChannel* outgoing = new GThreadChannel();
		GThreadChannel* incoming = new GThreadChannel();

		outgoing->SetSibling( incoming );

		outgoing->Ref();
		incoming->Ref();
		
		return m_channels[name] = { outgoing, incoming };
	}
	else {
		return res->second;
	}
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
		luaD_setcfunction( state, "Terminate", Terminate );
	}
	lua_pop( state, 1 );
}

int GThread::PushGThread( lua_State* state, GThread* thread ) {
	GThreadHandle* handle = luaD_new<GThreadHandle>( state, thread ); // TODO: Make a ctor/dtor
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
	if ( !handle->object ) return 0;
	handle->object->DetachLua();
	handle->object = NULL;
	luaD_delete( handle );
	return 0;
}

int GThread::Run( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->object ) return luaL_error( state, "Invalid GThread" );

	string code;
	if ( lua_isstring( state, 2 ) ) {
		size_t size;
		const char* code_c = luaL_checklstring( state, 2, &size );
		code.append( code_c, size );
	} else if ( lua_isfunction( state, 2 ) ) {
		lua_pushvalue( state, 2 );
		lua_dump( state, stringwriter, &code );
		lua_pop( state, 1 );
	} else
		return 0;

	handle->object->Run( code );
	return 0;
}

int GThread::OpenChannel( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	GThread* thread = handle->object;
	if ( !thread ) return luaL_error( state, "Invalid GThread" );

	const char* name = luaL_checkstring( state, 2 );

	auto pair = thread->OpenChannels(name);

	return
		GThreadChannel::PushGThreadChannel( state, pair.outgoing, thread ) +
		GThreadChannel::PushGThreadChannel( state, pair.incoming, thread );
}

int GThread::AttachChannel( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	GThread* thread = handle->object;
	if ( !thread ) return luaL_error( state, "Invalid GThread" );
	int nargs = lua_gettop( state );
	
	const char* name = luaL_checkstring( state, 2 );
	auto& res = thread->m_channels.find( name );
	DoubleChannel* channels = NULL;
	if ( res == end( thread->m_channels ) )
		channels = &(thread->m_channels[name] = { NULL, NULL });
	else
		channels = &res->second;

	if ( nargs >= 3 && lua_isuserdata( state, 3 ) && !channels->outgoing ) {
		channels->outgoing = GThreadChannel::Get( state, 3 );
		channels->outgoing->Ref();
	}

	if ( nargs >= 4 && lua_isuserdata( state, 4 ) && !channels->incoming ) {
		channels->incoming = GThreadChannel::Get( state, 4 );
		channels->incoming->Ref();
	}

	if ( channels->outgoing && channels->incoming )
		channels->outgoing->SetSibling( channels->incoming );

	return 0;
}

int GThread::Kill( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->object ) return luaL_error( state, "Invalid GThread" );

	handle->object->m_killed = true;
	handle->object->WakeUp();

	return 0;
}

int GThread::Terminate( lua_State* state ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( state, 1, "GThread" );
	if ( !handle->object ) return luaL_error( state, "Invalid GThread" );

	handle->object->Terminate();

	return 0;
}
