#ifndef DEF_H
#define DEF_H

#if _WIN32 && _MSC_VER <= 1800
#define noexcept
#endif

class GThread;
class GThreadChannel;
class GThreadPacket;

int luaopen_engine( lua_State *L, GThread* thread );
int luaopen_coroutine (lua_State *L); //ported from lua source because lua_shared.dll doesn't have it

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

#endif // DEF_H