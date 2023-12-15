include=""
linker="-L/usr/local/lib -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"

mkdir -p build

gcc src/state.c -o ./build/libstate.so -fPIC -shared $include $linker

sudo cp ./build/libstate.so /usr/local/lib/

gcc src/main.c -Wall -Wextra  -o build/visualizer $include $linker -L./build/

mkdir -p build/assets

