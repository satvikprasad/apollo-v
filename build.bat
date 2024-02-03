@echo off

SET include=-Ilib\raylib\src -Ilib\lua-5.4.6\src -Ilib\miniaudio -Ilib\jsmn -Ilib\curl-8.5.0\include\
SET linker=lib\raylib\src\libraylib.a lib\curl-8.5.0\lib\libcurl.a lib\lua-5.4.6\src\liblua.a -lgdi32 -lole32 -loleaut32 -limm32 -lwinmm
SET src=src\lmath.c src\hashmap.c src\main.c src\state.c .\src\ffmpeg_win32.c src\signals.c src\renderer.c src\parameter.c src\api.c src\arena.c src\permanent_storage.c src\loopback.c src\server.c src\json.c .\src\thread_win32.c .\src\animation.c 
mkdir build

REM gcc src\state.c -o .\build\libstate.so -fPIC -shared %include% %linker%

gcc %src% -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-braces -g -o build\lynx.exe %include% %linker% -L.\build\

mkdir build\assets
mkdir build\lua

xcopy .\assets .\build\assets /E
xcopy .\lua .\build\lua /E
