#include "defines.h"
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
    return TextFormat("/Applications/Apollo.app/Contents/Resources/assets/%s",
                      path);
#else
    return TextFormat("assets/%s", path);
#endif
}

const char *
FSFormatLuaDirectory(const char *path) {
#if defined(PLAT_MACOS)
    return TextFormat("/Applications/Apollo.app/Contents/Resources/lua/%s",
                      path);
#else
    return TextFormat("lua/%s", path);
#endif
}

const char *
FSFormatDataDirectory(const char *path) {
    char home[512];
    FSGetHomeDirectory(home);

    return TextFormat("%s/.config/apollo/%s", home, path);
}

void
FSGetConfigDirectory(char *path) {
    char home[512];
    FSGetHomeDirectory(home);

    strcpy(path, TextFormat("%s/.config", home));
}

void
FSGetApolloDirectory(char *path) {
    char home[512];
    FSGetHomeDirectory(home);

    strcpy(path, TextFormat("%s/.config/apollo", home));
}

B8
FSCanRunCMD(const char *cmd) {
    if (strchr(cmd, '/')) {
        // if cmd includes a slash, no path search must be performed,
        // go straight to checking if it's executable
        return access(cmd, X_OK) == 0;
    }
    const char *path = getenv("PATH");
    if (!path)
        return false; // something is horribly wrong...
    // we are sure we won't need a buffer any longer
    char *buf = malloc(strlen(path) + strlen(cmd) + 3);
    if (!buf)
        return false; // actually useless, see comment
    // loop as long as we have stuff to examine in path
    for (; *path; ++path) {
        // start from the beginning of the buffer
        char *p = buf;
        // copy in buf the current path element
        for (; *path && *path != ':'; ++path, ++p) {
            *p = *path;
        }
        // empty path entries are treated like "."
        if (p == buf)
            *p++ = '.';
        // slash and command name
        if (p[-1] != '/')
            *p++ = '/';
        strcpy(p, cmd);
        // check if we can execute it
        if (access(buf, X_OK) == 0) {
            free(buf);
            return true;
        }
        // quit at last cycle
        if (!*path)
            break;
    }
    // not found
    free(buf);
    return false;
}
