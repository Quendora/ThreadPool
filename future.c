#include "future.h"
#include "err.h"

#define WAITING 0
#define READY 1

typedef void *(*function_t)(void *);

static void runnableAsyncFunction(void *fut, size_t argsz __attribute__((unused)))
{
    future_t *future = (future_t *) fut;
    callable_t *callable = future->callable;

    future->result = (*(callable->function))(callable->arg, callable->argsz,
            &(future->resultSize));

    free(future->callable);
    future->status = READY;
    if (sem_post(&(future->mutex))) fatal("sem_post");
}

int async(thread_pool_t *pool, future_t *future, callable_t callable)
{
    if (!pool || !future)
    {
        syserr("pool OR future is NULL");
        return INVALID_POINTER;
    }

    if (!(future->callable = malloc(sizeof(callable_t))))
    {
        syserr("malloc");
        return MEMORY_ERROR;
    }

    future->callable->function = callable.function;
    future->callable->arg = callable.arg;
    future->callable->argsz = callable.argsz;
    future->status = WAITING;

    if (sem_init(&(future->mutex), PSHARED, 0))
    {
        free(future->callable);
        syserr("sem_init");
        return SEMAPHORE_ERROR;
    }

    int err;
    if ((err = defer(pool, (runnable_t) {
            .function = runnableAsyncFunction,
            .arg = future,
            .argsz = sizeof(future_t)
    })))
    {
        free(future->callable);
        sem_destroy(&(future->mutex));
        syserr("defer");
        return err;
    }

    return SUCCESS;
}

static void runnableMapFunction(void *args, size_t argsz __attribute__((unused)))
{
    mapData_t *mapData = args;
    void* resultFrom = await(mapData->from);
    size_t resultFromSize = mapData->from->resultSize;

    mapData->future->result = (mapData->function)(resultFrom, resultFromSize,
            &(mapData->future->resultSize));

    mapData->future->status = READY;
    if (sem_post(&(mapData->future->mutex))) fatal("sem_post");
    free(mapData);
}

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *))
{
    if (!pool || !future || !from || !function)
    {
        syserr("pool OR future OR from OR function is NULL");
        return INVALID_POINTER;
    }

    mapData_t *mapData = malloc(sizeof(mapData_t));

    if (mapData == NULL)
    {
        syserr("malloc");
        return MEMORY_ERROR;
    }

    mapData->future = future;
    mapData->from = from;
    mapData->function = function;

    future->status = WAITING;

    if (sem_init(&(future->mutex), PSHARED, 0))
    {
        free(mapData);
        syserr("sem_init");
        return SEMAPHORE_ERROR;
    }

    int err;
    if ((err = defer(pool, (runnable_t) {
            .function = runnableMapFunction,
            .arg = mapData,
            .argsz = sizeof(mapData_t)
    })))
    {
        free(mapData);
        sem_destroy(&(future->mutex));
        syserr("defer");
        return err;
    }

    return SUCCESS;
}

void *await(future_t *future)
{
    while (future->status != READY)
    {
        if (sem_wait(&(future->mutex))) fatal("sem_wait");
    }

    if (sem_destroy(&(future->mutex))) fatal("sem_destroy");

    return future->result;
}
