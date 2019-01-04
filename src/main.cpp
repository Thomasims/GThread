
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
		LUA_SET( CFunction, "getDetached", GThread::GetDetached );

		LUA_SET( Number, "CONTEXTTYPE_ISOLATED", ContextType::Isolated );
		
		LUA_SET( Number, "HEAD_W", Head::Write );
		LUA_SET( Number, "HEAD_R", Head::Read );
		
		LUA_SET( Number, "LOC_START", Location::Start );
		LUA_SET( Number, "LOC_CUR", Location::Current );
		LUA_SET( Number, "LOC_END", Location::End );
	}
	LUA->SetField(-2, "gthread");
	LUA->Pop();

	return 0;
}

GMOD_MODULE_CLOSE() {
	return 0;
}