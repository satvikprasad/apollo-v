#pragma once

#include "defines.h"
#include "frame.h"
#include "raylib.h"

#include <complex.h>

typedef struct State {
	Music music;
} State;

typedef void (*StateInitialiseT)(State *state, const char *fp);
typedef void (*StateUpdateT)(State *state);
typedef void (*StateDetachT)(State *state);
typedef void (*StateAttachT)(State *state);
