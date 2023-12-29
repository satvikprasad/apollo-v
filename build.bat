@echo off

SET include=-Ilib\raylib\src
SET linker=-lraylib -lgdi32 -lole32 -loleaut32 -limm32 -lwinmm

mkdir build

gcc src\state.c -o .\build\libstate.so -fPIC -shared %include% %linker%

gcc src\main.c -Wall -Wextra -o build\lynx.exe %include% %linker% -L.\build\

mkdir -p build\assets

xcopy .\assets .\build\assets /E

