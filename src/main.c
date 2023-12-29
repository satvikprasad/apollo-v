#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <unistd.h>

#include <raylib.h>

#include "defines.h"

#define SUPPORT_FILEFORMAT_FLAC

#ifdef HOT_RELOADABLE
#define X(name, ret, ...) name##_t *state_##name;
#else
#define X(name, ret, ...) name##_t state_##name;
#endif
  STATE_METHODS
#undef X

void *libstate;

#ifdef HOT_RELOADABLE
#include <dlfcn.h>
static inline void *load_func(const char *name)  {
  void *func = dlsym(libstate, name);
  if (func == NULL) {
    fprintf(stderr, "ERROR: Could not find symbol %s: %s\n", name, dlerror());
  }
  return func;
}

static inline b8 load_symbols() {
  if (libstate != NULL) {
    printf("Reloading\n");
    dlclose(libstate);
  }

  libstate = dlopen("libstate.so", RTLD_NOW);
  if (libstate == NULL) {
    fprintf(stderr, "ERROR: Could not load %s: %s\n", "libstate.so", dlerror());
    return false;
  }

#define X(name, ret, ...) state_##name = load_func("state_" #name);
  STATE_METHODS
#undef X

  return true;
}
#else
#define load_symbols() true
#endif

int main(void) {
  if(!load_symbols()) return 1;

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 1080, "Visualizer");
  SetTargetFPS(144);
  InitAudioDevice();

  state_initialise("assets/monks.mp3");
  while(!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      void *state = state_detach();
      if(!load_symbols()) return 1;
      state_attach(state);
    }

    state_update();
  }

  state_destroy();

  return 0;
}

