/***************************************************************************
*                     Test Basic JKThreads Primitives
*
*   File    : jktest.c
*   Purpose : Test functionality of basic thread primitives
*   Author  : Michael Dipperstein
*   Date    : March 16, 2000
*
*   $Id: jktest.c,v 1.4 2000/08/21 14:48:59 mdipper Exp $
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
#include "jkcthread.h"

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
FILE *outfile;
int threads;
jkbarrier barrier;

extern int _IO_fprintf(_IO_FILE*, const char*, ...);


/****************************************************************************
*   Function   : thread
*   Description: This function is the entry point for each thread.  It runs
*                them through three barriers and writes the status to
*                standard out.  This tests semphores, barriers, and
*                atomicity of the fprintf function.
*   Parameters : data - thread number
*   Effects    : Oputs thread termination status
*   Returned   : NULL
****************************************************************************/
void thread(void *data)
{
    int *local;
    
    /* create thread specific data and loose local pointer */
    local = (int *)jkthread_alloclocal(sizeof(int));
    *local = 13 * ((int)data);
    local = NULL;
    
    /* get local again */
    local = (int *)jkthread_getlocal();
    
    printf("Process %d running thread %d\n", getpid(), (int)data);
    printf("Thread %d entering first barrier\n", (int)data);
    fprintf(outfile, "Process %d running thread %d\n", getpid(), (int)data);
    fprintf(outfile, "Thread %d entering first barrier\n", (int)data);

    jkbarrier_enter(&barrier, threads);

    printf("Thread %d entering second barrier\n", (int)data);
    fprintf(outfile, "Thread %d entering second barrier\n", (int)data);

    jkbarrier_enter(&barrier, threads);

    printf("Thread %d entering third barrier\n", (int)data);
    fprintf(outfile, "Thread %d entering third barrier\n", (int)data);

    jkbarrier_enter(&barrier, threads);

    printf("Thread %d thread specific data: %d\n", (int)data, *local);
    fprintf(outfile, "Thread %d thread specific data: %d\n",
        (int)data, *local);

    return;
}


/****************************************************************************
*   Function   : terminate
*   Description: This function writes thread id and the id of the process
*                that terminates the thread
*   Parameters : data - thread number
*   Effects    : Oputs thread termination status
*   Returned   : NULL
*
*   Note: This function is called in context of signal in main process when
*   a thread terminates don't use recursive semaphores because they are not
*   signal safe.
****************************************************************************/
void terminate(void *data)
{
    _IO_fprintf(stdout, "Thread %d terminated from process %d\n",
        (int)data, getpid());
    return;
}


/****************************************************************************
*   Function   : main
*   Description: This function creates all the data structures and threads
*                used for this test.  After the threads have completed their
*                execution, all structures will be freed.
*   Parameters : argc - arguement count
*                argv - command line arguements
*   Effects    : Data structures and threads are created and finished after
*                they're used.
*   Returned   : 0
****************************************************************************/
int main(int argc, char *argv[])
{
    int i;
    int *pid;

    if (argc != 2)
    {
        threads = 2;
    }
    else
    {
        threads = atoi(argv[1]);
    }

    /* initialize package */
    jkthread_init();

    /* Allocate array to store thread process IDs */
    pid = (int *)malloc(threads * sizeof(int));

    if (pid == NULL)
    {
        perror("Allocating pid array");
        exit(0);
    }

    outfile = fopen("jktest.out", "w");
    printf("Main process: %d\n", getpid());
    fprintf(outfile, "Main process: %d\n", getpid());

    /* initialize barrier */
    jkbarrier_init(&barrier);

    /* create and start threads */
    for (i = 0; i < threads; i++)
    {
        pid[i] = jkthread_create(thread, (void *)i, 0, terminate);
        printf("jkthread_create returned %d\n", pid[i]);
        fprintf(outfile, "jkthread_create returned %d\n", pid[i]);
    }

    /* now wait for all the threads to join */
    for (i = 0; i < threads; i++)
    {
        jkthread_join(pid[i]);
    }

    printf("Process %d leaving main\n", getpid());
    fprintf(outfile, "Process %d leaving main\n", getpid());

    jkbarrier_kill(&barrier);
    fclose(outfile);
    free(pid);
    return (0);
}
