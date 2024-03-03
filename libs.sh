git submodule update --init --recursive

# Raylib
cd lib/raylib/src

make 

cd ../../../

# Lua
cd lib

curl -L -R -O https://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz

cd lua-5.4.6

make all

cd ../../

rm lib/lua-5.4.6.tar.gz 

# LibCURL
cd lib

curl -L -R -O https://curl.se/download/curl-8.5.0.tar.gz
tar zxf curl-8.5.0.tar.gz

rm curl-8.5.0.tar.gz

cd curl-8.5.0

./configure --with-openssl --disable-shared
make
make install

cd ../../

cp /usr/local/lib/libcurl.dylib ./build/
