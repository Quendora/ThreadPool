#include <stddef.h>
#include <stdio.h>

#include "future.h"
#include "threadpool.h"

#define NUM_OF_THREADS 3

typedef struct factorialArgs
{
    unsigned long long factorial;
    unsigned long long n;
} factorialArgs_t;

void *calculateFactorial(void *args, size_t argsz __attribute__((unused)), size_t *resultSize)
{
    *resultSize = sizeof(factorialArgs_t*);
    factorialArgs_t *factorialArgs = (factorialArgs_t *) args;
    factorialArgs->factorial *= factorialArgs->n;
    factorialArgs->n += NUM_OF_THREADS;

    return (void *) factorialArgs;
}

int main(void)
{
    unsigned long long result = 1;
    int err, n;
    scanf("%d", &n);

    if (n <= 3)
    {
        if (n < 0) printf("Invalid argument\n");
        else if (n <= 1) printf("%d\n", 1);
        else if (n == 2) printf("%d\n", 2);
        else printf("%d\n", 6);

        return 0;
    }

    future_t futures[n];
    factorialArgs_t factorialArgs[NUM_OF_THREADS];
    thread_pool_t pool;

    if ((err = thread_pool_init(&pool, NUM_OF_THREADS))) return err;

    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        factorialArgs[i].n = i + 1;
        factorialArgs[i].factorial = 1;

        if ((err = async(&pool, &futures[i], (callable_t) {
                .function = calculateFactorial,
                .arg = (void *) &factorialArgs[i],
                .argsz = sizeof(factorialArgs_t)}))) return err;
    }

    for (int i = NUM_OF_THREADS; i < n; i++) //TODO
    {
        if ((err = map(&pool, &futures[i], &futures[i - NUM_OF_THREADS], calculateFactorial))) return err;
    }

    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        factorialArgs_t *f = (factorialArgs_t *) await(&futures[n - i - 1]);
        result *= f->factorial;
    }

    thread_pool_destroy(&pool);
    printf("%llu\n", result);

    return 0;
}