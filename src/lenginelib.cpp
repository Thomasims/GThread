
#include "main.h"

struct handle {
	GThread* thread;
};

static int engine_block( lua_State* L ) {
	struct handle* handle = (struct handle*) luaL_checkudata( L, 1, "engine" );

	int nargs = lua_gettop( L ), actualnargs = 0;
	lua_Integer* args = new int[ nargs ];
	for( int i = 2; i <= nargs; i++ ) {
		lua_Integer ref = luaL_optinteger( L, i, 0 );
		if( ref ) {
			args[ actualnargs++ ] = ref;
		}
	}
	lua_Integer ret = handle->thread->Wait( args, actualnargs );

	if( !ret )
		return luaL_error( L, "Killed" );

	lua_pushinteger( L, ret );
	return 1;
}

int luaopen_engine ( lua_State *L, GThread* thread ) {
	struct handle* handle = (struct handle*) lua_newuserdata( L, sizeof(struct handle) );
	handle->thread = thread;

	luaL_newmetatable( L, "engine" );
	{
		lua_pushvalue( L, -1 );
		lua_setfield( L, -2, "__index" );

		lua_pushcfunction( L, engine_block );
		lua_setfield( L, -2, "Block" );
	}
	lua_setmetatable( L, -2 );

	lua_setfield( L, LUA_GLOBALSINDEX, "engine" );

	return 0;
}