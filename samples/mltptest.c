/***************************************************************************
*                       Test MLTP Thread Functions
*
*   File    : mltptest.c
*   Purpose : Test mltp functions and provide usage examples
*   Author  : Michael Dipperstein
*   Date    : February 19, 2000
*
*   $Id: mltptest.c,v 1.2 2000/08/21 20:22:20 mdipper Exp $
****************************************************************************
*
*   This program cycles through a simple series of tests which exercise a
*   majority of the mltp functions available.
*
*   Program execution:
*       mltptest [cycles [vps]]
*
*       cycles is the number times to cycle through the test
*       vps is the number of virtual processors (1 - 4 are meaningful)
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

#include <stdarg.h> /* For varargs tryout. */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "mltp.h"

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/* threads used by test */
mltp_t *thread0,
       *thread1,
       *thread2,
       *thread3,
       *bound;

unsigned int entryCount = 0;
unsigned int boundCount = 0;
volatile int done = 0;
mltp_barrier_t barrier;

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : Thread0Proc
*   Description: This function is the entry point for mltp thread0.  This
*                thread yields, enters a barrier and returns.  Execution
*                status is displayed along the way.
*   Parameters : None
*   Effects    : Thread execution status is sent to stdout at major events
*   Returned   : 13
****************************************************************************/
static void *Thread0Proc(void)
{
    printf("Entered thread 0\tEntry count: %d\n", ++entryCount);
    printf("\tNo parameters\n");

    printf("Thread 0 blocking.\n");
    mltp_yield();

    printf("Returned to thread 0\n");

    printf("Thread 0 entering barrier\n");
    mltp_barrier(&barrier, 4);

    printf("Leaving thread 0\n");
    return((void *)13);
}


/****************************************************************************
*   Function   : Thread1Proc
*   Description: This function is the entry point for mltp thread1.  This
*                thread sums its parameters, yields, enters a barrier and
*                returns.  Execution status is displayed along the way.
*   Parameters : a - first parameter
*                b - second parameter
*   Effects    : Thread execution status is sent to stdout at major events
*   Returned   : 26
****************************************************************************/
static void *Thread1Proc(int a, int b)
{
    int sum;

    printf("Entered thread 1\tEntry count: %d\n", ++entryCount);
    printf("\tParam a: %d\tParam b: %d\n", a, b);

    /* calculate sum before blocking */
    sum = a + b;

    printf("Thread 1 blocking.\n");
    mltp_yield();

    printf("Returned to thread 1\n");
    printf("\tParameter sum: %d\n", sum);

    printf("Thread 1 entering barrier\n");
    mltp_barrier(&barrier, 4);

    printf("Leaving thread 1\n");
    return((void *)26);
}


/****************************************************************************
*   Function   : Thread2Proc
*   Description: This function is the entry point for mltp thread2.  This
*                thread sums its parameters, yields to the head of the queue,
*                enters a barrier and returns.  Execution status is displayed
*                along the way.
*   Parameters : a - first parameter
*                b - second parameter
*                c - third parameter
*   Effects    : Thread execution status is sent to stdout at major events
*   Returned   : 39
****************************************************************************/
static void *Thread2Proc(double a, double b, double c)
{
    double sum;

    printf("Entered thread 2\tEntry count: %d\n", ++entryCount);
    printf("\tParam a: %f\tParam b: %f\tParam a: %f\n", a, b, c);

    /* calculate sum before blocking */
    sum = a + b +c;

    printf("Thread 2 blocking for quehead.\n");
    mltp_yield_to_first();

    printf("Returned to thread 2\n");
    printf("\tParameter sum: %f\n", sum);

    printf("Thread 2 entering barrier\n");
    mltp_barrier(&barrier, 4);

    printf("Leaving thread 2\n");
    return((void *)39);
}


/****************************************************************************
*   Function   : Thread3Proc
*   Description: This function is the entry point for mltp thread2.  This
*                thread yields to the head of the queue, enters a barrier,
*                and returns.  Execution status is displayed along the way.
*   Parameters : a - only parameter
*   Effects    : Thread execution status is sent to stdout at major events
*   Returned   : 42
****************************************************************************/
static void *Thread3Proc(void *a)
{
    printf("Entered thread 3\tEntry count: %d\n", ++entryCount);
    printf("\tParam a: %d\n", (int)a);

    printf("Thread 3 blocking.\n");
    mltp_yield();

    printf("Returned to thread 3\n");
    printf("\tParam a: %d\n", (int)a);

    printf("Thread 3 blocking for quehead.\n");
    mltp_yield_to_first();

    printf("Returned to thread 3\n");
    printf("\tParam a: %d\n", (int)a);

    printf("Thread 3 entering barrier\n");
    mltp_barrier(&barrier, 4);

    printf("Leaving thread 3\n");
    return((void *)42);
}


/****************************************************************************
*   Function   : BoundThread
*   Description: This function is the entry point for this program's bound
*                thread.  The thread yields and increments a counter
*                everytime it returns from a yield.   The yielding will stop
*                once the main thread sets the done flag
*   Parameters : p0 - not used
*   Effects    : Bound count is incremented everytime the thread executes.
*   Returned   : None
****************************************************************************/
static void BoundThread(void *p0)
{

    printf("Entered thread bound to %d.\n", getpid());

    /* keep running until all unbound threads finish */
    while (!done)
    {
        boundCount = (boundCount + 1) % UINT_MAX;
        mltp_yield_bound();
    }

    done = 0;
    return;
}


/****************************************************************************
*   Function   : ThreadTest
*   Description: This function starts the thread test.  It creates and
*                starts a bound thread and then for executes the unbound
*                thread test for the specified number of times.  Once all
*                tests have completed this function cleans up.
*   Parameters : n - number of unbound test cycles
*                vps - number of virtual processors
*   Effects    : Threads are created and tests are run
*   Returned   : None
****************************************************************************/
void ThreadTest(int n, int vps)
{
    mltp_init();
    mltp_barrier_init(&barrier);
    
    /* start bound thread */
    bound = mltp_create_bound(BoundThread, NULL, 0, NULL);

    while (n > 0)
    {
        n--;

        printf("\n\t********************\n\n");

        /* initialize all threads */
        thread0 = mltp_vcreate((mltp_vuserf_t*)Thread0Proc, 0);
        thread1 = mltp_vcreate((mltp_vuserf_t*)Thread1Proc,
                               2 * sizeof(qt_word_t), 42, 2112);
        thread2 = mltp_vcreate((mltp_vuserf_t*)Thread2Proc,
                               3 * sizeof(double), 2.71828, 3.14159, 10.5);
        thread3 = mltp_create((mltp_userf_t*)Thread3Proc, (void *)5150);
        
        mltp_start(vps);

        if (thread0 != NULL)
        {
            printf("Thread 0 returned %d\n", (int)thread0->retval);
            free(thread0);
        }
        else
        {
            printf("ERROR!! NULL thread structure\n");
        }

        if (thread1 != NULL)
        {
            printf("Thread 1 returned %d\n", (int)thread1->retval);
            free(thread1);
        }
        else
        {
            printf("ERROR!! NULL thread structure\n");
        }

        if (thread2 != NULL)
        {
            printf("Thread 2 returned %d\n", (int)thread2->retval);
            free(thread2);
        }
        else
        {
            printf("ERROR!! NULL thread structure\n");
        }

        if (thread3 != NULL)
        {
            printf("Thread 3 returned %d\n", (int)thread3->retval);
            free(thread3);
        }
        else
        {
            printf("ERROR!! NULL thread structure\n");
        }
    }
    
    /* signal bound thread that it's time to end. */
    done = 1;
    
    /* wait for bound thread to end */
    mltp_join_bound(*bound);

    printf("Bound thread ran %d cycles.\n", boundCount);
    free(bound);
}


/****************************************************************************
*   Function   : main
*   Description: This function is the entry point for the program, it parses
*                the command line arguements and kicks off thread execution.
*   Parameters : argc - number of arguements
*                argv - command line arguements
*   Effects    : Starts program execution
*   Returned   : 0
****************************************************************************/
int main (int argc, char **argv)
{
    int n, vps;

    if (argc == 1)
    {
        n = 3;
        vps = 1;
    }
    else if (argc == 2)
    {
        n = atoi(argv[1]);

        if (n == 0)
        {
            n = 1;
        }

        vps = 1;
    }
    else
    {
        n = atoi(argv[1]);

        if (n == 0)
        {
            n = 1;
        }

        vps = atoi(argv[2]);

        if (vps == 0)
        {
            vps = 1;
        }
    }

    ThreadTest(n, vps);

    return(0);
}
