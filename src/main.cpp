
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <string>

#include "lua_headers.h"
#include "def.h"

#include "GThread.h"
#include "GThreadChannel.h"
#include "GThreadPacket.h"

DLL_EXPORT int gmod13_open( lua_State* state ) {
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

DLL_EXPORT int gmod13_close( lua_State* state ) {
	return 0;
}