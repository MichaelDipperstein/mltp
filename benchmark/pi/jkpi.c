#include <stdio.h>
#include <sys/time.h>
#include "jkcthread.h"

volatile double pi = 0.0;
volatile int intervals;
volatile int threads;

jksem *sem_pi;      /* used to keep printed stuff together */

void process(void *data)
{
    register double width, localsum;
    register int i;
    register int iproc = (int)data;

    /* set width */
    width = 1.0 / intervals;

    /* do the local computations */
    localsum = 0;
    for (i = iproc; i < intervals; i += threads)
    {
        register double x = (i + 0.5) * width;
        localsum += 4.0 / (1.0 + x * x);
    }
    localsum *= width;

    /* get permission, update pi, and unlock */
    jksem_get(sem_pi);
    pi += localsum;
    jksem_release(sem_pi);
}

int main(int argc, char **argv)
{
    struct timeval t1, t2;
    double seconds;
    int i, *pid;

    if (argc != 3)
    {
        /* use default values */
        threads = 2;
        intervals = 1000;
    }
    else
    {
        /* get command line values */
        threads = atoi(argv[1]);
        intervals = atoi(argv[2]);
    }

    /* initialize package */
    jkthread_init();

    sem_pi = jksem_create();

    /* allocate space to store thread pids */
    pid = (int *)malloc(threads * sizeof(int));

    if (pid == NULL)
    {
        perror("allocating pid array");
        exit(0);
    }

    /* record start time */
    gettimeofday(&t1, NULL);

    /* create and start threads */
    for (i = 0; i < threads; i++)
    {
        pid[i] = jkthread_create(process, (void *)i, 0, NULL);
    }

    /* now wait for all the threads to increment sem_join */
    for (i = 0; i < threads; i++)
    {
        jkthread_join(pid[i]);
    }

    /* record finish time */
    gettimeofday(&t2, NULL);

    /* clean-up */
    jksem_kill(sem_pi);
    free(pid);

    seconds = (double)(t2.tv_usec - t1.tv_usec)/1000000.0;
    seconds += (double)(t2.tv_sec - t1.tv_sec);

    /* print the result */
    printf("Estimation of pi is %f, computation took %e seconds.\n",
        pi, seconds);

    return(0);
}
