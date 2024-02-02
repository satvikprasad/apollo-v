// TODO(satvik): Add license header
// Serialize and deserialize parameters in a neat file format.

#include <raylib.h>

#include "defines.h"
#include "state.h"

#define SUPPORT_FILEFORMAT_FLAC

void *libstate;

I32 main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 1080, "Lynx");

    SetExitKey(KEY_NULL);

    SetTargetFPS(144);
    InitAudioDevice();

    StateInitialise();
    while (!StateShouldClose()) {
        StateUpdate();
        StateRender();
    }

    StateDestroy();

    return 0;
}
