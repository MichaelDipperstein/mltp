/***************************************************************************
*                             MLTP Lock Testing
*
*   File    : locks.c
*   Purpose : Test mutex acquisition
*   Author  : Michael Dipperstein
*   Date    : June 25, 2000
*
*   $Id: locks.c,v 1.2 2000/08/21 20:22:20 mdipper Exp $
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
*                            GLOBAL VARIABLES
***************************************************************************/
/* one lock of each */
mltp_lock_t spinLock, blockLock, frontBlockLock, semLock;

mltp_barrier_t barrier; /* barrier between lock acquisitions */

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : Test
*   Description: This function is the entry point for each thread.  It
*                acquires one lock of each type.  Spining for a few cycles
*                while the lock is held.  Between lock acquisitions, a
*                barrier is entered allowing other threads to acquire a lock
*                of this type before the next lock type is tested.
*   Parameters : id - this thread id
*                threads - total number of threads
*   Effects    : Locks are acquired and released.
*   Returned   : NULL
****************************************************************************/
void *Test(int id, int threads)
{
    long i;

    /* every one go through a spin lock */
    mltp_lock(&spinLock);

    printf("Thread %02d of %02d acquired spin lock\n", id, threads);
    for(i = 0; i < 1000000; i++);

    mltp_unlock(&spinLock);

    /* wait for everyone to finish */
    mltp_barrier(&barrier, threads);

    /* every one go through a block lock */
    mltp_lock(&blockLock);

    printf("Thread %02d of %02d acquired block lock\n", id, threads);
    for(i = 0; i < 1000000; i++);

    mltp_unlock(&blockLock);

    /* wait for everyone to finish */
    mltp_barrier(&barrier, threads);

    /* every one go through a front block lock */
    mltp_lock(&frontBlockLock);

    printf("Thread %02d of %02d acquired front block lock\n", id, threads);
    for(i = 0; i < 1000000; i++);

    mltp_unlock(&frontBlockLock);

    /* wait for everyone to finish */
    mltp_barrier(&barrier, threads);

    /* every one go through a semaphore lock */
    mltp_lock(&semLock);

    printf("Thread %02d of %02d acquired semaphore lock\n", id, threads);
    for(i = 0; i < 1000000; i++);

    mltp_unlock(&semLock);

    return(NULL);
}


/****************************************************************************
*   Function   : ShowUsage
*   Description: This function is simply a call to malloc that verifies
*                success, otherwise it calls perror and aborts.
*   Parameters : program - name used to invoke this program from the command
*                line.
*   Effects    : Usage message is output on stderr
*   Returned   : None
****************************************************************************/
void ShowUsage(char *program)
{
    fprintf(stderr, "Usage: %s <threads> <vps>\n", program);
}

/****************************************************************************
*   Function   : main
*   Description: This is the main function of the lock test code.  It is
*                responsible for all but the threaded functions.
*   Parameters : argc - argument count
*                argv - arguments
*   Effects    : Initializes threads and locking structures
*   Returned   : 0 for success, otherwise non-zero
****************************************************************************/
int main(int argc, char *argv[])
{
    mltp_t **threads;
    int threadCount, vpCount;
    int i;

    /* determine number of threads to run */
    if (argc != 3)
    {
        ShowUsage(argv[0]);
        exit(1);
    }
    else
    {
        threadCount = atoi(argv[1]);
        vpCount = atoi(argv[2]);
    }

    /* validate parameters */
    if ((threadCount < 1) || (vpCount < 1) || (threadCount < vpCount))
    {
        ShowUsage(argv[0]);
        exit(1);
    }

    /* allocate threads */
    threads = (mltp_t **)malloc(threadCount * sizeof(mltp_t *));
    if (threads == NULL)
    {
        fprintf(stderr, "error: failed to allocate thread handles\n");
        exit(1);
    }

    mltp_init();

    /* initialize locks */
    mltp_lock_init(&spinLock, MLTP_LOCK_STD);
    mltp_lock_init(&blockLock, MLTP_LOCK_BLOCK);
    mltp_lock_init(&frontBlockLock, MLTP_LOCK_BLOCK_FRONT);
    mltp_lock_init(&semLock, MLTP_LOCK_SEMAPHORE);
    mltp_barrier_init(&barrier);

    /* create threads */
    for (i = 0; i < threadCount; i++)
    {
        threads[i] = mltp_vcreate((mltp_vuserf_t*)Test,
            2 * sizeof(qt_word_t), i + 1, threadCount);
    }

    /* start threads */
    mltp_start(vpCount);

    /* free thread structures */
    for (i = 0; i < threadCount; i++)
    {
        free(threads[i]);
    }

    return(0);
}

