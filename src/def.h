#pragma once

#define luaD_setcfunction( L, name, func ) \
	lua_pushcfunction( L, func ); \
	lua_setfield( L, -2, name );

#define luaD_setnumber( L, name, number ) \
	lua_pushnumber( L, number ); \
	lua_setfield( L, -2, name );

#define luaD_setstring( L, name, string ) \
	lua_pushstring( L, string ); \
	lua_setfield( L, -2, name );

enum ContextType {
	Isolated
};

enum Head {
	Write,
	Read
};

enum Location {
	Start,
	Current,
	End
};