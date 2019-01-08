#pragma once

#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "lua_headers.h"
#include "def.h"
#include "GThreadChannel.h"

using namespace std;

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
	virtual ~GThread();

	static void Setup( lua_State* state );

	lua_Integer Wait( const lua_Integer* refs, size_t n );
	void WakeUp( const char* channel );

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