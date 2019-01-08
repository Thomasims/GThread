#include "GThreadChannel.h"
#include "GThreadPacket.h"

GThreadChannel::GThreadChannel() {
	m_references = 0;
	m_closed = false;
}

GThreadChannel::~GThreadChannel() {

}


bool GThreadChannel::ShouldResume( chrono::system_clock::time_point* until ) {
	m_queuemtx.lock();
	if ( !m_queue.empty() )
		return true;
	m_queuemtx.unlock();
	return false;
}

int GThreadChannel::PushReturnValues( lua_State* state ) {
	GThreadPacket* packet = m_queue.front();
	GThreadPacket::PushGThreadPacket( state, packet );
	m_queue.pop();
	m_queuemtx.unlock();

	if ( CheckClosing() )
		delete this;

	return 0;
}

void GThreadChannel::QueuePacket( GThreadPacket* packet ) {
	lock_guard<mutex> lck( m_queuemtx );
	lock_guard<mutex> lck2( m_handlesmtx );

	m_queue.push( new GThreadPacket(*packet) );

	for ( GThreadChannelHandle* handle : m_handles ) {
		if ( handle->parent )
			handle->parent->WakeUp();
	}
}


void GThreadChannel::AddHandle( GThreadChannelHandle* handle ) {
	lock_guard<mutex> lck( m_handlesmtx );
	m_handles.insert( handle );
}

void GThreadChannel::RemoveHandle( GThreadChannelHandle* handle ) {
	lock_guard<mutex> lck( m_handlesmtx );
	m_handles.erase( handle );
}

bool GThreadChannel::CheckClosing() {
	lock_guard<mutex> lck( m_queuemtx );
	if ( m_closed && m_queue.empty() ) {
		for ( GThreadChannelHandle* handle : m_handles ) {
			handle->channel = NULL;
			if ( handle->parent )
				handle->parent->RemoveNotifier( handle->id );
		}
		return true;
	}
	return false;
}


GThreadPacket* GThreadChannel::FinishPacket() {
	return NULL; // TODO: Internal packet
}


void GThreadChannel::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadChannel" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		luaD_setcfunction( state, "__gc", _gc );
		luaD_setcfunction( state, "PullPacket", PullPacket );
		luaD_setcfunction( state, "PushPacket", PushPacket );
		luaD_setcfunction( state, "GetHandle", GetHandle );
		//luaD_setcfunction( state, "SetFilter", SetFilter ); // Postponed
		luaD_setcfunction( state, "StartPacket", PushPacket );
		luaD_setcfunction( state, "Close", Close );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) lua_newuserdata( state, sizeof(GThreadChannelHandle) );

	handle->channel = channel;
	handle->parent = parent;
	if ( parent )
		handle->id = parent->SetupNotifier( channel );
	++(channel->m_references);

	channel->AddHandle( handle );

	luaL_getmetatable( state, "GThreadChannel" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadChannel::Create( lua_State* state ) {
	return PushGThreadChannel( state, new GThreadChannel() );
}

int GThreadChannel::_gc( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->channel ) return luaL_error( state, "Invalid GThreadChannel" );

	handle->channel->RemoveHandle( handle );

	if ( ! --handle->channel->m_references )
		delete handle->channel;

	handle->channel = NULL;
	if ( handle->parent )
		handle->parent->RemoveNotifier( handle->id );

	return 0;
}

int GThreadChannel::PullPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->channel;
	if ( !channel ) return 0;

	if ( channel->ShouldResume( NULL ) )
		return channel->PushReturnValues( state );

	return 0;
}

int GThreadChannel::PushPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->channel;
	if ( !channel ) return luaL_error( state, "Invalid GThreadChannel" );
	if ( channel->m_closed ) return luaL_error( state, "This GThreadChannel is closed" );

	GThreadPacket* packet;
	if ( lua_isuserdata( state, 2 ) )
		packet = GThreadPacket::Get( state, 2 );
	else
		packet = channel->FinishPacket();

	if ( packet )
		channel->QueuePacket( packet );

	return 0;
}

int GThreadChannel::GetHandle( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->channel ) return 0;

	lua_pushinteger( state, handle->id );

	return 1;
}

int GThreadChannel::StartPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->channel;
	if ( !channel ) return luaL_error( state, "Invalid GThreadChannel" );
	if ( channel->m_closed ) return luaL_error( state, "This GThreadChannel is closed" );

	// TODO: Internal packet

	return 0;
}

int GThreadChannel::Close( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->channel;
	if ( !channel || channel->m_closed ) return 0;

	channel->m_closed = true;
	if ( channel->CheckClosing() )
		delete channel;

	return 0;
}