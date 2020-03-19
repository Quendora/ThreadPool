#include "threadpool.h"
#include "err.h"

static void *threadpool_thread(void *threadpool);

sem_t threadpoolsMutex;
Queue *threadpoolsQueue;

void clearPool(int sig __attribute__((unused)))
{
    while (true)
    {
        if (sem_wait(&threadpoolsMutex)) fatal("sem_wait");

        if (getQueueSize(threadpoolsQueue) == 0)
        {
            if (sem_post(&threadpoolsMutex)) fatal("sem_post");
            break;
        }

        thread_pool_t *tempPool = dequeue(threadpoolsQueue);

        if (!tempPool) fatal("dequeue");

        if (sem_post(&threadpoolsMutex)) fatal("sem_post");

        thread_pool_destroy(tempPool);
    }
}

static void threadpoolsConstructor(void) __attribute__((constructor));

void threadpoolsConstructor(void)
{
    if (sem_init(&threadpoolsMutex, PSHARED, 1)) fatal("sem_init");
    threadpoolsQueue = malloc(sizeof(Queue));

    if (!threadpoolsQueue) fatal("malloc");
    queueInit(threadpoolsQueue);

    struct sigaction action;
    sigset_t block_mask;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);

    action.sa_handler = clearPool;
    action.sa_mask = block_mask;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, 0) == -1)
        fatal("sigaction");

    if (sigprocmask(SIG_UNBLOCK, &block_mask, 0) == -1)
        fatal("sigprocmask block");
}

static void threadpoolsDestructor(void) __attribute__((destructor));

void threadpoolsDestructor(void)
{
    free(threadpoolsQueue);
    if (sem_destroy(&threadpoolsMutex)) fatal("sem_destroy");
}

int thread_pool_init(thread_pool_t *pool, size_t num_threads)
{
    if (!pool)
    {
        syserr("thread_pool_init");

        return INVALID_POINTER;
    }

    pool->noDefer = false;
    pool->sizeOfPool = 0;

    if (!(pool->tasksQueue = malloc(sizeof(Queue))))
    {
        free(pool);
        syserr("malloc");

        return MEMORY_ERROR;
    }

    queueInit(pool->tasksQueue);

    if (pthread_attr_init(&(pool->attr)))
    {
        free(pool->tasksQueue);
        free(pool);
        syserr("pthread_attr_init");

        return PTHREAD_ERROR;
    }

    if (pthread_attr_setdetachstate(&(pool->attr), PTHREAD_CREATE_JOINABLE) != 0)
    {
        free(pool->tasksQueue);
        free(pool);
        pthread_attr_destroy(&(pool->attr));
        syserr("pthread_attr_setdetachstate");

        return PTHREAD_ERROR;
    }

    if (sem_init(&(pool->tasksSemaphore), PSHARED, 0))
    {
        free(pool->tasksQueue);
        pthread_attr_destroy(&(pool->attr));
        free(pool);
        syserr("sem_init");

        return SEMAPHORE_ERROR;
    }

    if (sem_init(&(pool->poolMutex), PSHARED, 1))
    {
        free(pool->tasksQueue);
        pthread_attr_destroy(&(pool->attr));
        sem_destroy(&(pool->tasksSemaphore));
        free(pool);
        syserr("sem_init");

        return SEMAPHORE_ERROR;
    }

    if (!(pool->threads = malloc(num_threads * (sizeof(pthread_t)))))
    {
        free(pool->tasksQueue);
        pthread_attr_destroy(&(pool->attr));
        sem_destroy(&(pool->tasksSemaphore));
        sem_destroy(&(pool->poolMutex));
        free(pool);
        syserr("malloc");

        return MEMORY_ERROR;
    }

    for (; pool->sizeOfPool < num_threads; pool->sizeOfPool++)
    {
        if (pthread_create(&(pool->threads[pool->sizeOfPool]), &(pool->attr),
                threadpool_thread, (void *) pool))
        {
            thread_pool_destroy(pool);
            free(pool);
            syserr("pthread_create");

            return PTHREAD_ERROR;
        }
    }

    if (sem_wait(&threadpoolsMutex))
    {
        thread_pool_destroy(pool);
        free(pool);
        syserr("sem_wait");

        return SEMAPHORE_ERROR;
    }

    if (enqueue(threadpoolsQueue, pool))
    {
        if (sem_post(&threadpoolsMutex)) fatal("sem_post");
        thread_pool_destroy(pool);
        free(pool);
        syserr("malloc");

        return MEMORY_ERROR;
    }

    if (sem_post(&threadpoolsMutex)) fatal("sem_post");

    return SUCCESS;
}

void thread_pool_destroy(struct thread_pool *pool)
{
    if (!pool) fatal("pool is NULL");
    void *retval;

    if (sem_wait(&(pool->poolMutex))) fatal("sem_wait");

    pool->noDefer = true;

    if (sem_post(&(pool->poolMutex))) fatal("sem_post");

    for (size_t i = 0; i < pool->sizeOfPool; i++)
    {
        if (sem_post(&(pool->tasksSemaphore))) fatal("sem_post");
    }

    for (size_t i = 0; i < pool->sizeOfPool; i++)
    {
        if (pthread_join(pool->threads[i], &retval)) fatal("pthread_join");
    }

    free(pool->threads);
    free(pool->tasksQueue);
    pthread_attr_destroy(&(pool->attr));
    if (sem_destroy(&(pool->tasksSemaphore))) fatal("sem_destroy");
    if (sem_destroy(&(pool->poolMutex))) fatal("sem_destroy");

    if (sem_wait(&threadpoolsMutex)) fatal("sem_wait");

    deleteElement(threadpoolsQueue, pool);

    if (sem_post(&threadpoolsMutex)) fatal("sem_post");
}

int defer(struct thread_pool *pool, runnable_t runnable)
{
    if (!pool)
    {
        syserr("pool is NULL");

        return INVALID_POINTER;
    }

    if (sem_wait(&(pool->poolMutex)))
    {
        syserr("sem_wait");

        return SEMAPHORE_ERROR;
    }

    if (pool->noDefer)
    {
        if (sem_post(&(pool->poolMutex))) fatal("sem_post");
        syserr("defer after destroy");

        return USER_ERROR;
    }

    runnable_t *copy = malloc(sizeof(runnable_t));

    if (copy == NULL)
    {
        if (sem_post(&(pool->poolMutex))) fatal("sem_post");
        syserr("malloc");

        return MEMORY_ERROR;
    }

    copy->function = runnable.function;
    copy->arg = runnable.arg;
    copy->argsz = runnable.argsz;

    if (enqueue(pool->tasksQueue, copy))
    {
        if (sem_post(&(pool->poolMutex))) fatal("sem_post");
        free(copy);
        syserr("malloc");

        return MEMORY_ERROR;
    }

    if (sem_post(&(pool->poolMutex))) fatal("sem_post");
    if (sem_post(&(pool->tasksSemaphore))) fatal("sem_post");

    return SUCCESS;
}


static void *threadpool_thread(void *threadpool)
{
    thread_pool_t *pool = (thread_pool_t *) threadpool;

    while (true)
    {
        if (sem_wait(&(pool->tasksSemaphore))) fatal("sem_wait");

        if (sem_wait(&(pool->poolMutex))) fatal("sem_wait");

        if (pool->noDefer && getQueueSize(pool->tasksQueue) == 0)
        {
            if (sem_post(&(pool->poolMutex))) fatal("sem_wait");
            break;
        }

        runnable_t *task = dequeue(pool->tasksQueue);

        if (sem_post(&(pool->poolMutex))) fatal("sem_post");

        (*(task->function))(task->arg, task->argsz);
        free(task);
    }

    return NULL;
}