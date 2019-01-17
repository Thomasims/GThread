#include "GThread.h"

using namespace std;

GThreadChannel::GThreadChannel() {
	m_references = 0;
	m_closed = false;
	m_sibling = NULL;
}

GThreadChannel::~GThreadChannel() {
	DetachSibling();
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

	lua_pushinteger( state, packet->GetBytes() );
	return 1;
}

GThreadPacket* GThreadChannel::PopPacket() {
	GThreadPacket* packet = m_queue.front();
	m_queue.pop();
	m_queuemtx.unlock();

	return packet;
}

void GThreadChannel::QueuePacket( GThreadPacket* packet ) {
	lock_guard<mutex> lck( m_queuemtx );
	lock_guard<mutex> lck2( m_threadsmtx );

	m_queue.push( new GThreadPacket(*packet) );

	for ( GThread* thread : m_threads ) {
		thread->WakeUp();
	}
}


void GThreadChannel::AddThread( GThread* handle ) {
	lock_guard<mutex> lck( m_threadsmtx );
	m_threads.insert( handle );
}

void GThreadChannel::RemoveThread( GThread* handle ) {
	lock_guard<mutex> lck( m_threadsmtx );
	m_threads.erase( handle );
}

void GThreadChannel::DetachSibling() {
	if ( m_sibling ) {
		m_sibling->m_sibling = NULL;
		m_sibling = NULL;
	}
}

void GThreadChannel::SetSibling( GThreadChannel* other ) {
	if ( m_sibling == other ) return;

	DetachSibling();
	other->DetachSibling();
	m_sibling = other;
	other->m_sibling = this;
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

		luaD_setcfunction( state, "WriteByte"  , GThreadPacket::WriteNumber <GetPacketOut, int8_t> );
		luaD_setcfunction( state, "WriteShort" , GThreadPacket::WriteNumber <GetPacketOut, int16_t> );
		luaD_setcfunction( state, "WriteInt"   , GThreadPacket::WriteNumber <GetPacketOut, int32_t> );
		luaD_setcfunction( state, "WriteLong"  , GThreadPacket::WriteNumber <GetPacketOut, int64_t> );
		luaD_setcfunction( state, "WriteUByte" , GThreadPacket::WriteNumber <GetPacketOut, uint8_t> );
		luaD_setcfunction( state, "WriteUShort", GThreadPacket::WriteNumber <GetPacketOut, uint16_t> );
		luaD_setcfunction( state, "WriteUInt"  , GThreadPacket::WriteNumber <GetPacketOut, uint32_t> );
		luaD_setcfunction( state, "WriteULong" , GThreadPacket::WriteNumber <GetPacketOut, uint64_t> );
		luaD_setcfunction( state, "WriteFloat" , GThreadPacket::WriteNumber <GetPacketOut, float> );
		luaD_setcfunction( state, "WriteDouble", GThreadPacket::WriteNumber <GetPacketOut, double> );
		luaD_setcfunction( state, "WriteData"  , GThreadPacket::WriteData   <GetPacketOut> );
		luaD_setcfunction( state, "WriteString", GThreadPacket::WriteString <GetPacketOut> );

		luaD_setcfunction( state, "ReadByte"  , GThreadPacket::ReadNumber <GetPacketIn, int8_t> );
		luaD_setcfunction( state, "ReadShort" , GThreadPacket::ReadNumber <GetPacketIn, int16_t> );
		luaD_setcfunction( state, "ReadInt"   , GThreadPacket::ReadNumber <GetPacketIn, int32_t> );
		luaD_setcfunction( state, "ReadLong"  , GThreadPacket::ReadNumber <GetPacketIn, int64_t> );
		luaD_setcfunction( state, "ReadUByte" , GThreadPacket::ReadNumber <GetPacketIn, uint8_t> );
		luaD_setcfunction( state, "ReadUShort", GThreadPacket::ReadNumber <GetPacketIn, uint16_t> );
		luaD_setcfunction( state, "ReadUInt"  , GThreadPacket::ReadNumber <GetPacketIn, uint32_t> );
		luaD_setcfunction( state, "ReadULong" , GThreadPacket::ReadNumber <GetPacketIn, uint64_t> );
		luaD_setcfunction( state, "ReadFloat" , GThreadPacket::ReadNumber <GetPacketIn, float> );
		luaD_setcfunction( state, "ReadDouble", GThreadPacket::ReadNumber <GetPacketIn, double> );
		luaD_setcfunction( state, "ReadData"  , GThreadPacket::ReadData   <GetPacketIn> );
		luaD_setcfunction( state, "ReadString", GThreadPacket::ReadString <GetPacketIn> );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent ) {
	if (!channel) return 0;
	GThreadChannelHandle* handle = luaD_new<GThreadChannelHandle>( state, channel, parent, 0, nullptr, nullptr ); // TODO: Make a ctor/dtor

	if ( parent ) {
		handle->id = parent->SetupNotifier( channel, (void*) handle );
		channel->AddThread( handle->parent );
	}
	++(channel->m_references);


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

GThreadPacket* GThreadChannel::GetPacketIn( lua_State* state, int narg ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, narg, "GThreadChannel" );
	if ( !handle->object ) luaL_error( state, "Invalid GThreadChannel" );

	return handle->in_packet;
}

GThreadPacket* GThreadChannel::GetPacketOut( lua_State* state, int narg ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, narg, "GThreadChannel" );
	if ( !handle->object ) luaL_error( state, "Invalid GThreadChannel" );

	return handle->out_packet;
}

int GThreadChannel::_gc( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return 0;


	if ( ! --channel->m_references )
		delete channel;

	handle->object = NULL;
	if ( handle->parent ) {
		handle->parent->RemoveNotifier( handle->id );
		channel->RemoveThread( handle->parent );
	}

	if ( handle->in_packet )
		delete handle->in_packet;
	if ( handle->out_packet )
		delete handle->out_packet;

	luaD_delete( handle );

	return 0;
}

int GThreadChannel::PullPacket( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadChannel* channel = handle->object;
	if ( !channel ) return 0;

	if ( channel->ShouldResume( NULL, handle ) ) {
		return channel->PushReturnValues( state, handle );
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

	if ( packet ) {
		if ( channel->m_sibling ) {
			channel->m_sibling->QueuePacket( packet );
		} else {
			channel->QueuePacket( packet );
		}
	}

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