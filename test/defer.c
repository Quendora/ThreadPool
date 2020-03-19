#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "minunit.h"
#include "threadpool.h"

int tests_run = 0;

#define NROUNDS 10

static void pong_ping(void *args, size_t argsz __attribute__((unused)))
{
    printf("wchodzi do pong_ping\n");
    sem_t *ping = args;
    sem_t *pong = (sem_t *) args + 1;

    for (int i = 0; i < NROUNDS; ++i)
    {
        printf("przed ping wait poboczny %d\n", i);
        sem_wait(ping);
        sem_post(pong);
    }

    sem_destroy(ping);
}

static char *ping_pong()
{
    thread_pool_t pool;
    thread_pool_init(&pool, 1);

    assert(pool.tasksQueue != NULL);
    printf("tojuztu\n");

    sem_t *pingpong = malloc(sizeof(sem_t) * 2);
    sem_t *ping = pingpong;
    sem_t *pong = pingpong + 1;
    printf("po semaforach\n");

    sem_init(ping, 0, 0);
    sem_init(pong, 0, 0);

    printf("przed defer\n");
    defer(&pool, (runnable_t) {
            .function = pong_ping,
            .arg = pingpong,
            .argsz = sizeof(sem_t) * 2
    });

    printf("po defer\n");
    for (int i = 0; i < NROUNDS; ++i)
    {
        printf("przed ping glowny %d\n", i);
        sem_post(ping);
        printf("przed pong glowny %d\n", i);
        sem_wait(pong);
        printf("po pong glowny %d\n", i);

    }
    printf("po forze\n");

    sem_destroy(pong);
    free(pingpong);

    printf("przed destory\n");
    thread_pool_destroy(&pool);
    printf("po destory\n");
    return 0;
}

static char *all_tests()
{
    mu_run_test(ping_pong);
    return 0;
}

int main()
{
    char *result = all_tests();
    if (result != 0)
    {
        printf(__FILE__ ": %s\n", result);
    }
    else
    {
        printf(__FILE__ ": ALL TESTS PASSED\n");
    }
    printf(__FILE__ "Tests run: %d\n", tests_run);

    return result != 0;
}
