#include "state.h"
#include <dlfcn.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <unistd.h>

#include <raylib.h>

#include "defines.h"

#define SUPPORT_FILEFORMAT_FLAC

void *libstate = NULL;
StateDetachT StateDetach = NULL;
StateAttachT StateAttach = NULL;
StateUpdateT StateUpdate = NULL;
StateInitialiseT StateInitialise = NULL;

State state = {0};

static inline b8 LoadSymbols();

int main(void) {
	if(!LoadSymbols()) return 1;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Visualizer");
	SetTargetFPS(144);
	InitAudioDevice();

	StateInitialise(&state, "assets/maajin bae.mp3");
	while(!WindowShouldClose()) {
		if (IsKeyPressed(KEY_R)) {
			StateDetach(&state);
			if(!LoadSymbols()) return 1;

			StateAttach(&state);
		}

		StateUpdate(&state);
	}

	return 0;
}

static inline b8 LoadSymbols() {
	if (libstate != NULL) {
		printf("Reloading\n");
		dlclose(libstate);
	}
	
	libstate = dlopen("libstate.so", RTLD_NOW);
	if (libstate == NULL) {
		fprintf(stderr, "ERROR: Could not load %s: %s\n", "libstate.so", dlerror());
		return false;
	}
	
	StateDetach = dlsym(libstate, "StateDetach");
	if (StateUpdate == NULL) {
		fprintf(stderr, "ERROR: Could not find symbol StateDetach: %s\n", dlerror());
	}
	
	StateAttach = dlsym(libstate, "StateAttach");
	if (StateUpdate == NULL) {
		fprintf(stderr, "ERROR: Could not find symbol StateAttach: %s\n", dlerror());
	}

	StateUpdate = dlsym(libstate, "StateUpdate");
	if (StateUpdate == NULL) {
		fprintf(stderr, "ERROR: Could not find symbol StateUpdate: %s\n", dlerror());
	}

	StateInitialise = dlsym(libstate, "StateInitialise");
	if (StateInitialise == NULL) {
		fprintf(stderr, "ERROR: Could not find symbol StateInitialise: %s\n", dlerror());
	}

	return true;
}
