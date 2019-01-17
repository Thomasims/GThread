#pragma once

#include <unordered_set>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "lua_headers.h"
#include "def.h"
#include "Notifier.h"

class GThreadPacket;
class GThreadChannel;
class GThread;

typedef struct GThreadChannelHandle {
	GThreadChannel* object;
	GThread* parent;
	lua_Integer id;

	GThreadPacket* in_packet;
	GThreadPacket* out_packet;
} GThreadChannelHandle;

class GThreadChannel : Notifier {
	friend class GThread;
private:

	//Private functions
	GThreadPacket* PopPacket();
	void QueuePacket( GThreadPacket* );

	bool CheckClosing();

	void DetachSibling();

private:

	int m_references{0};
	bool m_closed{false};

	GThreadChannel* m_sibling{NULL};

	std::queue<GThreadPacket*> m_queue;
	std::mutex m_queuemtx;
	std::unordered_set<GThreadChannelHandle*> m_handles;
	std::mutex m_handlesmtx;

public:

	//Public functions
	GThreadChannel();
	virtual ~GThreadChannel();

	bool ShouldResume( std::chrono::system_clock::time_point* until, void* data ) override;
	int PushReturnValues( lua_State* state, void* data ) override;

	void AddHandle( GThreadChannelHandle* handle );
	void RemoveHandle( GThreadChannelHandle* handle );

	void SetSibling( GThreadChannel* other );

	static void Setup( lua_State* state );

	static int PushGThreadChannel( lua_State* state, GThreadChannel* channel, GThread* parent = NULL );
	static int Create( lua_State* state );

	static GThreadChannel* Get( lua_State* state, int narg );
	static GThreadPacket* GetPacketIn( lua_State* state, int narg );
	static GThreadPacket* GetPacketOut( lua_State* state, int narg );

	//Lua methods
	static int _gc( lua_State* state );

	static int PullPacket( lua_State* state );
	static int PushPacket( lua_State* state );
	static int GetHandle( lua_State* state );
	static int StartPacket( lua_State* state );
	static int Close( lua_State* state );

	static int GetInPacket( lua_State* state );
	static int GetOutPacket( lua_State* state );
};