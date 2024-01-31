#include "thread.h"

#include <windows.h>

typedef struct Thread {
    HANDLE handle;
} Thread;

Thread *ThreadAlloc(MemoryArena *arena) {
    return ArenaPushStruct(arena, Thread);
}

void ThreadCreate(Thread *thread, void *(*thread_func)(void *), void *data) {
    thread->handle = CreateThread(NULL, 0, thread_func, data, 0, NULL);
}

void ThreadJoin(Thread *thread) {
    WaitForSingleObject(thread->handle, INFINITE);
}
