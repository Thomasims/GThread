
#include <chrono>
#include <set>

#include "lua_headers.h"
#include "def.h"
#include "GThread.h"

static int engine_block( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );

	int nargs = lua_gettop( L );
	std::set<lua_Integer> args;
	for ( int i = 2; i <= nargs; ++i ) {
		lua_Integer ref = luaL_optinteger( L, i, 0 );
		if ( ref ) {
			args.insert( ref );
		}
	}

	return handle->object->Wait( L, args ) || luaL_error( L, "Killed" );
}

static int engine_createtimer( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );
	GThread* thread = handle->object;
	if (!thread) return luaL_error(L, "Invalid GThread");

	lua_pushinteger( L, thread->CreateTimer( std::chrono::system_clock::now() + std::chrono::milliseconds( luaL_checkinteger( L, 2 ) ) ) );

	return 1;
}

static int engine_openchannel( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*)luaL_checkudata(L, 1, "engine");
	GThread* thread = handle->object;
	if (!thread) return luaL_error(L, "Invalid GThread");

	const char* name = luaL_checkstring(L, 2);

	auto pair = thread->OpenChannels(name);

	return
		GThreadChannel::PushGThreadChannel(L, pair.incoming, thread) +
		GThreadChannel::PushGThreadChannel(L, pair.outgoing, thread);
}

static int engine__gc( lua_State* L ) {
	GThreadHandle* handle = (GThreadHandle*) luaL_checkudata( L, 1, "engine" );
	if ( !handle->object ) return 0;
	luaD_delete( handle );
	return 0;
}

int luaopen_engine( lua_State *L, GThread* thread ) {
	GThreadHandle* handle = luaD_new<GThreadHandle>( L, thread ); // TODO: Make a ctor/dtor

	GThreadChannel::Setup(L);
	GThreadPacket::Setup(L);

	luaL_newmetatable( L, "engine" );
	{
		lua_pushvalue( L, -1 );
		lua_setfield( L, -2, "__index" );

		luaD_setcfunction( L, "__gc", engine__gc );

		luaD_setcfunction( L, "Block", engine_block );
		luaD_setcfunction( L, "OpenChannel", engine_openchannel );
		luaD_setcfunction( L, "CreateTimer", engine_createtimer );
		
		//luaD_setcfunction( L, "newThread", GThread::Create ); // ? could allow this
		luaD_setcfunction( L, "newChannel", GThreadChannel::Create );
		luaD_setcfunction( L, "newPacket", GThreadPacket::Create );

		luaD_setnumber( L, "HEAD_W", Head::Write );
		luaD_setnumber( L, "HEAD_R", Head::Read );

		luaD_setnumber( L, "LOC_START", Location::Start );
		luaD_setnumber( L, "LOC_CUR", Location::Current );
		luaD_setnumber( L, "LOC_END", Location::End );
	}
	lua_setmetatable( L, -2 );

	lua_setglobal( L, "engine" );

	return 0;
}