#!/bin/bash
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

NAME="Apollo"

SRC=$(find src/*.c | tr '\n' ' ' | sed -r 's/ (src\/[^\s]*_win32.c)//g')
CFLAGS="-Ilib/raylib/src -Ilib/lua-5.4.6/src -Ilib/miniaudio/ -Ilib/jsmn -Ilib/curl-8.5.0/include -Ilib/portaudio/include/"
LDFLAGS="-lraylib -llua -L./lib/raylib/src/ -L./lib/lua-5.4.6/src -lportaudio -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lcurl -L./build/"

echo -e ${GREEN}Found source files: $SRC${NC}

echo -e ${BLUE}Removing redundant directories...${NC}
rm -rf ${NAME}.app/
rm -rf /Applications/${NAME}.app
rm -rf *.dmg
if [ -d "/Volumes/${NAME}" ]; then diskutil unmount /Volumes/${NAME}; fi

echo -e ${BLUE}Creating directories...${NC}
mkdir -p ${NAME}.app/
mkdir -p ${NAME}.app/Contents/
mkdir -p ${NAME}.app/Contents/MacOS/
mkdir -p ${NAME}.app/Contents/Resources/

echo -e ${BLUE}Building for macOS...${NC}
clang ${SRC} -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-pointer-type-mismatch -D PLAT_MACOS -O3 -o ./${NAME}.app/Contents/MacOS/${NAME} ${CFLAGS} ${LDFLAGS}
echo "clang ${SRC} -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-pointer-type-mismatch -D PLAT_MACOS -O3 -o ./${NAME}.app/Contents/MacOS/${NAME} ${CFLAGS} ${LDFLAGS}"
mkdir -p ${NAME}.app/Contents/Frameworks/

echo -e ${BLUE}Creating icon${NC}
./macos/createicn.sh ./assets/logo.png ./${NAME}.app/Contents/Resources/icon.iconset ./${NAME}.app/Contents/Resources/${NAME}.icns

echo -e ${BLUE}Copying libraries...${NC}
cp -r /usr/local/lib/libcurl.4.dylib ${NAME}.app/Contents/Frameworks/
cp -r /usr/local/lib/libportaudio.2.dylib ${NAME}.app/Contents/Frameworks/
cp -r /usr/local/lib/libportaudio.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/libnghttp2/lib/libnghttp2.14.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/libidn2/lib/libidn2.0.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/libunistring/lib/libunistring.5.dylib ${NAME}.app/Contents/Frameworks/
cp -r /opt/homebrew/opt/gettext/lib/libintl.8.dylib ${NAME}.app/Contents/Frameworks/
cp -r $(which ffmpeg) ${NAME}.app/Contents/Frameworks/
cp -r build/assets ${NAME}.app/Contents/Resources/
cp -r build/lua ${NAME}.app/Contents/Resources/
cat macos/Info.plist | sed "s/\${NAME}/${NAME}/g" > ${NAME}.app/Contents/Info.plist

echo -e ${BLUE}Fixing library paths...${NC}
install_name_tool -change /usr/local/lib/libportaudio.2.dylib @executable_path/../Frameworks/libportaudio.2.dylib ${NAME}.app/Contents/MacOS/${NAME}
install_name_tool -change /usr/local/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ${NAME}.app/Contents/MacOS/${NAME}

echo -e ${BLUE}Signing libcurl...${NC}
install_name_tool -change /opt/homebrew/opt/libnghttp2/lib/libnghttp2.14.dylib @executable_path/../Frameworks/libnghttp2.14.dylib ${NAME}.app/Contents/Frameworks/libcurl.4.dylib
install_name_tool -change /opt/homebrew/opt/libidn2/lib/libidn2.0.dylib @executable_path/../Frameworks/libidn2.0.dylib ${NAME}.app/Contents/Frameworks/libcurl.4.dylib
install_name_tool -change /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib @executable_path/../Frameworks/libssl.3.dylib ${NAME}.app/Contents/Frameworks/libcurl.4.dylib
install_name_tool -change /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib @executable_path/../Frameworks/libcrypto.3.dylib ${NAME}.app/Contents/Frameworks/libcurl.4.dylib

echo -e ${BLUE}Signing libidn2...${NC}
install_name_tool -change /opt/homebrew/opt/libunistring/lib/libunistring.5.dylib @executable_path/../Frameworks/libunistring.5.dylib ${NAME}.app/Contents/Frameworks/libidn2.0.dylib	
install_name_tool -change /opt/homebrew/opt/gettext/lib/libintl.8.dylib @executable_path/../Frameworks/libintl.8.dylib ${NAME}.app/Contents/Frameworks/libidn2.0.dylib

echo -e ${BLUE}Signing libssl...${NC}
install_name_tool -change /opt/homebrew/Cellar/openssl@3/3.2.1/lib/libcrypto.3.dylib @executable_path/../Frameworks/libcrypto.3.dylib ${NAME}.app/Contents/Frameworks/libssl.3.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/Frameworks/libcurl.4.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/Frameworks/libportaudio.2.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/Frameworks/libportaudio.dylib
codesign --remove-signature ./${NAME}.app/Contents/Frameworks/libidn2.0.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/Frameworks/libidn2.0.dylib
codesign --remove-signature ./${NAME}.app/Contents/Frameworks/libssl.3.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/Frameworks/libssl.3.dylib
codesign -s vizzy-codesign-cert ./${NAME}.app/Contents/MacOS/${NAME}

echo -e ${BLUE}Creating ${NAME}.dmg...${NC}
hdiutil create -size 64m -fs HFS+ -volname "${NAME}" ${NAME}RW.dmg
hdiutil attach ${NAME}RW.dmg
cp -r ./${NAME}.app /Volumes/${NAME}/
ln -s /Applications /Volumes/${NAME}/
hdiutil detach /Volumes/${NAME}
hdiutil convert ${NAME}RW.dmg -format UDZO -o ${NAME}.dmg
rm ${NAME}RW.dmg
rm -rf ./${NAME}.app

echo -e ${GREEN}Done.${NC}
