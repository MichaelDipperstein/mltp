#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mltp.h"

volatile double pi = 0.0;  /* Approximation to pi (shared) */
mltp_lock_t pi_lock;       /* Lock for above */
volatile double intervals; /* How many intervals? */

static void *Process(int iproc, int nthrds)
{
    register double width, localsum;
    register int i;

    /* Set width */
    width = 1.0 / intervals;

    localsum = 0;

    /* Do the local computations */
    for (i = iproc; i < intervals; i += nthrds)
    {
        register double x = (i + 0.5) * width;
        localsum += 4.0 / (1.0 + x * x);
    }

    localsum *= width;

    /* Lock pi for update, update it, and unlock */
    mltp_lock(&pi_lock);
    pi += localsum;
    mltp_unlock(&pi_lock);

    return(NULL);
}


int main(int argc, char *argv[])
{
    mltp_t **threads;
    int nthrds, nvps, i;
    struct timeval t1, t2;
    double seconds;

    /* Check arguments */
    if (argc != 4)
    {
        fprintf(stderr, "syntax: %s vps threads intervals\n", argv[0]);
        exit(1);
    }
    
    /* Get number of threads */
    nthrds = atoi(argv[2]);
    if (nthrds < 1)
    {
        fprintf(stderr,
            "error: number of threads (%s) must be greater than 0\n",
            argv[2]);
        exit(1);
    }

    /* Get number of virtual processors */
    nvps = atoi(argv[1]);
    if (nvps > nthrds)
    {
        fprintf(stderr,
            "error: number of vps (%s) must be <= number of threads (%d)\n",
            argv[1], nthrds);
        exit(1);
    }
    else if (nvps < 1)
    {
        fprintf(stderr,
            "error: number of vps (%s) must be greater than 0\n",
            argv[1]);
        exit(1);
    }

    /* Get the number of intervals */
    intervals = atoi(argv[3]);
    if (intervals < 1)
    {
        fprintf(stderr,
            "error: number of intervals (%s) must be greater than 0\n",
            argv[3]);
        exit(1);
    }

    /* Allocate handles for threads */
    threads = (mltp_t **)malloc(nthrds * sizeof(mltp_t *));
    if (threads == NULL)
    {
        fprintf(stderr, "error: failed to allocate thread handles\n");
        exit(1);
    }

    /* record start time */
    gettimeofday(&t1, NULL);
            
    /* Initialize the thread package */
    mltp_init();

    /* Initialize the lock on pi */
    mltp_lock_init(&pi_lock, MLTP_LOCK_STD);

    /* Make a thread for each process */
    for (i = 0; i < nthrds; i++)
    {
        threads[i] = mltp_vcreate(
            (mltp_vuserf_t*)Process, 2 * sizeof(int), i, nthrds);
    }

    /* Run the threads */
    mltp_start(nvps);
    
    /* Free the threads */
    for (i = 0; i < nthrds; i++)
    {
        free(threads[i]);
    }
    free(threads);

    /* record finish time */
    gettimeofday(&t2, NULL);
    seconds = (double)(t2.tv_usec - t1.tv_usec)/1000000.0;
    seconds += (double)(t2.tv_sec - t1.tv_sec);
    printf("Computation took %e seconds, ", seconds);

    /* Print the result */
    printf("estimation of pi is %f\n", pi);

    /* Check-out */
    exit(0);
}

