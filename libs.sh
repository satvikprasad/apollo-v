cd lib/raylib/src

make 

cd ../../../

cd lib
curl -L -R -O https://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz
cd lua-5.4.6
make all
cd ../../
rm lib/lua-5.4.6.tar.gz 
