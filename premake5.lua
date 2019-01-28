workspace "gmsv_gthread"
	configurations { "Debug", "Release" }
	location ( "projects/" .. os.target() )

project "gmsv_gthread"
	kind         "SharedLib"
	architecture "x86"
	language     "C++"
	includedirs  "include/"
	libdirs      "libs/"
	targetdir    "build"

	if os.istarget( "windows" ) then targetsuffix "_win32" end
	if os.istarget( "macosx" )  then targetsuffix "_osx"   end
	if os.istarget( "linux" )   then targetsuffix "_linux" end

	if _ACTION == "gmake2" then
		-- I have no idea if any of this applies to mac, it's mainly for linux
		linkoptions "-l:garrysmod/bin/lua_shared.so"
		postbuildcommands "{MOVE} %{cfg.targetdir}/lib%{prj.name}%{cfg.targetsuffix}.so %{cfg.targetdir}/%{prj.name}%{cfg.targetsuffix}.dll"
	else
		links "lua_shared"
	end

	files
	{
		"src/**.*",
		"include/**.*"
	}

	configuration "Debug"
		optimize "Debug"

	configuration "Release"
		optimize "Speed"
		staticruntime "On"