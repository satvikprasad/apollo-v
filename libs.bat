@echo off

git submodule update --init --recursive

REM Raylib
cd lib\raylib\src

make 

cd ..\..\..\

REM Lua
cd lib

curl -L -R -O https://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz

cd lua-5.4.6

make PLAT=mingw all

cd ..\..\

del lib\lua-5.4.6.tar.gz 

REM LibCURL
cd lib

curl -L -R -O https://curl.se/download/curl-8.5.0.tar.gz
tar zxf curl-8.5.0.tar.gz

del curl-8.5.0.tar.gz

cd curl-8.5.0\builds\libcurl-vc-x64-release-dll-ipv6-sspi-schannel\bin\

gendef libcurl.dll
dlltool --as-flags=--64 -m i386:x86-64 -k --output-lib libcurl.a --input-def libcurl.def

cd ..\..\..\..\..\

copy .\lib\curl-8.5.0\builds\libcurl-vc-x64-release-dll-ipv6-sspi-schannel\bin\libcurl.a .\lib\curl-8.5.0\lib\
