// TODO(satvik): Add license header
// Serialize and deserialize parameters in a neat file format.

#include <raylib.h>
#include <stdio.h>

#include "defines.h"
#include "state.h"

#define SUPPORT_FILEFORMAT_FLAC

void *libstate;

I32 main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 1080, "Lynx");

    SetTargetFPS(144);
    InitAudioDevice();

    StateInitialise();
    while (!WindowShouldClose()) {
        StateUpdate();
        StateRender();
    }

    StateDestroy();

    return 0;
}
