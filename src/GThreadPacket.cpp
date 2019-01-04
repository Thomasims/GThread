#include "GThreadPacket.h"

GThreadPacket::GThreadPacket() {

}

GThreadPacket::~GThreadPacket() {

}


void GThreadPacket::SetupMetaFields(ILuaBase* LUA) {
	LUA_SET( CFunction, "__gc", __gc );
}

LUA_METHOD_IMPL(GThreadPacket::__gc) {
	return 0;
}

LUA_METHOD_IMPL(GThreadPacket::Create) {
	GThreadPacket* packet = new GThreadPacket();
	LUA->PushUserType(packet, TypeID_Packet);
	return 1;
}