#pragma once

#include <string>
#include <cstdint>

#include "lua_headers.h"
#include "def.h"
#include "GThreadChannel.h"
#include "Buffer.hpp"

class GThreadPacket {
	friend class GThreadChannel;
private:

	//Private functions

	template<class T>
	int Write( lua_State* state, T data );

	int WriteData( lua_State* state, const char* data, size_t len );


	template<class T>
	T Read();

	int ReadData( lua_State* state, size_t len );

private:

	int m_references;
	Buffer m_buffer;

public:

	//Public functions
	GThreadPacket();
	GThreadPacket( const GThreadPacket& );
	virtual ~GThreadPacket();

	void Clear();
	int GetBits();

	static void Setup( lua_State* state );

	static int PushGThreadPacket( lua_State* state, GThreadPacket* packet );
	static int Create( lua_State* state );

	static GThreadPacket* Get( lua_State* state, int narg );

	//Lua methods
	static int _gc( lua_State* state );

	template<class T>
	static int WriteNumber( lua_State* );
	/*static int WriteByte( lua_State* );
	static int WriteShort( lua_State* );
	static int WriteInt( lua_State* );
	static int WriteLong( lua_State* );
	static int WriteUByte( lua_State* );
	static int WriteUShort( lua_State* );
	static int WriteUInt( lua_State* );
	static int WriteULong( lua_State* );
	static int WriteFloat( lua_State* );
	static int WriteDouble( lua_State* );*/
	static int WriteData( lua_State* );
	static int WriteString( lua_State* );
	
	template<class T>
	static int ReadNumber( lua_State* );
	/*static int ReadByte( lua_State* );
	static int ReadShort( lua_State* );
	static int ReadInt( lua_State* );
	static int ReadLong( lua_State* );
	static int ReadUByte( lua_State* );
	static int ReadUShort( lua_State* );
	static int ReadUInt( lua_State* );
	static int ReadULong( lua_State* );
	static int ReadFloat( lua_State* );
	static int ReadDouble( lua_State* );*/
	static int ReadData( lua_State* );
	static int ReadString( lua_State* );
};

typedef struct GThreadPacketHandle {
	GThreadPacket* object;
} GThreadPacketHandle;