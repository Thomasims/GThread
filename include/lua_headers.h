/*
	Function headers for most of the functions exported in lua_shared.dll
	Only a few of these were tested, no guarantees they all work.
*/

#ifndef LUA_HEADERS_H
#define LUA_HEADERS_H

#include <stddef.h>
#include <stdarg.h>

typedef struct lua_State lua_State;

#ifdef _WIN32
    #define DLL_EXPORT extern "C" __declspec( dllexport )
#else
    #define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif


// TODO: Clean up this mess

#define LUA_MULTRET	(-1)

/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))


/* thread status; 0 is OK */
#define LUA_OK        0
#define LUA_YIELD     1
#define LUA_ERRRUN    2
#define LUA_ERRSYNTAX 3
#define LUA_ERRMEM    4
#define LUA_ERRERR    5

/*
** basic types
*/
#define LUA_TNONE		(-1)

#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8


/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20

/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define lua_pop(L,n)		lua_settop(L, -(n)-1)

#define lua_newtable(L)		lua_createtable(L, 0, 0)

#define lua_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))

#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)

#define lua_strlen(L,i)		lua_objlen(L, (i))

#define lua_isfunction(L,n)	(lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)	(lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)		(lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L, (n)) <= 0)

#define lua_pushliteral(L, s)	\
	lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)

#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_getglobal(L,s)	lua_getfield(L, LUA_GLOBALSINDEX, (s))

#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)

#define luaL_newlibtable(L,l)	\
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)  \
  (luaL_checkversion(L), luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

#define luaL_argcheck(L, cond,arg,extramsg)	\
		((void)((cond) || luaL_argerror(L, (arg), (extramsg))))

#define luaL_argexpected(L,cond,arg,tname)	\
		((void)((cond) || luaL_argerror(L, (arg), (tname))))

#define luaL_checkstring(L,n)	(luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)	(luaL_optlstring(L, (n), (d), NULL))

#define luaL_typename(L,i)	lua_typename(L, lua_type(L,(i)))

#define luaL_dofile(L, fn) \
	(luaL_loadfile(L, fn) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_dostring(L, s) \
	(luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_getmetatable(L,n)	(lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_opt(L,f,n,d)	(lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))


typedef double lua_Number;
typedef ptrdiff_t lua_Integer;
typedef struct lua_Debug {
	int event;
	const char *name;           /* (n) */
	const char *namewhat;       /* (n) */
	const char *what;           /* (S) */
	const char *source;         /* (S) */
	int currentline;            /* (l) */
	int nups;                   /* (u) number of upvalues */
	int linedefined;            /* (S) */
	int lastlinedefined;        /* (S) */
	char short_src[60];         /* (S) */
	/* private part */
	int i_ci;
} lua_Debug;

typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
typedef int(*lua_CFunction) (lua_State *L);
typedef const char * (*lua_Reader) (lua_State *L, void *data, size_t *size);
typedef int(*lua_Writer) (lua_State *L, const void* p, size_t sz, void* ud);
typedef void(*lua_Hook) (lua_State *L, lua_Debug *ar);

typedef struct luaL_Buffer {
	char *p; /* current position in buffer */
	int lvl; /* number of strings in the stack (level) */
	lua_State *L;
	char buffer[256];
} luaL_Buffer;

typedef struct luaL_Reg {
	const char *name;
	lua_CFunction func;
} luaL_Reg;

//                        CreateInterface
//                        GMOD_LoadBinaryModule
//                        cvar
DLL_EXPORT int            luaJIT_setmode       (lua_State *L, int idx, int mode);
//                        luaJIT_version_2_0_4
DLL_EXPORT void           luaL_addlstring      (luaL_Buffer *B, const char *s, size_t l);
DLL_EXPORT void           luaL_addstring       (luaL_Buffer *B, const char *s);
DLL_EXPORT void           luaL_addvalue        (luaL_Buffer *B);
DLL_EXPORT int            luaL_argerror        (lua_State *L, int narg, const char *extramsg);
DLL_EXPORT void           luaL_buffinit        (lua_State *L, luaL_Buffer *B);
DLL_EXPORT int            luaL_callmeta        (lua_State *L, int obj, const char *e);
DLL_EXPORT void           luaL_checkany        (lua_State *L, int narg);
DLL_EXPORT lua_Integer    luaL_checkinteger    (lua_State *L, int narg);
DLL_EXPORT const char    *luaL_checklstring    (lua_State *L, int narg, size_t *l);
DLL_EXPORT lua_Number     luaL_checknumber     (lua_State *L, int narg);
DLL_EXPORT int            luaL_checkoption     (lua_State *L, int narg, const char *def, const char *const lst[]);
DLL_EXPORT void           luaL_checkstack      (lua_State *L, int sz, const char *msg);
DLL_EXPORT void           luaL_checktype       (lua_State *L, int narg, int t);
DLL_EXPORT void          *luaL_checkudata      (lua_State *L, int narg, const char *tname);
DLL_EXPORT int            luaL_error           (lua_State *L, const char *fmt, ...);
DLL_EXPORT int            luaL_execresult      (lua_State *L, int stat);
DLL_EXPORT int            luaL_fileresult      (lua_State *L, int stat, const char *fname);
DLL_EXPORT const char    *luaL_findtable       (lua_State *L, int idx, const char *fname, int szhint);
DLL_EXPORT int            luaL_getmetafield    (lua_State *L, int obj, const char *e);
DLL_EXPORT const char    *luaL_gsub            (lua_State *L, const char *s, const char *p, const char *r);
DLL_EXPORT int            luaL_loadbuffer      (lua_State *L, const char *buff, size_t sz, const char *name);
DLL_EXPORT int            luaL_loadbufferx     (lua_State *L, const char *buff, size_t size, const char *name, const char *mode);
DLL_EXPORT int            luaL_loadfile        (lua_State *L, const char *filename);
DLL_EXPORT int            luaL_loadfilex       (lua_State *L, const char *filename, const char *mode);
DLL_EXPORT int            luaL_loadstring      (lua_State *L, const char *s);
DLL_EXPORT int            luaL_newmetatable    (lua_State *L, const char *tname);
//                        luaL_newmetatable_type?
DLL_EXPORT lua_State     *luaL_newstate        (void);
DLL_EXPORT void           luaL_openlib         (lua_State *L, const char *libname, const luaL_Reg *l, int nup);
DLL_EXPORT void           luaL_openlibs        (lua_State *L);
DLL_EXPORT lua_Integer    luaL_optinteger      (lua_State *L, int narg, lua_Integer d);
DLL_EXPORT const char    *luaL_optlstring      (lua_State *L, int narg, const char *d, size_t *l);
DLL_EXPORT lua_Number     luaL_optnumber       (lua_State *L, int narg, lua_Number d);
DLL_EXPORT char          *luaL_prepbuffer      (luaL_Buffer *B);
DLL_EXPORT void           luaL_pushresult      (luaL_Buffer *B);
DLL_EXPORT int            luaL_ref             (lua_State *L, int t);
DLL_EXPORT void           luaL_register        (lua_State *L, const char *libname, const luaL_Reg *l);
DLL_EXPORT void           luaL_traceback       (lua_State *L, lua_State *L1, const char *msg, int level);
DLL_EXPORT int            luaL_typerror        (lua_State *L, int narg, const char *tname);
DLL_EXPORT void           luaL_unref           (lua_State *L, int t, int ref);
DLL_EXPORT void           luaL_where           (lua_State *L, int lvl);
DLL_EXPORT lua_CFunction  lua_atpanic          (lua_State *L, lua_CFunction panicf);
DLL_EXPORT void           lua_call             (lua_State *L, int nargs, int nresults);
DLL_EXPORT int            lua_checkstack       (lua_State *L, int extra);
DLL_EXPORT void           lua_close            (lua_State *L);
DLL_EXPORT void           lua_concat           (lua_State *L, int n);
DLL_EXPORT int            lua_cpcall           (lua_State *L, lua_CFunction func, void *ud);
DLL_EXPORT void           lua_createtable      (lua_State *L, int narr, int nrec);
DLL_EXPORT int            lua_dump             (lua_State *L, lua_Writer writer, void *data);
DLL_EXPORT int            lua_equal            (lua_State *L, int index1, int index2);
DLL_EXPORT int            lua_error            (lua_State *L);
DLL_EXPORT int            lua_gc               (lua_State *L, int what, int data);
DLL_EXPORT lua_Alloc      lua_getallocf        (lua_State *L, void **ud);
DLL_EXPORT void           lua_getfenv          (lua_State *L, int index);
DLL_EXPORT void           lua_getfield         (lua_State *L, int index, const char *k);
DLL_EXPORT lua_Hook       lua_gethook          (lua_State *L);
DLL_EXPORT int            lua_gethookcount     (lua_State *L);
DLL_EXPORT int            lua_gethookmask      (lua_State *L);
DLL_EXPORT int            lua_getinfo          (lua_State *L, const char *what, lua_Debug *ar);
DLL_EXPORT const char    *lua_getlocal         (lua_State *L, lua_Debug *ar, int n);
DLL_EXPORT int            lua_getmetatable     (lua_State *L, int index);
DLL_EXPORT int            lua_getstack         (lua_State *L, int level, lua_Debug *ar);
DLL_EXPORT void           lua_gettable         (lua_State *L, int index);
DLL_EXPORT int            lua_gettop           (lua_State *L);
DLL_EXPORT const char    *lua_getupvalue       (lua_State *L, int funcindex, int n);
DLL_EXPORT void           lua_insert           (lua_State *L, int index);
DLL_EXPORT int            lua_iscfunction      (lua_State *L, int index);
DLL_EXPORT int            lua_isnumber         (lua_State *L, int index);
DLL_EXPORT int            lua_isstring         (lua_State *L, int index);
DLL_EXPORT int            lua_isuserdata       (lua_State *L, int index);
DLL_EXPORT int            lua_lessthan         (lua_State *L, int index1, int index2);
DLL_EXPORT int            lua_load             (lua_State *L, lua_Reader reader, void *data, const char *chunkname);
DLL_EXPORT int            lua_loadx            (lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode);
DLL_EXPORT lua_State     *lua_newstate         (lua_Alloc f, void *ud);
DLL_EXPORT lua_State     *lua_newthread        (lua_State *L);
DLL_EXPORT void          *lua_newuserdata      (lua_State *L, size_t size);
DLL_EXPORT int            lua_next             (lua_State *L, int index);
DLL_EXPORT size_t         lua_objlen           (lua_State *L, int index);
DLL_EXPORT int            lua_pcall            (lua_State *L, int nargs, int nresults, int errfunc);
DLL_EXPORT void           lua_pushboolean      (lua_State *L, int b);
DLL_EXPORT void           lua_pushcclosure     (lua_State *L, lua_CFunction fn, int n);
DLL_EXPORT const char    *lua_pushfstring      (lua_State *L, const char *fmt, ...);
DLL_EXPORT void           lua_pushinteger      (lua_State *L, lua_Integer n);
DLL_EXPORT void           lua_pushlightuserdata(lua_State *L, void *p);
DLL_EXPORT void           lua_pushlstring      (lua_State *L, const char *s, size_t len);
DLL_EXPORT void           lua_pushnil          (lua_State *L);
DLL_EXPORT void           lua_pushnumber       (lua_State *L, lua_Number n);
DLL_EXPORT void           lua_pushstring       (lua_State *L, const char *s);
DLL_EXPORT int            lua_pushthread       (lua_State *L);
DLL_EXPORT void           lua_pushvalue        (lua_State *L, int index);
DLL_EXPORT const char    *lua_pushvfstring     (lua_State *L, const char *fmt, va_list argp);
DLL_EXPORT int            lua_rawequal         (lua_State *L, int index1, int index2);
DLL_EXPORT void           lua_rawget           (lua_State *L, int index);
DLL_EXPORT void           lua_rawgeti          (lua_State *L, int index, int n);
DLL_EXPORT void           lua_rawset           (lua_State *L, int index);
DLL_EXPORT void           lua_rawseti          (lua_State *L, int index, int n);
DLL_EXPORT void           lua_remove           (lua_State *L, int index);
DLL_EXPORT void           lua_replace          (lua_State *L, int index);
DLL_EXPORT int            lua_resume_real      (lua_State *L, int narg);
DLL_EXPORT void           lua_setallocf        (lua_State *L, lua_Alloc f, void *ud);
DLL_EXPORT int            lua_setfenv          (lua_State *L, int index);
DLL_EXPORT void           lua_setfield         (lua_State *L, int index, const char *k);
DLL_EXPORT int            lua_sethook          (lua_State *L, lua_Hook f, int mask, int count);
DLL_EXPORT const char    *lua_setlocal         (lua_State *L, lua_Debug *ar, int n);
DLL_EXPORT int            lua_setmetatable     (lua_State *L, int index);
DLL_EXPORT void           lua_settable         (lua_State *L, int index);
DLL_EXPORT void           lua_settop           (lua_State *L, int index);
DLL_EXPORT const char    *lua_setupvalue       (lua_State *L, int funcindex, int n);
DLL_EXPORT int            lua_status           (lua_State *L);
DLL_EXPORT int            lua_toboolean        (lua_State *L, int index);
DLL_EXPORT lua_CFunction  lua_tocfunction      (lua_State *L, int index);
DLL_EXPORT lua_Integer    lua_tointeger        (lua_State *L, int index);
DLL_EXPORT const char    *lua_tolstring        (lua_State *L, int index, size_t *len);
DLL_EXPORT lua_Number     lua_tonumber         (lua_State *L, int index);
DLL_EXPORT const void    *lua_topointer        (lua_State *L, int index);
DLL_EXPORT lua_State     *lua_tothread         (lua_State *L, int index);
DLL_EXPORT void          *lua_touserdata       (lua_State *L, int index);
DLL_EXPORT int            lua_type             (lua_State *L, int index);
DLL_EXPORT const char    *lua_typename         (lua_State *L, int tp);
DLL_EXPORT void          *lua_upvalueid        (lua_State *L, int idx, int n);
DLL_EXPORT void           lua_upvaluejoin      (lua_State *L, int idx1, int n1, int idx2, int n2);
DLL_EXPORT void           lua_xmove            (lua_State *from, lua_State *to, int n);
DLL_EXPORT int            lua_yield            (lua_State *L, int nresults);
DLL_EXPORT int            luaopen_base         (lua_State *L);
DLL_EXPORT int            luaopen_bit          (lua_State *L);
DLL_EXPORT int            luaopen_debug        (lua_State *L);
DLL_EXPORT int            luaopen_jit          (lua_State *L);
DLL_EXPORT int            luaopen_math         (lua_State *L);
DLL_EXPORT int            luaopen_os           (lua_State *L);
DLL_EXPORT int            luaopen_package      (lua_State *L);
DLL_EXPORT int            luaopen_string       (lua_State *L);
DLL_EXPORT int            luaopen_table        (lua_State *L);

// UTILITY

#ifdef __cplusplus

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
	new(ptr) T( std::forward<Args>( args )... );
	return ptr;
}

template<class T>
void luaD_delete( T* ptr ) {
	ptr->~T();
}

#endif // __cplusplus

#endif // LUA_HEADERS_H