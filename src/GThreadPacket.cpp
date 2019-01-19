#include "GThread.h"

using namespace std;

GThreadPacket::GThreadPacket( const GThreadPacket& other )
	: m_references{ 0 }
	, m_buffer{ other.m_buffer }
	, m_tag{ other.m_tag }
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

size_t GThreadPacket::Slice( size_t start, size_t end ) {
	return m_buffer.Slice( start, end > start ? end - start : 0 );
}


void GThreadPacket::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadPacket" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		luaD_setcfunction( state, "__gc", _gc );

		luaD_setcfunction( state, "WriteByte"    , WriteNumber   <Get, int8_t> );
		luaD_setcfunction( state, "WriteShort"   , WriteNumber   <Get, int16_t> );
		luaD_setcfunction( state, "WriteInt"     , WriteNumber   <Get, int32_t> );
		luaD_setcfunction( state, "WriteLong"    , WriteNumber   <Get, int64_t> );
		luaD_setcfunction( state, "WriteUByte"   , WriteNumber   <Get, uint8_t> );
		luaD_setcfunction( state, "WriteUShort"  , WriteNumber   <Get, uint16_t> );
		luaD_setcfunction( state, "WriteUInt"    , WriteNumber   <Get, uint32_t> );
		luaD_setcfunction( state, "WriteULong"   , WriteNumber   <Get, uint64_t> );
		luaD_setcfunction( state, "WriteSize"    , WriteNumber   <Get, size_t> );
		luaD_setcfunction( state, "WriteFloat"   , WriteNumber   <Get, float> );
		luaD_setcfunction( state, "WriteDouble"  , WriteNumber   <Get, double> );
		luaD_setcfunction( state, "WriteData"    , WriteData     <Get> );
		luaD_setcfunction( state, "WriteString"  , WriteString   <Get> );
		luaD_setcfunction( state, "WriteFunction", WriteFunction <Get> );
		
		luaD_setcfunction( state, "ReadByte"    , ReadNumber   <Get, int8_t> );
		luaD_setcfunction( state, "ReadShort"   , ReadNumber   <Get, int16_t> );
		luaD_setcfunction( state, "ReadInt"     , ReadNumber   <Get, int32_t> );
		luaD_setcfunction( state, "ReadLong"    , ReadNumber   <Get, int64_t> );
		luaD_setcfunction( state, "ReadUByte"   , ReadNumber   <Get, uint8_t> );
		luaD_setcfunction( state, "ReadUShort"  , ReadNumber   <Get, uint16_t> );
		luaD_setcfunction( state, "ReadUInt"    , ReadNumber   <Get, uint32_t> );
		luaD_setcfunction( state, "ReadULong"   , ReadNumber   <Get, uint64_t> );
		luaD_setcfunction( state, "ReadSize"    , ReadNumber   <Get, size_t> );
		luaD_setcfunction( state, "ReadFloat"   , ReadNumber   <Get, float> );
		luaD_setcfunction( state, "ReadDouble"  , ReadNumber   <Get, double> );
		luaD_setcfunction( state, "ReadData"    , ReadData     <Get> );
		luaD_setcfunction( state, "ReadString"  , ReadString   <Get> );
		luaD_setcfunction( state, "ReadFunction", ReadFunction <Get> );

		luaD_setcfunction( state, "Seek", Seek );
		luaD_setcfunction( state, "GetSize", GetSize );
		luaD_setcfunction( state, "Slice", Slice );
		luaD_setcfunction( state, "SetFilter", SetFilter );
		luaD_setcfunction( state, "GetFilter", GetFilter );
	}
	lua_pop( state, 1 );
}

int GThreadPacket::PushGThreadPacket( lua_State* state, GThreadPacket* packet ) {
	if ( !packet ) {
		lua_pushnil( state );
		return 1;
	}

	GThreadPacketHandle* handle = luaD_new<GThreadPacketHandle>( state, packet );
	luaL_getmetatable( state, "GThreadPacket" );
	lua_setmetatable( state, -2 );

	return 1;
}

int GThreadPacket::Create( lua_State* state ) {
	GThreadPacket* packet = new GThreadPacket();

	if ( lua_gettop( state ) > 0 ) {
		packet->m_tag = luaL_checkinteger( state, 1 );
	}

	return PushGThreadPacket( state, packet );
}

GThreadPacket* GThreadPacket::Get( lua_State* state, int narg ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, narg, "GThreadPacket" );
	if ( !handle->object ) luaL_error( state, "Invalid GThreadPacket" );

	return handle->object;
}

int GThreadPacket::_gc( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	luaD_delete( handle );

	return 0;
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

int GThreadPacket::GetSize( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->GetBytes() );
	return 1;
}

int GThreadPacket::Slice( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	size_t start = luaL_checkinteger( state, 2 );
	size_t end = -1;
	if ( lua_gettop( state ) > 2 )
		end = luaL_checkinteger( state, 3 );

	lua_pushinteger( state, packet->Slice( start, end ) );
	return 1;

}

int GThreadPacket::SetFilter( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	packet->m_tag = luaL_checkinteger( state, 1 );

	return 0;
}

int GThreadPacket::GetFilter( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	GThreadPacket* packet = handle->object;
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->m_tag );

	return 1;
}