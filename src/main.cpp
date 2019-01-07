
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <string>
#include "main.h"

using namespace GarrysMod::Lua;

GMOD_MODULE_OPEN() {
	GThread::Setup( state );
	GThreadChannel::Setup( state );
	GThreadPacket::Setup( state );

	lua_newtable( state );
	{
		luaD_setcfunction( state, "newThread", GThread::Create );
		luaD_setcfunction( state, "newChannel", GThreadChannel::Create );
		luaD_setcfunction( state, "newPacket", GThreadPacket::Create );
		luaD_setcfunction( state, "getDetached", GThread::GetDetached );

		luaD_setnumber( state, "CONTEXTTYPE_ISOLATED", ContextType::Isolated );
		
		luaD_setnumber( state, "HEAD_W", Head::Write );
		luaD_setnumber( state, "HEAD_R", Head::Read );
		
		luaD_setnumber( state, "LOC_START", Location::Start );
		luaD_setnumber( state, "LOC_CUR", Location::Current );
		luaD_setnumber( state, "LOC_END", Location::End );

		luaD_setstring( state, "Version", "0.1" );
	}
	lua_setglobal( state, "gthread" );

	return 0;
}

GMOD_MODULE_CLOSE() {
	return 0;
}