#pragma once

#include <unordered_set>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "lua_headers.h"
#include "def.h"
#include "Notifier.h"

class GThreadPacket;

typedef struct GThreadChannelHandle {
	GThreadChannel* channel;
	GThread* parent;
	lua_Integer id;
} GThreadChannelHandle;

using namespace std;

class GThreadChannel: Notifier {

private:

	//Private functions

private:

	int m_references;
	bool m_closed;

	queue<GThreadPacket*> m_queue;
	mutex m_queuemtx;
	unordered_set<GThreadChannelHandle*> m_handles;
	mutex m_handlesmtx;

public:

	//Public functions
	GThreadChannel();
	virtual ~GThreadChannel();
	
	bool ShouldResume( chrono::system_clock::time_point* until ) override;
	int PushReturnValues( lua_State* state ) override;
	void QueuePacket( GThreadPacket* );

	void AddHandle( GThreadChannelHandle* handle );
	void RemoveHandle( GThreadChannelHandle* handle );

	bool CheckClosing();

	GThreadPacket* FinishPacket();

	static void Setup( lua_State* state );

	static int PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent = NULL );
	static int Create( lua_State* state );

	//Lua methods
	static int _gc( lua_State* state );
	
	static int PullPacket( lua_State* state );
	static int PushPacket( lua_State* state );
	static int GetHandle( lua_State* state );
	static int StartPacket( lua_State* state );
	static int Close( lua_State* state );
};