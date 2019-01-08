#include "GThreadPacket.h"

GThreadPacket::GThreadPacket() {

}

GThreadPacket::~GThreadPacket() {

}


void GThreadPacket::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadPacket" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		lua_pushcfunction( state, _gc );
		lua_setfield( state, -2, "__gc" );
	}
	lua_pop( state, 1 );
}

int GThreadPacket::_gc( lua_State* state ) {
	return 0;
}

int GThreadPacket::PushGThreadPacket( lua_State* state, GThreadPacket* packet ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) lua_newuserdata( state, sizeof(GThreadPacketHandle) );
	handle->packet = packet;
	luaL_getmetatable( state, "GThreadPacket" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadPacket::Create( lua_State* state ) {
	return PushGThreadPacket( state, new GThreadPacket() );
}