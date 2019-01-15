#include "Timing.hpp"
#include "GThread.h"

using namespace std;
using namespace std::chrono;

bool Timing::ShouldResume( system_clock::time_point* until, void* data ) {
	auto& b = begin( times );
	if( b == end( times ) )
		return false;
	if( b->first > system_clock::now() && *until > b->first )
		*until = b->first;
	return true;
};

int Timing::PushReturnValues( lua_State* state, void* data ) {
	auto& b = begin( times );

	lua_pop( state, 1 );
	lua_pushinteger( state, b->second );

	GThread* thread = static_cast<GThread*>(data);
	thread->RemoveNotifier( b->second );
	times.erase( b );
	return 0;
};

void Timing::CreatePoint( system_clock::time_point when, lua_Integer id ) {
	times.emplace( when, id );
}