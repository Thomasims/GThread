#include "GThreadPacket.h"

GThreadPacket::GThreadPacket() {
	m_references = 0;
}

GThreadPacket::GThreadPacket( const GThreadPacket& other ) {

}

GThreadPacket::~GThreadPacket() {

}

void GThreadPacket::Clear() {

}

int GThreadPacket::GetBits() {
	return 0;
}


template<class T>
int GThreadPacket::Write( lua_State* state, T data ) {
	m_buffer.WriteBytes( (void*) &data, sizeof(T) );
	lua_pushinteger( state, sizeof(T) );
	return 1;
}

int GThreadPacket::WriteData( lua_State* state, const char* data, size_t len ) {
	m_buffer.WriteBytes( data, len );
	lua_pushinteger( state, len );
	return 1;
}

template<class T>
T GThreadPacket::Read() {
	T data;
	m_buffer.ReadBytes( &data, sizeof(T) );
	return data;
}

int GThreadPacket::ReadData( lua_State* state, size_t len ) {
	std::string dt = m_buffer.ReadBytes( len );
	lua_pushlstring( state, dt.data(), dt.length() );
	return 1;
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
		luaD_setcfunction( state, "WriteUByte", WriteNumber<int8_t> );
		luaD_setcfunction( state, "WriteUShort", WriteNumber<int16_t> );
		luaD_setcfunction( state, "WriteUInt", WriteNumber<int32_t> );
		luaD_setcfunction( state, "WriteULong", WriteNumber<int64_t> );
		luaD_setcfunction( state, "WriteFloat", WriteNumber<float> );
		luaD_setcfunction( state, "WriteDouble", WriteNumber<double> );
		luaD_setcfunction( state, "WriteData", WriteData );
		luaD_setcfunction( state, "WriteString", WriteString );
		
		luaD_setcfunction( state, "ReadByte", ReadNumber<int8_t> );
		luaD_setcfunction( state, "ReadShort", ReadNumber<int16_t> );
		luaD_setcfunction( state, "ReadInt", ReadNumber<int32_t> );
		luaD_setcfunction( state, "ReadLong", ReadNumber<int64_t> );
		luaD_setcfunction( state, "ReadUByte", ReadNumber<int8_t> );
		luaD_setcfunction( state, "ReadUShort", ReadNumber<int16_t> );
		luaD_setcfunction( state, "ReadUInt", ReadNumber<int32_t> );
		luaD_setcfunction( state, "ReadULong", ReadNumber<int64_t> );
		luaD_setcfunction( state, "ReadFloat", ReadNumber<float> );
		luaD_setcfunction( state, "ReadDouble", ReadNumber<double> );
		luaD_setcfunction( state, "ReadData", ReadData );
		luaD_setcfunction( state, "ReadString", ReadString );
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


/*#define WriteF(TYPE, CTYPE) \
	int GThreadPacket::Write ## TYPE( lua_State* state ) \
{\
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" ); \
if ( !handle->object ) return 0; \
	return handle->object->Write( state, CTYPE( luaL_checknumber( state, 2 ) ) ); \
}

WriteF( Byte, int8_t )
WriteF( Short, int16_t )
WriteF( Int, int32_t )
WriteF( Long, int64_t )
WriteF( UByte, uint8_t )
WriteF( UShort, uint16_t )
WriteF( UInt, uint32_t )
WriteF( ULong, uint64_t )
WriteF( Float, float )
WriteF( Double, double )*/

template<class T>
static int GThreadPacket::WriteNumber( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;
	return handle->object->Write<T>( state, static_cast<T>( luaL_checknumber( state, 2 ) ) );
}

int GThreadPacket::WriteData( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );
	auto asked_len = size_t( luaL_checknumber( state, 3 ) );

	return handle->object->WriteData( state, data, asked_len > len ? len : asked_len );
}

int GThreadPacket::WriteString( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );

	return handle->object->Write( state, len ) + handle->object->WriteData( state, data, len );
}


/*#define ReadF(TYPE, CTYPE) \
	int GThreadPacket::Read ## TYPE( lua_State* state ) \
{ \
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" ); \
if ( !handle->object ) return 0; \
	lua_pushnumber( state, lua_Number( handle->object->Read<CTYPE>() ) ); \
	return 1; \
}

ReadF( Byte, int8_t )
ReadF( Short, int16_t )
ReadF( Int, int32_t )
ReadF( Long, int64_t )
ReadF( UByte, uint8_t )
ReadF( UShort, uint16_t )
ReadF( UInt, uint32_t )
ReadF( ULong, uint64_t )
ReadF( Float, float )
ReadF( Double, double )*/

template<class T>
static int GThreadPacket::ReadNumber( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;
	lua_pushnumber( state, lua_Number( handle->object->Read<T>() ) );
	return 1;
}

int GThreadPacket::ReadData( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	return handle->object->ReadData( state, size_t( luaL_checknumber( state, 2 ) ) );
}

int GThreadPacket::ReadString( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->object ) return 0;

	size_t len = handle->object->Read<size_t>();

	return handle->object->ReadData( state, len );
}