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
	
	detachedmtx.lock();
	detached[ m_id ] = this;
	detachedmtx.unlock();
}

void GThread::ReattachLua( bool update ) {
	m_attached = true;
	
	if( update ) {
		detachedmtx.lock();
		detached.erase( m_id );
		detachedmtx.unlock();
	}
}

void GThread::Run( string code ) {
	m_codemtx.lock();
	m_codequeue.push( code );
	m_codecvar.notify_all();
	m_codemtx.unlock();
}

int log(lua_State* state) {
	size_t size = 0;
	const char* text = luaL_checklstring( state, 1, &size );
	string output(text, size);
	std::cout << "log: " << text << std::endl;
	return 0;
}

int onerror(lua_State* state) {
	size_t size = 0;
	const char* text = luaL_checklstring( state, -1, &size );
	std::cout << "error: " << text << std::endl;
	string output(text, size);
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
	lua_setfield( state, LUA_GLOBALSINDEX, "log" );

	return state;
}

void GThread::ThreadMain( GThread* handle ) {
	// Create Lua state
	// Wait on code queue
	// Destroy if empty & detached
	lua_State* state = newstate();
	luaopen_engine( state, handle );

	unique_lock<mutex> lck( handle->m_codemtx );
	while( !handle->m_codequeue.empty() || handle->m_attached ) {
		if( handle->m_codequeue.empty() ) {
			handle->m_codecvar.wait( lck );
		} else {
			string code = handle->m_codequeue.front();
			handle->m_codequeue.pop();

			int ret = luaL_loadstring( state, code.c_str() ) || lua_pcall( state, 0, 0, NULL );

			switch( ret ) {
				case LUA_OK:
					break;
				case LUA_ERRRUN:
				case LUA_ERRSYNTAX:
				case LUA_ERRMEM:
				case LUA_ERRERR:
					onerror( state );
					break;
			}
		}
	}

	lua_close( state );
}


lua_Integer GThread::Wait( const lua_Integer* refs, size_t n ) {
	return 0;
}

void GThread::WakeUp( const char* channel ) {

}

void GThread::SetupMetaFields( ILuaBase* LUA ) {
	LUA_SET( CFunction, "__gc", __gc );
	LUA_SET( CFunction, "Run", Run );
	LUA_SET( CFunction, "OpenChannel", OpenChannel );
	LUA_SET( CFunction, "AttachChannel", AttachChannel );
	LUA_SET( CFunction, "Kill", Kill );
}

LUA_METHOD_IMPL( GThread::__gc ) {
	LUA->CheckType(1, TypeID_Thread);
	GThread* thread = LUA->GetUserType<GThread>(1, TypeID_Thread);
	thread->DetachLua();
	LUA->SetUserType(1, NULL);
	return 0;
}

LUA_METHOD_IMPL( GThread::Create ) {
	GThread* thread = new GThread();
	LUA->PushUserType(thread, TypeID_Thread);
	return 1;
}

LUA_METHOD_IMPL( GThread::GetDetached ) {
	LUA->CreateTable();
	{
		int i = 1;
		detachedmtx.lock();
		for( auto& el: detached ) {
			el.second->ReattachLua( false );
			LUA->PushNumber( i );
			LUA->PushUserType( el.second, TypeID_Thread );
			LUA->SetTable( -3 );
		}
		detached.clear();
		detachedmtx.unlock();
	}
	return 1;
}

LUA_METHOD_IMPL( GThread::Run ) {
	LUA->CheckType(1, TypeID_Thread);
	const char* code = LUA->CheckString( 2 );
	GThread* thread = LUA->GetUserType<GThread>(1, TypeID_Thread);
	thread->Run( code );
	return 0;
}

LUA_METHOD_IMPL( GThread::OpenChannel ) {
	return 0;
}

LUA_METHOD_IMPL( GThread::AttachChannel ) {
	return 0;
}


LUA_METHOD_IMPL( GThread::Kill ) {
	return 0;
}

