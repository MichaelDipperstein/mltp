/***************************************************************************
*                       Pi Approximation Using MLTP
*
*   File    : mltppi.c
*   Purpose : Provides an approximation of pi
*   Author  : Michael Dipperstein
*   Date    : March 16, 2000
*
*   $Id: mltppi.c,v 1.2 2000/08/21 20:22:20 mdipper Exp $
****************************************************************************
*
*   This program uses the rectangle rule for integration to approximate
*   the area of a unit circle (Pi).  Several variations of this code exist
*   for other thread packages.  The original author is unknown.
*
*   Required parameters are:
*       The number of virtual processors
*       The number of threads
*       The number of rectangular intervals
*
****************************************************************************
*
* MLTP: Multi-Layer thread package for SMP Linux
* Copyright (C) 2000 by Michael Dipperstein (mdipper@cs.ucsb.edu)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "mltp.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#undef DEBUG

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
volatile double pi = 0.0;  /* Approximation to pi (shared) */
mltp_lock_t pi_lock;       /* Lock for above */
volatile double intervals; /* How many intervals? */

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : Process
*   Description: This function is the entry point for each mltp thread.  Each
*                thread entering the function will compute the area for each
*                interval it is responsible for and add the total area of
*                those intervals to a global sum.
*   Parameters : iproc - thread id
*                nthrds - total number of threads
*   Effects    : The portion of the calculation made here is added to a
*                global sum.
*   Returned   : NULL
****************************************************************************/
static void *Process(int iproc, int nthrds)
{
    register double width, localsum;
    register int i;

#ifdef DEBUG
    printf("Entered thread %d\n", iproc);
#endif
    
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

#ifdef DEBUG
    printf("Exiting thread %d\n", iproc);
#endif

    return(NULL);
}


/****************************************************************************
*   Function   : main
*   Description: This function creates all the data structures and threads
*                required to solve the problem, then it starts the threads.
*                once the calculations are done, timed results are printed
*                and the program is exited.
*   Parameters : argc - arguement count
*                argv - command line arguements
*   Effects    : Data structures and threads are created and results of the
*                calculation are output.
*   Returned   : 0 on success, 1 on failure
****************************************************************************/
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
    printf("Computation took %e seconds.\n", seconds);

    /* Print the result */
    printf("Estimation of pi is %f\n", pi);

    /* Check-out */
    exit(0);
}

