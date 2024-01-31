@echo off

SET include=-Ilib\raylib\src -Ilib\lua-5.4.6\src -Ilib\miniaudio -Ilib\jsmn -Ilib\curl-8.5.0\include
SET linker=lib\raylib\src\libraylib.a lib\lua-5.4.6\src\liblua.a -lgdi32 -lole32 -loleaut32 -limm32 -lwinmm -lcurl
SET src=src\lmath.c src\hashmap.c src\main.c src\state.c src\ffmpeg_unix.c src\signals.c src\renderer.c src\parameter.c src\api.c src\arena.c src\permanent_storage.c src\loopback.c src\server.c src\json.c
mkdir build

REM gcc src\state.c -o .\build\libstate.so -fPIC -shared %include% %linker%

gcc %src% -Wall -Wextra -o build\lynx.exe %include% %linker% -L.\build\

mkdir -p build\assets
mkdir -p build\lua

xcopy .\assets .\build\assets /E
xcopy .\lua .\build\lua /E
