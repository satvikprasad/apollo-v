@echo off

SET include=-Ilib\raylib\src -Ilib\lua-5.4.6\src
SET linker=lib\raylib\src\libraylib.a lib\lua-5.4.6\src\liblua.a -lgdi32 -lole32 -loleaut32 -limm32 -lwinmm
SET src=src\lmath.c src\main.c src\state.c src\ffmpeg_windows.c src\signals.c src\renderer.c src\parameter.c src\hashmap.c src\loopback.c src\api.c src\arena.c src\permanent_storage.c

mkdir build

REM gcc src\state.c -o .\build\libstate.so -fPIC -shared %include% %linker%

gcc %src% -Wall -Wextra -o build\lynx.exe %include% %linker% -L.\build\

mkdir -p build\assets

xcopy .\assets .\build\assets /E

