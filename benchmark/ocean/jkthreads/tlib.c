#include <stdlib.h>
#include <stdio.h>
#include "tlib.h"

/*
EXTERN_ENV
*/

#include "extenv.h"

void MainInit(void)
{
    /* initialize thread package */
    jkthread_init();
    
    /* initialize thread lock */
    threadMutex = jksem_create();
}

void MainEnd(void)
{
    /* kill mutex */
    jksem_kill(threadMutex);

    /* free thread pointers */
    free(parmacsThreads);
}

void Create(int numThreads, void (*startRoutine)(void *))
{

    /* Lock to access global thread stuff */
    jksem_get(threadMutex);
    threadCount++;

    if (threadCount == 1)
    {
        parmacsThreads = (int *)malloc(numThreads * sizeof(pid_t *));
    }
    
    parmacsThreads[threadCount - 1] =
        jkthread_create(startRoutine, 0, 0, NULL);

    jksem_release(threadMutex);
}

void JoinAllThreads(void)
{
    int i;
    
    /* Lock to access global thread stuff */
    jksem_get(threadMutex);

    /* join all the threads back after they are done */
    for(i = 0; i < threadCount; i++)
    {
        /* Join threads back */
        jkthread_join(parmacsThreads[i]);
    }

    threadCount = 0;
    jksem_release(threadMutex);
}

void Clock(unsigned int *time)
{
    struct timeval fullTime;

    gettimeofday(&fullTime, NULL);
    *time = (unsigned int)((fullTime.tv_usec) +
        1000000 * (fullTime.tv_sec));
}
