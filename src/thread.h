#pragma once

#include "arena.h"
typedef struct Thread Thread;

Thread *ThreadAlloc(MemoryArena *arena);
void    ThreadCreate(Thread *thread, void *(*thread_func)(void *), void *data);
void    ThreadJoin(Thread *thread);
