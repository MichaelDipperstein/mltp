#include <stdlib.h>
#include <stdio.h>
#include "tlib.h"

/*
EXTERN_ENV
*/

#include "extenv.h"

void MainInit(void)
{
    mltp_init();
    mltp_lock_init(&threadMutex, MLTP_LOCK_SPIN);
}

void MainEnd(void)
{
    int i;

    for (i = 0; i < threadCount; i++)
    {
        free(parmacsThreads[i]);
    }

    free(parmacsThreads);
}

void Create(int numThreads, mltp_userf_t *startRoutine)
{

    /* Lock to access global thread stuff */
    mltp_lock(&threadMutex);
    threadCount++;

    if (threadCount == 1)
    {
        parmacsThreads = (mltp_t **)malloc(numThreads * sizeof(mltp_t *));
    }
    
    parmacsThreads[threadCount - 1] =
        mltp_create(startRoutine, (void *)NULL);

    mltp_unlock(&threadMutex);
}

void Clock(unsigned int *time)
{
    struct timeval fullTime;

    gettimeofday(&fullTime, NULL);
    *time = (unsigned int)((fullTime.tv_usec) +
        1000000 * (fullTime.tv_sec));
}
