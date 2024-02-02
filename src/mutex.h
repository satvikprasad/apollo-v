#pragma once

#include <pthread.h>

typedef struct Mutex {
    pthread_mutex_t mutex;
} Mutex;

void MutexCreate(Mutex *mutex);
void MutexDestroy(Mutex *mutex);
void MutexLock(Mutex *mutex);
void MutexUnlock(Mutex *mutex);
