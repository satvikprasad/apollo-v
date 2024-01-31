#include "mutex.h"

void MutexCreate(Mutex *mutex) { pthread_mutex_init(&mutex->mutex, NULL); }

void MutexDestroy(Mutex *mutex) { pthread_mutex_destroy(&mutex->mutex); }

void MutexLock(Mutex *mutex) { pthread_mutex_lock(&mutex->mutex); }

void MutexUnlock(Mutex *mutex) { pthread_mutex_unlock(&mutex->mutex); }
