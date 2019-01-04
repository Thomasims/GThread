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
	void ReattachLua( bool update = true );
	void Run(string code);

	static void ThreadMain( GThread* );

	thread* m_thread;
	map<string, DoubleChannel> m_channels;

	queue<string> m_codequeue;
	mutex m_codemtx;
	condition_variable m_codecvar;

	bool m_attached;

	unsigned int m_id;

	static unsigned int count;
	static map<unsigned int, GThread*> detached;
	static mutex detachedmtx;

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