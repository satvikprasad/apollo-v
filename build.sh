include="-Ilib/raylib/src"
linker="-lraylib -L./lib/raylib/src/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"

mkdir -p build

gcc src/lmath.c src/main.c src/state.c src/ffmpeg_unix.c src/signals.c src/renderer.c -Wall -Wextra -g  -o build/lynx $include $linker -L./build/
mkdir -p build/assets

cp -r ./assets/* build/assets/
