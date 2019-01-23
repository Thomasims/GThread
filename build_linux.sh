gcc -m32 -std=c++14 -shared -o build/gmsv_gthread_linux.so src/*.cpp -Iinclude/ -Llibs/ -l:garrysmod/bin/lua_shared.so
