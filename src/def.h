
#define LUA_METHOD_DECL( FUNC ) \
        static int FUNC##__Imp( GarrysMod::Lua::ILuaBase* LUA ); \
        static int FUNC( lua_State* L ) \
        { \
            GarrysMod::Lua::ILuaBase* LUA = L->luabase; \
            LUA->SetState(L); \
            return FUNC##__Imp( LUA ); \
        };

#define LUA_METHOD_IMPL( FUNC ) \
        int FUNC##__Imp( GarrysMod::Lua::ILuaBase* LUA )

#define LUA_SET( TYPE, NAME, VAL ) \
        LUA->Push##TYPE(VAL); \
        LUA->SetField(-2, NAME)


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