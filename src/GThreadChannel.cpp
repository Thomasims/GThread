#include <algorithm>

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
	if(m_queue.empty()) return false;
	GThreadChannelHandle* handle = static_cast<GThreadChannelHandle*>(data);
	lock_guard<mutex> lck( m_queuemtx );
	auto it = find_if(begin(m_queue), end(m_queue), [handle]( GThreadPacket* packet ) {
		return handle->filter_exclusive == (handle->filter.count( packet->m_tag ) > 0);
	});
	if ( it != end( m_queue ) ) {
		if ( handle->tmp_packet )
			delete handle->tmp_packet; // Should never happen
		handle->tmp_packet = *it;
		m_queue.erase(it);
		return true;
	}
	return false;
}

int GThreadChannel::PushReturnValues( lua_State* state, void* data ) {
	GThreadChannelHandle* handle = static_cast<GThreadChannelHandle*>(data);
	GThreadPacket* packet = PopPacket( handle->filter, handle->filter_exclusive, data );

	if ( handle->in_packet )
		delete handle->in_packet;

	handle->in_packet = packet;

	lua_pushinteger( state, packet->GetBytes() );
	lua_pushinteger( state, packet->m_tag );
	return 2;
}

GThreadPacket* GThreadChannel::PopPacket( set<uint16_t>& filter, bool exclusive, void* data ) {
	GThreadChannelHandle* handle = static_cast<GThreadChannelHandle*>(data);

	GThreadPacket* packet = handle->tmp_packet;
	handle->tmp_packet = nullptr;

	return packet;
}

void GThreadChannel::QueuePacket( GThreadPacket* packet ) {
	lock_guard<mutex> lck( m_queuemtx );
	lock_guard<mutex> lck2( m_threadsmtx );

	m_queue.emplace_back( new GThreadPacket(*packet) );

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
		luaD_setcfunction( state, "SetFilter", SetFilter );

		luaD_setcfunction( state, "WriteByte"    , GThreadPacket::WriteNumber   <GetPacketOut, int8_t> );
		luaD_setcfunction( state, "WriteShort"   , GThreadPacket::WriteNumber   <GetPacketOut, int16_t> );
		luaD_setcfunction( state, "WriteInt"     , GThreadPacket::WriteNumber   <GetPacketOut, int32_t> );
		luaD_setcfunction( state, "WriteLong"    , GThreadPacket::WriteNumber   <GetPacketOut, int64_t> );
		luaD_setcfunction( state, "WriteUByte"   , GThreadPacket::WriteNumber   <GetPacketOut, uint8_t> );
		luaD_setcfunction( state, "WriteUShort"  , GThreadPacket::WriteNumber   <GetPacketOut, uint16_t> );
		luaD_setcfunction( state, "WriteUInt"    , GThreadPacket::WriteNumber   <GetPacketOut, uint32_t> );
		luaD_setcfunction( state, "WriteULong"   , GThreadPacket::WriteNumber   <GetPacketOut, uint64_t> );
		luaD_setcfunction( state, "WriteSize"    , GThreadPacket::WriteNumber   <GetPacketOut, size_t> );
		luaD_setcfunction( state, "WriteFloat"   , GThreadPacket::WriteNumber   <GetPacketOut, float> );
		luaD_setcfunction( state, "WriteDouble"  , GThreadPacket::WriteNumber   <GetPacketOut, double> );
		luaD_setcfunction( state, "WriteData"    , GThreadPacket::WriteData     <GetPacketOut> );
		luaD_setcfunction( state, "WriteString"  , GThreadPacket::WriteString   <GetPacketOut> );
		luaD_setcfunction( state, "WriteFunction", GThreadPacket::WriteFunction <GetPacketOut> );

		luaD_setcfunction( state, "ReadByte"    , GThreadPacket::ReadNumber   <GetPacketIn, int8_t> );
		luaD_setcfunction( state, "ReadShort"   , GThreadPacket::ReadNumber   <GetPacketIn, int16_t> );
		luaD_setcfunction( state, "ReadInt"     , GThreadPacket::ReadNumber   <GetPacketIn, int32_t> );
		luaD_setcfunction( state, "ReadLong"    , GThreadPacket::ReadNumber   <GetPacketIn, int64_t> );
		luaD_setcfunction( state, "ReadUByte"   , GThreadPacket::ReadNumber   <GetPacketIn, uint8_t> );
		luaD_setcfunction( state, "ReadUShort"  , GThreadPacket::ReadNumber   <GetPacketIn, uint16_t> );
		luaD_setcfunction( state, "ReadUInt"    , GThreadPacket::ReadNumber   <GetPacketIn, uint32_t> );
		luaD_setcfunction( state, "ReadULong"   , GThreadPacket::ReadNumber   <GetPacketIn, uint64_t> );
		luaD_setcfunction( state, "ReadSize"    , GThreadPacket::ReadNumber   <GetPacketIn, size_t> );
		luaD_setcfunction( state, "ReadFloat"   , GThreadPacket::ReadNumber   <GetPacketIn, float> );
		luaD_setcfunction( state, "ReadDouble"  , GThreadPacket::ReadNumber   <GetPacketIn, double> );
		luaD_setcfunction( state, "ReadData"    , GThreadPacket::ReadData     <GetPacketIn> );
		luaD_setcfunction( state, "ReadString"  , GThreadPacket::ReadString   <GetPacketIn> );
		luaD_setcfunction( state, "ReadFunction", GThreadPacket::ReadFunction <GetPacketIn> );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent ) {
	if (!channel) return 0;

	GThreadChannelHandle* handle = luaD_new<GThreadChannelHandle>( state, channel, parent );
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
	if ( !handle->object ) return 0;

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

	if ( lua_gettop( state ) > 1 ) {
		handle->out_packet->m_tag = luaL_checkinteger( state, 2 );
	}

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

int GThreadChannel::SetFilter( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	if ( !handle->object ) return 0;

	handle->filter.clear();

	int nargs = lua_gettop( state );
	if( nargs > 1 ) {
		lua_Integer first = luaL_checkinteger( state, 2 );
		handle->filter_exclusive = first >= 0;

		handle->filter.insert( abs( first ) );
		for ( int i = 3; i <= nargs; ++i )
			handle->filter.insert( abs( luaL_checkinteger( state, i ) ) );
	} else {
		handle->filter_exclusive = false;
	}

	return 0;
}