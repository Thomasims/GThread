#include "GThreadPacket.h"

GThreadPacket::GThreadPacket() {

}

GThreadPacket::GThreadPacket( const GThreadPacket& other ) {

}

GThreadPacket::~GThreadPacket() {

}


void GThreadPacket::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadPacket" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );
		
		luaD_setcfunction( state, "__gc", _gc );
	}
	lua_pop( state, 1 );
}

int GThreadPacket::_gc( lua_State* state ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, 1, "GThreadPacket" );
	if ( !handle->packet ) return luaL_error( state, "Invalid GThreadPacket" );

	if ( ! --handle->packet->m_references )
		delete handle->packet;

	handle->packet = NULL;
	return 0;
}

int GThreadPacket::PushGThreadPacket( lua_State* state, GThreadPacket* packet ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) lua_newuserdata( state, sizeof(GThreadPacketHandle) );
	handle->packet = packet;
	++(packet->m_references);
	luaL_getmetatable( state, "GThreadPacket" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadPacket::Create( lua_State* state ) {
	return PushGThreadPacket( state, new GThreadPacket() );
}

GThreadPacket* GThreadPacket::Get( lua_State* state, int narg ) {
	GThreadPacketHandle* handle = (GThreadPacketHandle*) luaL_checkudata( state, narg, "GThreadPacket" );
	if ( !handle->packet ) luaL_error( state, "Invalid GThreadPacket" );

	return handle->packet;
}