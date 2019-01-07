#pragma once

#include "lua_headers.h"
#include "def.h"

class GThreadPacket {

private:

	//Private functions

public:

	//Public functions
	GThreadPacket();
	virtual ~GThreadPacket();

	static void Setup( lua_State* state );
	
	static int PushGThreadPacket( lua_State*, GThreadPacket* );
	static int Create( lua_State* );

	//Lua methods
	static int _gc( lua_State* );
};

typedef struct GThreadPacketHandle {
	GThreadPacket* packet;
} GThreadPacketHandle;