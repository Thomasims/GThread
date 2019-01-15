#pragma once

#include <map>
#include <chrono>

#include "lua_headers.h"
#include "Notifier.h"

class Timing: Notifier {
	std::multimap<std::chrono::system_clock::time_point, lua_Integer> times;
public:
	bool ShouldResume( std::chrono::system_clock::time_point* until, void* data ) override;

	int PushReturnValues( lua_State* state, void* data ) override;

	void CreatePoint( std::chrono::system_clock::time_point when, lua_Integer id );
};
