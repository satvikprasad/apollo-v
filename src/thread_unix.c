#include "arena.h"
#include "thread.h"

#include <pthread.h>

typedef struct Thread {
    pthread_t thread;
} Thread;

Thread *
ThreadAlloc(MemoryArena *arena) {
    return ArenaPushStruct(arena, Thread);
}

void
ThreadCreate(Thread *thread, void *(*thread_func)(void *), void *data) {
    pthread_create(&thread->thread, NULL, thread_func, data);
}

void
ThreadJoin(Thread *thread) {
    pthread_join(thread->thread, NULL);
}
