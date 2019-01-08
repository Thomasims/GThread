#pragma once

#include "lua_headers.h"
#include "def.h"
#include "GThreadPacket.h"

class GThreadChannel {

private:

	//Private functions

public:

	//Public functions
	GThreadChannel();
	virtual ~GThreadChannel();

	static void Setup( lua_State* state );

	static int PushGThreadChannel( lua_State*, GThreadChannel* );
	static int Create( lua_State* );

	//Lua methods
	static int _gc( lua_State* );
};

typedef struct GThreadChannelHandle {
	GThreadChannel* channel;
} GThreadChannelHandle;