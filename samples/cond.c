/***************************************************************************
*                  MLTP Conditional Waiting and Signaling
*
*   File    : cond.c
*   Purpose : Verify mltp conditional primitives
*   Author  : Michael Dipperstein
*   Date    : June 20, 2000
*
*   $Id: cond.c,v 1.2 2000/08/18 14:50:23 mdipper Exp $
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
/* thread handles */
mltp_t *thread0,
       *thread1,
       *thread2,
       *thread3,
       *thread4,
       *thread5,
       *thread6,
       *thread7;


mltp_cond_t cond;   /* conditional signalled on */


/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : ThreadProc
*   Description: This is the thread function to test conditionals.  Each
*                thread uses the same function however, the thread's ID is
*                to determine the action taken by each thread.
*   Parameters : thread - the thread ID
*   Effects    : None
*   Returned   : None
****************************************************************************/
static void *ThreadProc(void *thread)
{
    printf("\tEntered thread %d\n", (int)thread);

    switch ((int)thread)
    {
        case 0:
        case 3:
        case 5:
            /* wait on condition */
            printf("\tThread %d waiting\n", (int)thread);
            mltp_cond_wait(&cond);
            printf("\tThread %d restarted\n", (int)thread);
            break;

        case 1:
        case 4:
        case 7:
            /* yeild on run queue */
            printf("\tThread %d yielding\n", (int)thread);
            mltp_yield();
            printf("\tThread %d restarted\n", (int)thread);
            break;

        case 2:
            /* signal for individual thread */
            printf("\tThread %d signalling\n", (int)thread);
            mltp_cond_signal(&cond);
            break;

        case 6:
            /* broadcast to release all threads */
            printf("\tThread %d signalling\n", (int)thread);
            mltp_cond_broadcast(&cond);
            break;
    }

    printf("\tThread %d returning\n", (int)thread);
    return(NULL);
}


/****************************************************************************
*   Function   : ThreadTest
*   Description: This function is responsible for creating, dispatching, and
*                freeing each thread.
*   Parameters : n - number of times to repeate the process
*   Effects    : None
*   Returned   : None
****************************************************************************/
void ThreadTest(int n)
{
    int pass = 0;

    mltp_init();
    mltp_cond_init(&cond);

    while (pass < n)
    {
        pass++;

        /* create and initialize all threads */
        thread0 = mltp_create((mltp_userf_t*)ThreadProc, (void *)0);
        thread1 = mltp_create((mltp_userf_t*)ThreadProc, (void *)1);
        thread2 = mltp_create((mltp_userf_t*)ThreadProc, (void *)2);
        thread3 = mltp_create((mltp_userf_t*)ThreadProc, (void *)3);
        thread4 = mltp_create((mltp_userf_t*)ThreadProc, (void *)4);
        thread5 = mltp_create((mltp_userf_t*)ThreadProc, (void *)5);
        thread6 = mltp_create((mltp_userf_t*)ThreadProc, (void *)6);
        thread7 = mltp_create((mltp_userf_t*)ThreadProc, (void *)7);

        
        printf("Pass %d of %d\n", pass, n);
        mltp_start(1);  /* run threads on 1 VP */

        /* free threads */
        free(thread0);
        free(thread1);
        free(thread2);
        free(thread3);
        free(thread4);
        free(thread5);
        free(thread6);
        free(thread7);

        printf("\n");
    }

}


/****************************************************************************
*   Function   : main
*   Description: This function is the entry and exit point for the program.
*                It parses the input for an itteration count and then uses
*                that value to call the function that kicks of the testing.
*   Parameters : argc - number of arguments (should be 1 or 2)
*                argv - list of arguments
*   Effects    : None
*   Returned   : 0 is returned upon completion
****************************************************************************/
int main (int argc, char **argv)
{
    int n;

    if (argc == 1)
    {
        /* default three passes */
        n = 3;
    }
    else
    {
        /* accept users pass count */
        n = atoi(argv[1]);

        if (n <= 0)
        {
            n = 1;
        }
    }

    /* run the test */
    ThreadTest(n);

    return(0);
}
