#include "GThreadPacket.h"

using namespace std;

GThreadPacket::GThreadPacket( const GThreadPacket& other )
	: m_references{ 0 }
	, m_buffer{ other.m_buffer }
{
}

void GThreadPacket::Clear() {
	m_buffer.Clear();
}

int GThreadPacket::GetBytes() {
	return m_buffer.Written();
}


template<class T>
size_t GThreadPacket::Write( T data ) {
	m_buffer.WriteBytes( (void*) &data, sizeof(T) );
	return sizeof(T);
}

size_t GThreadPacket::WriteData( const char* data, size_t len ) {
	m_buffer.WriteBytes( data, len );
	return len;
}

template<class T>
T GThreadPacket::Read() {
	auto data = unique_ptr<T>{ new T{} };
	m_buffer.ReadBytes( data.get(), sizeof(T) );
	return *data;
}

std::string GThreadPacket::ReadData( size_t len ) {
	return m_buffer.ReadBytes( len );
}

size_t GThreadPacket::Seek( Head head, Location loc, ptrdiff_t bytes ) {
	return m_buffer.Seek( head == Head::Write ? 0 : 1, loc == Location::Start ? 0 : ( loc == Location::Current ? 1 : 2 ), bytes );
}


void GThreadPacket::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadPacket" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		luaD_setcfunction( state, "__gc", _gc );

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

		luaD_setcfunction( state, "Seek", Seek );
	}
	lua_pop( state, 1 );
}

int GThreadPacket::PushGThreadPacket( lua_State* state, GThreadPacket* packet ) {
	if ( !packet ) {
		lua_pushnil( state );
		return 1;
	}
	GThreadPacketHandle* handle = (GThreadPacketHandle*) lua_newuserdata( state, sizeof(GThreadPacketHandle) );
	handle->object = packet;
	++(packet->m_references);
	luaL_getmetatable( state, "GThreadPacket" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadPacket::Create( lua_State* state ) {
	return PushGThreadPacket( state, new GThreadPacket() );
}

GThreadPacket* GThreadPacket::Get( lua_State* state, int narg ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, narg, "GThreadPacket" );
	if ( !handle->object ) luaL_error( state, "Invalid GThreadPacket" );

	return handle->object;
}

int GThreadPacket::_gc( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	if ( ! --handle->object->m_references )
		delete handle->object;

	handle->object = NULL;
	return 0;
}


template<class T>
int GThreadPacket::WriteNumber( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->Write<T>( static_cast<T>( luaL_checknumber( state, 2 ) ) ) );
	return 1;
}

int GThreadPacket::WriteData( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );
	auto asked_len = size_t( luaL_checknumber( state, 3 ) );

	lua_pushinteger( state, packet->WriteData( data, asked_len > len ? len : asked_len ) );
	return 1;
}

int GThreadPacket::WriteString( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );

	lua_pushinteger( state, packet->Write( len ) + handle->object->WriteData( data, len ) );
	return 1;
}


template<class T>
int GThreadPacket::ReadNumber( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;
	lua_pushnumber( state, lua_Number( packet->Read<T>() ) );
	return 1;
}

int GThreadPacket::ReadData( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	std::string data = packet->ReadData( size_t( luaL_checknumber( state, 2 ) ) );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

int GThreadPacket::ReadString( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	size_t len = packet->Read<size_t>();

	std::string data = packet->ReadData( len );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

int GThreadPacket::Seek( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	Head head = (Head) luaL_checkinteger( state, 2 );
	Location loc = (Location) luaL_checkinteger( state, 3 );
	ptrdiff_t bytes = (ptrdiff_t) luaL_checkinteger( state, 4 );

	lua_pushinteger( state, packet->Seek( head, loc, bytes ) );
	return 1;
}