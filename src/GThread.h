#pragma once

#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>

#include "lua_headers.h"
#include "def.h"

using namespace std;

class Notifier;
class GThreadChannel;

typedef struct DoubleChannel {
	class GThreadChannel* outgoing;
	class GThreadChannel* incoming;
} DoubleChannel;

class GThread {

private:

	//Private functions
	GThread();

	void DetachLua();
	void ReattachLua( bool update = true );
	void Run( string code );

	static void ThreadMain( GThread* );

private:

	thread* m_thread;
	map<string, DoubleChannel> m_channels;

	map<lua_Integer, Notifier*> m_notifiers;
	mutex m_notifiersmtx;
	condition_variable m_notifierscvar;
	lua_Integer m_topnotifierid;

	queue<string> m_codequeue;
	mutex m_codemtx;
	condition_variable m_codecvar;

	bool m_attached;
	bool m_killed;

	unsigned int m_id;

	static unsigned int count;
	static map<unsigned int, GThread*> detached;
	static mutex detachedmtx;

public:

	//Public functions
	virtual ~GThread();

	static void Setup( lua_State* state );

	lua_Integer Wait( lua_State* state, const lua_Integer* refs, size_t n );
	void WakeUp();

	lua_Integer SetupNotifier( Notifier* notifier );
	void RemoveNotifier( lua_Integer id );

	static int PushGThread( lua_State*, GThread* );
	static int Create( lua_State* );
	static int GetDetached( lua_State* );

	//Lua methods
	static int _gc( lua_State* );
	static int Run( lua_State* );
	static int OpenChannel( lua_State* );
	static int AttachChannel( lua_State* );
	static int Kill( lua_State* );
};

typedef struct GThreadHandle {
	GThread* thread;
} GThreadHandle;

int luaopen_engine( lua_State *L, GThread* thread );