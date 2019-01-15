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

	template<class T>
	static int WriteNumber( lua_State* );
	static int WriteData( lua_State* );
	static int WriteString( lua_State* );
	
	template<class T>
	static int ReadNumber( lua_State* );
	static int ReadData( lua_State* );
	static int ReadString( lua_State* );

	static int Seek( lua_State* );
	static int GetSize( lua_State* );
	static int Slice( lua_State* );
};

typedef struct GThreadPacketHandle {
	GThreadPacket* object;
} GThreadPacketHandle;