#include "Timing.hpp"
#include "GThread.h"

using namespace std;
using namespace std::chrono;

bool Timing::ShouldResume( system_clock::time_point* until, void* data ) {
	auto& b = begin( times );
	if( b == end( times ) )
		return false;
	system_clock::time_point now = system_clock::now();
	if( b->first > now && *until > b->first )
		*until = b->first;
	if( b->first <= now )
		return true;
	return false;
};

int Timing::PushReturnValues( lua_State* state, void* data ) {
	auto& b = begin( times );

	lua_pop( state, 1 );
	lua_pushinteger( state, b->second );

	GThread* thread = static_cast<GThread*>(data);
	thread->RemoveNotifier( b->second, true );
	times.erase( b );
	return 0;
};

void Timing::CreatePoint( system_clock::time_point when, lua_Integer id ) {
	times.emplace( when, id );
}