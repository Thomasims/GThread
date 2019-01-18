#pragma once

#include <set>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <memory>

#include "lua_headers.h"
#include "def.h"
#include "Timing.h"
#include "Notifier.h"
#include "Buffer.h"



class GThread;
class GThreadChannel;
class GThreadPacket;



typedef struct DoubleChannel {
	class GThreadChannel* outgoing;
	class GThreadChannel* incoming;
} DoubleChannel;

typedef struct NotifierInstance {
	Notifier* notifier;
	void* data;
} NotifierInstance;

int luaopen_engine( lua_State *L, GThread* thread );



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

	std::thread* m_thread{nullptr};
	std::map<std::string, DoubleChannel> m_channels;

	Timing m_timing;
	lua_Integer m_timingid;

	std::map<lua_Integer, NotifierInstance> m_notifiers;
	std::mutex m_notifiersmtx;
	std::condition_variable m_notifierscvar;
	lua_Integer m_topnotifierid{0};

	std::queue<std::string> m_codequeue;
	std::mutex m_codemtx;
	std::condition_variable m_codecvar;

	bool m_attached{true};
	bool m_killed{false};

	unsigned int m_id{0};

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

	static int stringwriter( lua_State* state, const void* chunk, size_t len, void* data ) {
		if ( !data ) return 1;
		static_cast<std::string*>(data)->append( (const char*) chunk, len );
		return 0;
	}

	//Lua methods
	static int _gc( lua_State* );
	static int Run( lua_State* );
	static int OpenChannel( lua_State* );
	static int AttachChannel( lua_State* );
	static int Kill( lua_State* );
	static int Terminate( lua_State* );
};



class GThreadChannel : public Notifier {

private:

	//Private functions
	GThreadPacket* PopPacket();
	void QueuePacket( GThreadPacket* );

	void DetachSibling();

private:

	int m_references{0};
	bool m_closed{false};

	GThreadChannel* m_sibling{nullptr};

	std::queue<GThreadPacket*> m_queue;
	std::mutex m_queuemtx;
	std::unordered_set<GThread*> m_threads;
	std::mutex m_threadsmtx;

public:

	//Public functions
	GThreadChannel();
	virtual ~GThreadChannel();

	void Ref() { ++m_references; };
	void UnRef() { if(!--m_references) delete this; };

	bool ShouldResume( std::chrono::system_clock::time_point* until, void* data ) override;
	int PushReturnValues( lua_State* state, void* data ) override;

	void AddThread( GThread* handle );
	void RemoveThread( GThread* handle );

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



class GThreadPacket {

private:

	//Private functions

	template<class T>
	size_t Write( T data );
	size_t WriteData( const char* data, size_t len );

	template<class T>
	T Read();
	std::string ReadData( size_t len );

	size_t Seek( Head head, Location loc, ptrdiff_t bytes );
	size_t Slice( size_t start, size_t end );

private:

	int m_references{ 0 };
	Buffer m_buffer;

public:

	//Public functions
	GThreadPacket() = default;
	GThreadPacket( const GThreadPacket& );

	void Ref() { ++m_references; };
	void UnRef() { if(!--m_references) delete this; };

	void Clear();
	int GetBytes();

	static void Setup( lua_State* state );

	static int PushGThreadPacket( lua_State* state, GThreadPacket* packet );
	static int Create( lua_State* state );

	static GThreadPacket* Get( lua_State* state, int narg );

	//Lua methods
	static int _gc( lua_State* state );

	using PacketGetter = GThreadPacket* (*)(lua_State*, int);

	template<PacketGetter, class T> static int WriteNumber( lua_State* );
	template<PacketGetter> static int WriteData( lua_State* );
	template<PacketGetter> static int WriteString( lua_State* );
	template<PacketGetter> static int WriteFunction( lua_State* );
	
	template<PacketGetter, class T> static int ReadNumber( lua_State* );
	template<PacketGetter> static int ReadData( lua_State* );
	template<PacketGetter> static int ReadString( lua_State* );
	template<PacketGetter> static int ReadFunction( lua_State* );

	static int Seek( lua_State* );
	static int GetSize( lua_State* );
	static int Slice( lua_State* );
};


template<GThreadPacket::PacketGetter GetP, class T>
int GThreadPacket::WriteNumber( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	lua_pushinteger( state, packet->Write<T>( static_cast<T>( luaL_checknumber( state, 2 ) ) ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::WriteData( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );
	auto asked_len = size_t( luaL_checknumber( state, 3 ) );

	lua_pushinteger( state, packet->WriteData( data, asked_len > len ? len : asked_len ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::WriteString( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len;
	const char* data = luaL_checklstring( state, 2, &len );

	lua_pushinteger( state, packet->Write( len ) + packet->WriteData( data, len ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::WriteFunction( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	if ( !lua_isfunction( state, 2 ) ) return 0;
	if ( lua_getupvalue( state, 2, 1 ) ) return luaL_error( state, "Function must not have upvalues" );

	std::string code;
	lua_pushvalue( state, 2 );
	lua_dump( state, GThread::stringwriter, &code );
	lua_pop( state, 1 );

	lua_pushinteger( state, packet->Write( code.length() ) + packet->WriteData( code.data(), code.length() ) );
	return 1;
}


template<GThreadPacket::PacketGetter GetP, class T>
int GThreadPacket::ReadNumber( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;
	lua_pushnumber( state, lua_Number( packet->Read<T>() ) );
	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::ReadData( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	std::string data = packet->ReadData( size_t( luaL_checknumber( state, 2 ) ) );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::ReadString( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;

	size_t len = packet->Read<size_t>();

	std::string data = packet->ReadData( len );
	lua_pushlstring( state, data.data(), data.length() );

	return 1;
}

template<GThreadPacket::PacketGetter GetP>
int GThreadPacket::ReadFunction( lua_State* state ) {
	GThreadPacket* packet = GetP( state, 1 );
	if ( !packet ) return 0;
	
	size_t len = packet->Read<size_t>();

	std::string code = packet->ReadData( len );
	luaL_loadbuffer( state, code.data(), code.length(), "TFunc" );
	lua_pushlstring( state, code.data(), code.length() );

	return 2;
}



typedef struct GThreadHandle {
	GThreadHandle( GThread* thread )
		: object{ thread } {
	}
	GThread* object{ nullptr };
} GThreadHandle;

typedef struct GThreadChannelHandle {
	GThreadChannelHandle( GThreadChannel* channel, GThread* parent )
		: object{ channel }
		, parent{ parent } {
		channel->Ref();
		if ( parent ) {
			id = parent->SetupNotifier( channel, (void*) this );
			channel->AddThread( parent );
		}
	};
	~GThreadChannelHandle() {
		if(parent) {
			parent->RemoveNotifier( id );
			object->RemoveThread( parent );
		}
		if ( in_packet )
			delete in_packet;
		if ( out_packet )
			delete out_packet;
		if( object )
			object->UnRef();
		object = nullptr;
		parent = nullptr;
	};

	GThreadChannel* object{ nullptr };
	GThread* parent{ nullptr };
	lua_Integer id{ 0 };

	GThreadPacket* in_packet{ nullptr };
	GThreadPacket* out_packet{ nullptr };
} GThreadChannelHandle;

typedef struct GThreadPacketHandle {
	GThreadPacketHandle( GThreadPacket* packet)
		:object{ packet } {
		packet->Ref();
	};
	~GThreadPacketHandle() {
		if( object )
			object->UnRef();
		object = nullptr;
	};

	GThreadPacket* object{ nullptr };
} GThreadPacketHandle;