#pragma once

#include <map>
#include <chrono>

#include "lua_headers.h"
#include "Notifier.h"

class Timing: Notifier {
	std::multimap<std::chrono::system_clock::time_point, lua_Integer> times;
public:
	bool ShouldResume( std::chrono::system_clock::time_point* until, void* data ) override {
		auto& b = begin( times );
		if( b == end( times ) )
			return false;
		if( b->first > std::chrono::system_clock::now() )
			*until = b->first;
		return true;
	};

	int PushReturnValues( lua_State* state, void* data ) override {
		auto& b = begin( times );
		lua_pop( state, 1 );
		lua_pushinteger( state, b->second );
		times.erase( b );
		return 0;
	};

	void CreatePoint( std::chrono::system_clock::time_point when, lua_Integer id ) {
		times.emplace( when, id );
	}
};
