#pragma once

#include <set>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>

#include "lua_headers.h"
#include "def.h"
#include "Timing.hpp"

class Notifier;
class GThreadChannel;

typedef struct DoubleChannel {
	class GThreadChannel* outgoing;
	class GThreadChannel* incoming;
} DoubleChannel;

typedef struct NotifierInstance {
	Notifier* notifier;
	void* data;
} NotifierInstance;

class GThread {

private:

	//Private functions
	GThread();

	void DetachLua();
	void ReattachLua( bool update = true );
	void Run( std::string code );

	void Terminate();

	static void ThreadMain( GThread* );

private:

	std::thread* m_thread;
	std::map<std::string, DoubleChannel> m_channels;

	Timing m_timing;
	lua_Integer m_timingid;

	std::map<lua_Integer, NotifierInstance> m_notifiers;
	std::mutex m_notifiersmtx;
	std::condition_variable m_notifierscvar;
	lua_Integer m_topnotifierid;

	std::queue<std::string> m_codequeue;
	std::mutex m_codemtx;
	std::condition_variable m_codecvar;

	bool m_attached{true};
	bool m_killed{false};

	unsigned int m_id;

	static unsigned int count;
	static std::map<unsigned int, GThread*> detached;
	static std::mutex detachedmtx;

public:

	//Public functions
	virtual ~GThread();

	static void Setup( lua_State* state );

	lua_Integer Wait( lua_State* state, std::set<lua_Integer> refs );
	void WakeUp();

	lua_Integer SetupNotifier( Notifier* notifier, void* data );
	void RemoveNotifier( lua_Integer id, bool nolock = false );

	lua_Integer CreateTimer( std::chrono::system_clock::time_point );

	DoubleChannel OpenChannels( std::string name );

	static int PushGThread( lua_State*, GThread* );
	static int Create( lua_State* );
	static int GetDetached( lua_State* );

	//Lua methods
	static int _gc( lua_State* );
	static int Run( lua_State* );
	static int OpenChannel( lua_State* );
	static int AttachChannel( lua_State* );
	static int Kill( lua_State* );
	static int Terminate( lua_State* );
};

typedef struct GThreadHandle {
	GThread* object;
} GThreadHandle;

int luaopen_engine( lua_State *L, GThread* thread );