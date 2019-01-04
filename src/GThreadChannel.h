#pragma once

#include "main.h"

extern int TypeID_Channel;

class GThreadChannel {

private:

	//Private functions
	~GThreadChannel();

public:

	//Public functions
	GThreadChannel();
	static void SetupMetaFields( ILuaBase* LUA );

	LUA_METHOD_DECL( Create );

	//Lua methods
	LUA_METHOD_DECL( __gc );

};