#pragma once

#include "lua_headers.h"
#include "GThread.h"
#include <chrono>

class Notifier {
public:
	virtual bool ShouldResume( std::chrono::system_clock::time_point* until, void* data ) = 0;
	virtual int PushReturnValues( lua_State* state, void* data ) = 0;
};