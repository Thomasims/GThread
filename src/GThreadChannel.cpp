#include "GThreadChannel.h"
#include "GThreadPacket.h"

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


bool GThreadChannel::CheckClosing() {
	{ return false; }
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

		luaD_setcfunction( state, "WriteByte", WriteNumber<int8_t> );
		luaD_setcfunction( state, "WriteShort", WriteNumber<int16_t> );
		luaD_setcfunction( state, "WriteInt", WriteNumber<int32_t> );
		luaD_setcfunction( state, "WriteLong", WriteNumber<int64_t> );
		luaD_setcfunction( state, "WriteUByte", WriteNumber<uint8_t> );
		luaD_setcfunction( state, "WriteUShort", WriteNumber<uint16_t> );
		luaD_setcfunction( state, "WriteUInt", WriteNumber<uint32_t> );
		luaD_setcfunction( state, "WriteULong", WriteNumber<uint64_t> );
		luaD_setcfunction( state, "WriteFloat", WriteNumber<float> );
		luaD_setcfunction( state, "WriteDouble", WriteNumber<double> );
		luaD_setcfunction( state, "WriteData", WriteData );
		luaD_setcfunction( state, "WriteString", WriteString );

		luaD_setcfunction( state, "ReadByte", ReadNumber<int8_t> );
		luaD_setcfunction( state, "ReadShort", ReadNumber<int16_t> );
		luaD_setcfunction( state, "ReadInt", ReadNumber<int32_t> );
		luaD_setcfunction( state, "ReadLong", ReadNumber<int64_t> );
		luaD_setcfunction( state, "ReadUByte", ReadNumber<uint8_t> );
		luaD_setcfunction( state, "ReadUShort", ReadNumber<uint16_t> );
		luaD_setcfunction( state, "ReadUInt", ReadNumber<uint32_t> );
		luaD_setcfunction( state, "ReadULong", ReadNumber<uint64_t> );
		luaD_setcfunction( state, "ReadFloat", ReadNumber<float> );
		luaD_setcfunction( state, "ReadDouble", ReadNumber<double> );
		luaD_setcfunction( state, "ReadData", ReadData );
		luaD_setcfunction( state, "ReadString", ReadString );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent ) {
	if (!channel) return 0;
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


template<class T>
int GThreadChannel::WriteNumber( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->out_packet;
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->Write<T>( static_cast<T>( luaL_checknumber( state, 2 ) ) ) );
	return 1;
}

int GThreadChannel::WriteData( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->out_packet;
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );
	auto asked_len = size_t( luaL_checknumber( state, 3 ) );

	lua_pushinteger( state, packet->WriteData( data, asked_len > len ? len : asked_len ) );
	return 1;
}

int GThreadChannel::WriteString( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->out_packet;
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );

	lua_pushinteger( state, packet->Write( len ) + packet->WriteData( data, len ) );
	return 1;
}


template<class T>
int GThreadChannel::ReadNumber( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->in_packet;
	if ( !packet ) return 0;

	lua_pushnumber( state, lua_Number( packet->Read<T>() ) );
	return 1;
}

int GThreadChannel::ReadData( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->in_packet;
	if ( !packet ) return 0;

	std::string data = packet->ReadData( size_t( luaL_checknumber( state, 2 ) ) );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

int GThreadChannel::ReadString( lua_State* state ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) luaL_checkudata( state, 1, "GThreadChannel" );
	GThreadPacket* packet = handle->in_packet;
	if ( !packet ) return 0;

	size_t len = packet->Read<size_t>();

	std::string data = packet->ReadData( len );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}