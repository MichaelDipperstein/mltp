/***************************************************************************
*                    Pi Approximation Using JKThreads
*
*   File    : jkpi.c
*   Purpose : Provides an approximation of pi
*   Author  : Michael Dipperstein
*   Date    : March 16, 2000
*
*   $Id: jkpi.c,v 1.2 2000/08/21 14:48:45 mdipper Exp $
****************************************************************************
*
*   This program uses the rectangle rule for integration to approximate
*   the area of a unit circle (Pi).  Several variations of this code exist
*   for other thread packages.  The original author is unknown.
*
*   Required parameters are:
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
#include <sys/time.h>
#include "jkcthread.h"

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
volatile double pi = 0.0;
volatile int intervals;
volatile int threads;

jksem *sem_pi;      /* used to keep printed stuff together */


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

    /* print the result */
    printf("Estimation of pi is %f\n", pi);

    seconds = (double)(t2.tv_usec - t1.tv_usec)/1000000.0;
    seconds += (double)(t2.tv_sec - t1.tv_sec);
    printf("Computation took %e seconds.\n", seconds);

    return(0);
}
