include=""
linker="-lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"

mkdir -p build

gcc src/state.c -o ./build/libstate.so -fPIC -shared $include $linker

gcc src/main.c -Wall -Wextra  -o build/lynx $include $linker -L./build/ -DHOT_RELOADABLE

mkdir -p build/assets

cp -r ./assets/* build/assets/
