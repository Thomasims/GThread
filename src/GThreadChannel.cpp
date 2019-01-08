#include "GThreadChannel.h"

GThreadChannel::GThreadChannel() {

}

GThreadChannel::~GThreadChannel() {

}


void GThreadChannel::Setup( lua_State* state ) {
	luaL_newmetatable( state, "GThreadChannel" );
	{
		lua_pushvalue( state, -1 );
		lua_setfield( state, -2, "__index" );

		lua_pushcfunction( state, _gc );
		lua_setfield( state, -2, "__gc" );
	}
	lua_pop( state, 1 );
}

int GThreadChannel::_gc( lua_State* state ) {
	return 0;
}

int GThreadChannel::PushGThreadChannel( lua_State* state, GThreadChannel* channel ) {
	GThreadChannelHandle* handle = (GThreadChannelHandle*) lua_newuserdata( state, sizeof(GThreadChannelHandle) );
	handle->channel = channel;
	luaL_getmetatable( state, "GThreadChannel" );
	lua_setmetatable( state, -2 );
	return 1;
}

int GThreadChannel::Create( lua_State* state ) {
	return PushGThreadChannel( state, new GThreadChannel() );
}