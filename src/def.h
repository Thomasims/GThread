#pragma once

#include <new>

#define luaD_setcfunction( L, name, func ) \
	lua_pushcfunction( L, func ); \
	lua_setfield( L, -2, name );

#define luaD_setnumber( L, name, number ) \
	lua_pushnumber( L, number ); \
	lua_setfield( L, -2, name );

#define luaD_setstring( L, name, string ) \
	lua_pushstring( L, string ); \
	lua_setfield( L, -2, name );

template<class T, typename ...Args>
T* luaD_new( lua_State* state, Args&&... args ) {
	T* ptr = (T*) lua_newuserdata( state, sizeof(T) );
	new(ptr) T{ std::forward<Args>( args )... };
	return ptr;
}

template<class T>
void luaD_delete( T* ptr ) {
	ptr->~T();
}

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