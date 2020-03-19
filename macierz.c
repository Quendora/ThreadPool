#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadpool.h"
#include "err.h"

int *values;
sem_t *mutexes;
thread_pool_t pool;

typedef struct matrixArgs
{
    int value;
    int time;
    int row;
} matrixArgs_t;

static void calculateValue(void *args, size_t argsz __attribute__((unused)))
{
    matrixArgs_t *matrixArgs = (matrixArgs_t *) args;

    usleep(matrixArgs->time * 1000);

    if (sem_wait(&(mutexes[matrixArgs->row]))) fatal("sem_wait");

    values[matrixArgs->row] += matrixArgs->value;

    if (sem_post(&(mutexes[matrixArgs->row]))) fatal("sem_post");
}


int main(void)
{
    thread_pool_init(&pool, 4);

    int n, m, err;
    scanf("%d", &n);
    scanf("%d", &m);

    values = malloc(sizeof(int) * n);
    mutexes = malloc(sizeof(sem_t) * n);

    matrixArgs_t matrixArgsArr[n][m];

    for (int r = 0; r < n; r++)
    {
        values[r] = 0;
        if (sem_init(&mutexes[r], 0, 1)) return -1;

        for (int k = 0; k < m; k++)
        {
            matrixArgsArr[r][k].row = r;
            scanf("%d", &matrixArgsArr[r][k].value);
            scanf("%d", &matrixArgsArr[r][k].time);

            if ((err = defer(&pool, (runnable_t) {
                    .function = calculateValue,
                    .arg = &matrixArgsArr[r][k],
                    .argsz = sizeof(matrixArgs_t)
            })))
                return err;
        }
    }

    thread_pool_destroy(&pool);

    for (int r = 0; r < n; r++)
    {
        printf("%d\n", values[r]);
    }

    free(values);
    free(mutexes);

    return 0;
}