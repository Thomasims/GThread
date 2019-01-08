#include "GThreadChannel.h"
#include "GThreadPacket.h"

GThreadChannel::GThreadChannel() {
	m_references = 0;
	m_closed = false;
}

GThreadChannel::~GThreadChannel() {

}


bool GThreadChannel::ShouldResume( chrono::system_clock::time_point* until, void* data ) {
	// TODO: Check filter with data
	m_queuemtx.lock();
	if ( !m_queue.empty() )
		return true;
	m_queuemtx.unlock();
	return false;
}

int GThreadChannel::PushReturnValues( lua_State* state, void* data ) {
	// TODO: Check filter with data
	GThreadChannelHandle* handle = (GThreadChannelHandle*) data;
	GThreadPacket* packet = PopPacket();

	if ( handle->in_packet )
		delete handle->in_packet;

	handle->in_packet = packet;

	lua_pushinteger( state, packet->GetBits() );
	return 1;
}

GThreadPacket* GThreadChannel::PopPacket() {
	GThreadPacket* packet = m_queue.front();
	m_queue.pop();
	m_queuemtx.unlock();

	if ( CheckClosing() )
		delete this;

	return packet;
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
			handle->object = NULL;
			if ( handle->parent )
				handle->parent->RemoveNotifier( handle->id );
		}
		return true;
	}
	return false;
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
		luaD_setcfunction( state, "StartPacket", StartPacket );
		luaD_setcfunction( state, "Close", Close );

		luaD_setcfunction( state, "GetInPacket", GetInPacket );
		luaD_setcfunction( state, "GetOutPacket", GetOutPacket );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent ) {
	if (!channel) {
		lua_pushnil(state);
		return 1;
	}
	GThreadChannelHandle* handle = (GThreadChannelHandle*) lua_newuserdata( state, sizeof(GThreadChannelHandle) );

	handle->object = channel;
	handle->parent = parent;
	if ( parent )
		handle->id = parent->SetupNotifier( channel, (void*) handle );
	handle->in_packet = NULL;
	handle->out_packet = NULL;
	++(channel->m_references);

	channel->AddHandle( handle );

	luaL_getmetatable( state, "GThreadChannel" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadChannel::Create( lua_State* state ) {
	return PushGThreadChannel( state, new GThreadChannel() );
}

GThreadChannel* GThreadChannel::Get( lua_State* state, int narg ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, narg, "GThreadChannel" );
	if ( !handle->object ) luaL_error( state, "Invalid GThreadChannel" );

	return handle->object;
}

int GThreadChannel::_gc( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return 0;

	channel->RemoveHandle( handle );

	if ( ! --channel->m_references )
		delete channel;

	handle->object = NULL;
	if ( handle->parent )
		handle->parent->RemoveNotifier( handle->id );

	if ( handle->in_packet )
		delete handle->in_packet;
	if ( handle->out_packet )
		delete handle->out_packet;

	return 0;
}

int GThreadChannel::PullPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return 0;

	if ( channel->ShouldResume( NULL, handle ) ) {
		GThreadPacket* packet = channel->PopPacket();
		GThreadPacket::PushGThreadPacket( state, packet );
		return 1;
	}

	return 0;
}

int GThreadChannel::PushPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return luaL_error( state, "Invalid GThreadChannel" );
	if ( channel->m_closed ) return luaL_error( state, "This GThreadChannel is closed" );

	GThreadPacket* packet;
	if ( lua_isuserdata( state, 2 ) ) {
		packet = GThreadPacket::Get( state, 2 );
	} else {
		packet = handle->out_packet;
		handle->out_packet = NULL;
	}

	if ( packet )
		channel->QueuePacket( packet );

	return 0;
}

int GThreadChannel::GetHandle( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->object ) return 0;

	lua_pushinteger( state, handle->id );

	return 1;
}

int GThreadChannel::StartPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return luaL_error( state, "Invalid GThreadChannel" );
	if ( channel->m_closed ) return luaL_error( state, "This GThreadChannel is closed" );

	if ( handle->out_packet )
		handle->out_packet->Clear();
	else
		handle->out_packet = new GThreadPacket();

	return 0;
}

int GThreadChannel::Close( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel || channel->m_closed ) return 0;

	channel->m_closed = true;
	if ( channel->CheckClosing() )
		delete channel;

	return 0;
}

int GThreadChannel::GetInPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->object ) return 0;

	if ( handle->in_packet ) {
		GThreadPacket::PushGThreadPacket( state, handle->in_packet );
		handle->in_packet = NULL;
		return 1;
	}

	return 0;
}

int GThreadChannel::GetOutPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->object ) return 0;

	if ( handle->out_packet ) {
		GThreadPacket::PushGThreadPacket( state, handle->out_packet );
		handle->out_packet = NULL;
		return 1;
	}

	return 0;
}