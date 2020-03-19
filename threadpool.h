#ifndef THREADPOOL_H
#define THREADPOOL_H

#define INVALID_POINTER -1
#define MEMORY_ERROR -2
#define PTHREAD_ERROR -3
#define SEMAPHORE_ERROR -4
#define USER_ERROR -5
#define PSHARED 0
#define SUCCESS 0

#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <signal.h>

#include "queue.h"

typedef struct runnable
{
    void (*function)(void *, size_t);
    void *arg;
    size_t argsz;
}
runnable_t;

typedef struct thread_pool
{
    bool noDefer;
    size_t sizeOfPool;
    Queue* tasksQueue;
    pthread_attr_t attr;
    sem_t tasksSemaphore;
    sem_t poolMutex;
    pthread_t *threads;
}
thread_pool_t;

void signalDetector();

int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

#endif
