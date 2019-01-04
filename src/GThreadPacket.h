#pragma once

#include "main.h"

extern int TypeID_Packet;

class GThreadPacket {

private:

	//Private functions
	~GThreadPacket();

public:

	//Public functions
	GThreadPacket();
	static void SetupMetaFields( ILuaBase* LUA );

	LUA_METHOD_DECL( Create );

	//Lua methods
	LUA_METHOD_DECL( __gc );

};