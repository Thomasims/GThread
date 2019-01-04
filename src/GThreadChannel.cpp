#include "GThreadChannel.h"

GThreadChannel::GThreadChannel() {

}

GThreadChannel::~GThreadChannel() {

}


void GThreadChannel::SetupMetaFields(ILuaBase* LUA) {
	LUA_SET( CFunction, "__gc", __gc );
}

LUA_METHOD_IMPL(GThreadChannel::__gc) {
	return 0;
}

LUA_METHOD_IMPL(GThreadChannel::Create) {
	GThreadChannel* channel = new GThreadChannel();
	LUA->PushUserType(channel, TypeID_Channel);
	return 1;
}