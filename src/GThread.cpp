#include "GThread.h"

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

void GThread::ThreadMain( GThread* handle ) {
	// Create Lua state
	// Wait on code queue
	// Destroy if empty & detached
	unique_lock<mutex> lck( handle->m_codemtx );
	while( !handle->m_codequeue.empty() || handle->m_attached ) {
		if( handle->m_codequeue.empty() ) {
			handle->m_codecvar.wait( lck );
		} else {
			string code = handle->m_codequeue.front();
			handle->m_codequeue.pop();
			//Run code
		}
	}
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

