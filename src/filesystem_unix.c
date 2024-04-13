#include "filesystem.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void
FSCreateDirectory(const char *path) {
    mkdir(path, 0777);
}

void
FSGetHomeDirectory(char *path) {
    char *home = getenv("HOME");

    strcpy(path, home);
}

const char *
FSFormatAssetsDirectory(const char *path) {
#if defined(PLAT_MACOS)
    return TextFormat("/Applications/Vizzy.app/Contents/Resources/assets/%s",
                      path);
#else
    return TextFormat("assets/%s", path);
#endif
}

const char *
FSFormatLuaDirectory(const char *path) {
#if defined(PLAT_MACOS)
    return TextFormat("/Applications/Vizzy.app/Contents/Resources/lua/%s",
                      path);
#else
    return TextFormat("lua/%s", path);
#endif
}

const char *
FSFormatDataDirectory(const char *path) {
    char home[512];
    FSGetHomeDirectory(home);

    return TextFormat("%s/.config/vizzy/%s", home, path);
}
