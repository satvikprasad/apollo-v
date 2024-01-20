include="-Ilib/raylib/src -Ilib/lua-5.4.6/src"
linker="-lraylib -llua -L./lib/raylib/src/ -L./lib/lua-5.4.6/src -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
src="src/lmath.c src/hashmap.c src/main.c src/state.c src/ffmpeg_unix.c src/signals.c src/renderer.c src/parameter.c src/api.c src/arena.c src/permanent_storage.c"

mkdir -p build

clang $src -Wall -Wextra -g  -o build/lynx $include $linker -L./build/
mkdir -p build/assets
mkdir -p build/lua

cp -r ./assets/* build/assets/
cp -r ./lua/* build/lua/
