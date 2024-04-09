CC=clang

ifeq ($(OS), Windows_NT)
	CFLAGS=-Ilib\raylib\src -Ilib\lua-5.4.6\src -Ilib\miniaudio -Ilib\jsmn -Ilib\curl-8.5.0\include
	LDFLAGS=lib\raylib\src\libraylib.a lib\curl-8.5.0\lib\libcurl.a lib\lua-5.4.6\src\liblua.a 
	LDFLAGS+=-lgdi32 -lole32 -loleaut32 -limm32 -lwinmm -L.\build\
	SRC=$(patsubst %_unix.c, , $(wildcard src/*.c))
	OUT=build\lynx.exe
	OS=Windows
	COPY=xcopy /E
	MKDIR=mkdir
else 
	CFLAGS=-Ilib/raylib/src -Ilib/lua-5.4.6/src -Ilib/miniaudio/ -Ilib/jsmn -Ilib/curl-8.5.0/include -Ilib/portaudio/include/
	LDFLAGS=-lraylib -llua -L./lib/raylib/src/ -L./lib/lua-5.4.6/src -lportaudio
	LDFLAGS+=-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lcurl -L./build/
	SRC=$(patsubst %_win32.c, , $(wildcard src/*.c))
	OUT=build/lynx
	OS=Unix
	COPY=cp -r
	MKDIR=mkdir -p
endif

.PHONY: build

build: $(SRC)
	$(CC) $(SRC) -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-pointer-type-mismatch -g -o $(OUT) $(CFLAGS) $(LDFLAGS)

package: assets
	@rm -rf Vizzy.app/
	@rm -rf /Applications/Vizzy.app
	@rm -rf *.dmg
	@echo Cleaned up...
	@mkdir -p Vizzy.app/
	@mkdir -p Vizzy.app/Contents/
	@mkdir -p Vizzy.app/Contents/MacOS/
	@$(CC) $(SRC) -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-pointer-type-mismatch -D PLAT_MACOS -O3 -o ./Vizzy.app/Contents/MacOS/Vizzy $(CFLAGS) $(LDFLAGS)
	@mkdir -p Vizzy.app/Contents/Frameworks/
	@$(COPY) /usr/local/lib/libcurl.4.dylib Vizzy.app/Contents/Frameworks/
	@$(COPY) /usr/local/lib/libportaudio.2.dylib Vizzy.app/Contents/Frameworks/
	@$(COPY) /usr/local/lib/libportaudio.dylib Vizzy.app/Contents/Frameworks/
	@mkdir -p Vizzy.app/Contents/Resources/
	@$(COPY) build/assets Vizzy.app/Contents/Resources/
	@$(COPY) build/lua Vizzy.app/Contents/Resources/
	@$(COPY) Info.plist Vizzy.app/Contents/
	@echo Fixing library paths...
	@install_name_tool -change /usr/local/lib/libportaudio.2.dylib @executable_path/../Frameworks/libportaudio.2.dylib Vizzy.app/Contents/MacOS/Vizzy
	@install_name_tool -change /usr/local/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib Vizzy.app/Contents/MacOS/Vizzy
	@codesign -v -f -s "F0A4C88DBC7C2AD8949C59E4D18FE1BEFDFF4180" --entitlements entitlements.plist ./Vizzy.app
	@find ./Vizzy.app/Contents/ -type f \( -name "Vizzy" -or -name "*.dylib" \) -exec codesign --verbose -f -s "F0A4C88DBC7C2AD8949C59E4D18FE1BEFDFF4180" --entitlements entitlements.plist {} \;
	@hdiutil create -size 64m -fs HFS+ -volname "Vizzy" VizzyRW.dmg
	@hdiutil attach VizzyRW.dmg
	@cp -r ./Vizzy.app /Volumes/Vizzy/
	@ln -s /Applications /Volumes/Vizzy/
	@hdiutil detach /Volumes/Vizzy
	@hdiutil convert VizzyRW.dmg -format UDZO -o Vizzy.dmg
	@rm VizzyRW.dmg
	@rm -rf ./Vizzy.app
	@echo Done.

run: build assets
	@./build/lynx

clean: 
	@rm -rf build

libs: dirs tars
	@cd lib/raylib/src; make;
	@cd lib/lua-5.4.6; make all; 
	@cd lib/curl-8.5.0; ./configure --with-openssl --disable-shared; make; make install;
	# @cd lib/portaudio; ./configure; make; make install;
	@$(COPY) /usr/local/lib/libcurl.dylib ./build/
	@$(COPY) /usr/local/lib/libportaudio.2.dylib ./build/
	@$(COPY) /usr/local/lib/libportaudio.dylib ./build/

install:
	@$(COPY) ./build/libcurl.dylib /usr/local/lib/
	@$(COPY) ./build/libportaudio.2.dylib /usr/local/lib/
	@$(COPY) ./build/libportaudio.dylib /usr/local/lib/

dirs:
	@$(MKDIR) build
	@$(MKDIR) build/assets
	@$(MKDIR) build/lua

assets: dirs
	@$(COPY) ./assets/* build/assets/
	@$(COPY) ./lua/* build/lua

tars:
	@echo Fetching lua...
	@cd lib; curl -s -L -R -O https://www.lua.org/ftp/lua-5.4.6.tar.gz; tar zxf lua-5.4.6.tar.gz; rm lua-5.4.6.tar.gz
	@echo Fetching libcurl...
	@cd lib; curl -s -L -R -O https://curl.se/download/curl-8.5.0.tar.gz; tar zxf curl-8.5.0.tar.gz; rm curl-8.5.0.tar.gz
	@echo Fetching portaudio...
	@cd lib; curl -s -L -R -O https://files.portaudio.com/archives/pa_stable_v190600_20161030.tgz; tar zxf pa_stable_v190600_20161030.tgz; rm pa_stable_v190600_20161030.tgz
