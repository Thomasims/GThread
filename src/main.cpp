
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <string>
#include "main.h"

int TypeID_Thread, TypeID_Channel, TypeID_Packet;

GMOD_MODULE_OPEN() {
	TypeID_Thread = LUA->CreateMetaTable("GThread");
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");

		GThread::SetupMetaFields(LUA);
	}
	LUA->Pop();
	
	TypeID_Channel = LUA->CreateMetaTable("GThreadChannel");
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");
		
		GThreadChannel::SetupMetaFields(LUA);
	}
	LUA->Pop();
	
	TypeID_Packet = LUA->CreateMetaTable("GThreadPacket");
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");
		
		GThreadPacket::SetupMetaFields(LUA);
	}
	LUA->Pop();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CreateTable();
	{
		LUA_SET( CFunction, "newThread", GThread::Create );
		LUA_SET( CFunction, "newChannel", GThreadChannel::Create );
		LUA_SET( CFunction, "newPacket", GThreadPacket::Create );
		LUA_SET( CFunction, "GetDetached", GThread::GetDetached );

		LUA_SET( Number, "CONTEXTTYPE_ISOLATED", 1 );
		
		LUA_SET( Number, "HEAD_W", 1 );
		LUA_SET( Number, "HEAD_R", 2 );
		
		LUA_SET( Number, "LOC_START", 1 );
		LUA_SET( Number, "LOC_CUR", 2 );
		LUA_SET( Number, "LOC_END", 3 );
	}
	LUA->SetField(-2, "gthread");
	LUA->Pop();

	return 0;
}

GMOD_MODULE_CLOSE() {
	return 0;
}