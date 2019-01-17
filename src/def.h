#pragma once

#include <new>

void inline luaD_setcfunction( lua_State* state, const char* name, int( *func )(lua_State*), int upvalues = 0 ) {
	lua_pushcclosure( state, func, upvalues );
	lua_setfield( state, -2, name );
}
void inline luaD_setnumber( lua_State* state, const char* name, lua_Number number ) {
	lua_pushnumber( state, number );
	lua_setfield( state, -2, name );
}
void inline luaD_setstring( lua_State* state, const char* name, const char* string ) {
	lua_pushstring( state, string );
	lua_setfield( state, -2, name );
}

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