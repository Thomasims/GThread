
#include "lua_headers.h"
#include "GThread.h"

static int engine_block( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );

	int nargs = lua_gettop( L ), actualnargs = 0;
	lua_Integer* args = new int[nargs];
	for ( int i = 2; i <= nargs; ++i ) {
		lua_Integer ref = luaL_optinteger( L, i, 0 );
		if ( ref ) {
			args[actualnargs++] = ref;
		}
	}

	return handle->thread->Wait( L, args, actualnargs ) || luaL_error( L, "Killed" );
}

static int engine_openchannel( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );
	return 0;
}

static int engine_createtimer( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );
	return 0;
}

int luaopen_engine( lua_State *L, GThread* thread ) {
	GThreadHandle* handle = (GThreadHandle*) lua_newuserdata( L, sizeof(GThreadHandle) );
	handle->thread = thread;

	luaL_newmetatable( L, "engine" );
	{
		lua_pushvalue( L, -1 );
		lua_setfield( L, -2, "__index" );

		lua_pushcfunction( L, engine_block );
		lua_setfield( L, -2, "Block" );

		lua_pushcfunction( L, engine_openchannel );
		lua_setfield( L, -2, "OpenChannel" );

		lua_pushcfunction( L, engine_createtimer );
		lua_setfield( L, -2, "CreateTimer" );
	}
	lua_setmetatable( L, -2 );

	lua_setglobal( L, "engine" );

	return 0;
}