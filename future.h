#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct callable
{
    void *(*function)(void *, size_t, size_t *);
    void *arg;
    size_t argsz;
} callable_t;

typedef struct future
{
    bool status;
    sem_t mutex;
    void *result;
    size_t resultSize;
    callable_t* callable;

} future_t;

typedef struct mapData
{
    future_t *future;
    future_t *from;
    void *(*function)(void *, size_t, size_t *);
} mapData_t;

int async(thread_pool_t *pool, future_t *future, callable_t callable);

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *));

void *await(future_t *future);

#endif
