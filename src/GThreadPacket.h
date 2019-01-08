#pragma once

#include "lua_headers.h"
#include "def.h"

class GThreadPacket {

private:

	//Private functions

private:

	int m_references;

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
};

typedef struct GThreadPacketHandle {
	GThreadPacket* object;
} GThreadPacketHandle;