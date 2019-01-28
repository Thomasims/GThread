# GThread
Multi-threading module for Garry's Mod servers.

This module allows you to run code in separate threads and communicate with them using data channels.

## Installation

The installation is fairly straight forward:
1. Place the module (.dll) in `garrysmod/lua/bin/` (create the folder bin if it doesn't exist)
2. Copy `lua/includes/modules/threading.lua` into the same folder in `garrysmod/`

There is no extra dependency so this should be all you need. Make sure to use the right DLL for your OS.

## Example

```lua
require "threading"

local thread = threading.newThread()

local datachannel = thread:OpenChannel( "data" )

datachannel:Receive( function( bytes, tag )
	print( "result: " .. datachannel:ReadDouble() )
end )

thread:Run( function() -- You can either pass a string or a function with no upvalues
	local datachannel = engine:OpenChannel( "data" )

	while true do
		local bytes, tag = datachannel:Wait()
		if not bytes then return end

		local a = datachannel:ReadDouble()
		local b = datachannel:ReadDouble()

		engine:Sleep( 1000 ) -- Do some expensive work, this is entirely separate

		datachannel:StartPacket()
		datachannel:WriteDouble( a + b )
		datachannel:PushPacket()
	end
end )

datachannel:StartPacket()
datachannel:WriteDouble( 45 )
datachannel:WriteDouble( 81 )
datachannel:PushPacket()

local packet = threading.newPacket()
packet:WriteDouble( 11 )
packet:WriteDouble( 54 )
datachannel:PushPacket( packet )
datachannel:PushPacket( packet )
datachannel:PushPacket( packet )
```

## Building

If you wish to build the module yourself, the build files are included in this repo.

### _Windows_

To build for Windows you'll need Visual Studio and [premake5.exe](https://premake.github.io/download.html)
1. Drop `premake5.exe` next to `BuildProjects.bat`
2. Run `BuildProjects.bat`
3. Open the VS solution from `projects/windows/` and build
4. Output is `build/gmsv_gthread_win32.dll`

### _Linux_

1. Get `premake5` and place it next to `BuildProjects.bat`
2. Run `premake5 --os=linux gmake2`
3. CD into `projects/linux/`
4. `make` optionally with `config=(debug|release)`
5. Output is `build/gmsv_gthread_linux.dll` (the makefile renames the created .so to the GMod module format)