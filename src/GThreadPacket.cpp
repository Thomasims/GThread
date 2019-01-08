#include "GThreadPacket.h"

GThreadPacket::GThreadPacket() {
	m_references = 0;
}

GThreadPacket::GThreadPacket( const GThreadPacket& other ) {

}

GThreadPacket::~GThreadPacket() {

}

void GThreadPacket::Clear() {

}

int GThreadPacket::GetBits() {
	return 0;
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
	if ( !handle->object ) return 0;

	if ( ! --handle->object->m_references )
		delete handle->object;

	handle->object = NULL;
	return 0;
}

int GThreadPacket::PushGThreadPacket( lua_State* state, GThreadPacket* packet ) {
	if (!packet) {
		lua_pushnil(state);
		return 1;
	}
	GThreadPacketHandle* handle = (GThreadPacketHandle*) lua_newuserdata( state, sizeof(GThreadPacketHandle) );
	handle->object = packet;
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
	if ( !handle->object ) luaL_error( state, "Invalid GThreadPacket" );

	return handle->object;
}