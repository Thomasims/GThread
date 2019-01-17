#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include "lua_headers.h"
#include "def.h"
#include "GThreadChannel.h"
#include "Buffer.hpp"

class GThreadPacket {
	friend class GThreadChannel;
private:

	//Private functions

	template<class T>
	size_t Write( T data );
	size_t WriteData( const char* data, size_t len );

	template<class T>
	T Read();
	std::string ReadData( size_t len );

	size_t Seek( Head head, Location loc, ptrdiff_t bytes );
	size_t Slice( size_t start, size_t end );

private:

	int m_references{ 0 };
	Buffer m_buffer;

public:

	//Public functions
	GThreadPacket() = default;
	GThreadPacket( const GThreadPacket& );

	void Clear();
	int GetBytes();

	static void Setup( lua_State* state );

	static int PushGThreadPacket( lua_State* state, GThreadPacket* packet );
	static int Create( lua_State* state );

	static GThreadPacket* Get( lua_State* state, int narg );

	//Lua methods
	static int _gc( lua_State* state );

	using PacketGetter = GThreadPacket* (*)(lua_State*, int);

	template<PacketGetter, class T> static int WriteNumber( lua_State* );
	template<PacketGetter> static int WriteData( lua_State* );
	template<PacketGetter> static int WriteString( lua_State* );
	
	template<PacketGetter, class T> static int ReadNumber( lua_State* );
	template<PacketGetter> static int ReadData( lua_State* );
	template<PacketGetter> static int ReadString( lua_State* );

	static int Seek( lua_State* );
	static int GetSize( lua_State* );
	static int Slice( lua_State* );
};

typedef struct GThreadPacketHandle {
	GThreadPacket* object;
} GThreadPacketHandle;


template<GThreadPacket::PacketGetter GetP, class T>
int GThreadPacket::WriteNumber( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->Write<T>( static_cast<T>( luaL_checknumber( state, 2 ) ) ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::WriteData( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );
	auto asked_len = size_t( luaL_checknumber( state, 3 ) );

	lua_pushinteger( state, packet->WriteData( data, asked_len > len ? len : asked_len ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::WriteString( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );

	lua_pushinteger( state, packet->Write( len ) + packet->WriteData( data, len ) );
	return 1;
}


template<GThreadPacket::PacketGetter GetP, class T>
int GThreadPacket::ReadNumber( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;
	lua_pushnumber( state, lua_Number( packet->Read<T>() ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::ReadData( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	std::string data = packet->ReadData( size_t( luaL_checknumber( state, 2 ) ) );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::ReadString( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len = packet->Read<size_t>();

	std::string data = packet->ReadData( len );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}