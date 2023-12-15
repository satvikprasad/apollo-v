cd lib/raylib

cmake -D BUILD_SHARED_LIBS=ON .

cd raylib

sudo make install

sudo install_name_tool -id '/usr/local/lib/libraylib.dylib' /usr/local/lib/libraylib.dylib

cd ../../../

