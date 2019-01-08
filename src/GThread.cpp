#include "GThread.h"

#include <iostream>
#include <chrono>

unsigned int GThread::count = 0;
map<unsigned int, GThread*> GThread::detached;
mutex GThread::detachedmtx;

GThread::GThread() {
	m_attached = true;
	m_thread = new thread( ThreadMain, this );
	m_thread->detach();

	m_id = count++;
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
	// Create Lua state
	// Wait on code queue
	// Destroy if empty & detached
	lua_State* state = newstate();
	luaopen_engine( state, handle );

	unique_lock<mutex> lck( handle->m_codemtx );
	while ( !handle->m_codequeue.empty() || handle->m_attached ) {
		if ( handle->m_codequeue.empty() ) {
			handle->m_codecvar.wait( lck );
		}
		else {
			string code = handle->m_codequeue.front();
			handle->m_codequeue.pop();

			struct string_source source { &code, false };

			int ret = lua_load( state, string_reader, &source, "GThread" ) || lua_pcall( state, 0, 0, NULL );

			if ( ret )
				onerror( state );
		}
	}

	lua_close( state );
}


lua_Integer GThread::Wait( const lua_Integer* refs, size_t n ) {
	return 0;
}

void GThread::WakeUp( const char* channel ) {

}

void GThread::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThread" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		lua_pushcfunction( state, _gc );
		lua_setfield( state, -2, "__gc" );

		lua_pushcfunction( state, Run );
		lua_setfield( state, -2, "Run" );

		lua_pushcfunction( state, OpenChannel );
		lua_setfield( state, -2, "OpenChannel" );

		lua_pushcfunction( state, AttachChannel );
		lua_setfield( state, -2, "AttachChannel" );

		lua_pushcfunction( state, Kill );
		lua_setfield( state, -2, "Kill" );
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
	return 0;
}
