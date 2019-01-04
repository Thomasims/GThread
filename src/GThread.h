#pragma once

#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "main.h"

extern int TypeID_Thread;

typedef struct DoubleChannel {
	GThreadChannel* outgoing = NULL;
	GThreadChannel* incoming = NULL;
} DoubleChannel;

class GThread {

private:

	//Private functions
	GThread();
	~GThread();

	void DetachLua();
	void ReattachLua();
	void Run(std::string code);

	static void ThreadMain( GThread* );

	std::thread* m_thread;
	std::map<std::string, DoubleChannel> m_channels;

	std::queue<std::string> m_codequeue;
	std::mutex m_codemtx;
	std::condition_variable m_codecvar;

	bool m_attached;

	static unsigned int count;
	static std::map<unsigned int, GThread*> detached;

public:

	//Public functions
	static void SetupMetaFields( ILuaBase* LUA );

	LUA_METHOD_DECL( Create );
	LUA_METHOD_DECL( GetDetached );

	//Lua methods
	LUA_METHOD_DECL( __gc );
	LUA_METHOD_DECL( Run );
	LUA_METHOD_DECL( OpenChannel );
	LUA_METHOD_DECL( AttachChannel );
	LUA_METHOD_DECL( Kill );

};