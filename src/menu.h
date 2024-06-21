#pragma once

#include "arena.h"
#include <stdbool.h>

typedef struct MenuData MenuData;

MenuData *MenuCreate(MemoryArena *arena);
void MenuTogglePlayPause(MenuData *data);
