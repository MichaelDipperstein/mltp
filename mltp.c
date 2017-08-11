/***************************************************************************
*                    Multi-layering and User Primitives
*
*   File    : mltp.c
*   Purpose : Multi-Layering and user primitives
*   Author  : Michael Dipperstein
*   Date    : February 19, 2000
*
*   $Id: mltp.c,v 1.4 2000/08/22 14:01:31 mdipper Exp $
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
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mltp.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define MLTP_STKSIZE        (0x10000)   /* 128Kbyte stack size */
#define MLTP_PRIVATE_SIZE   (1024 * sizeof(void*))

/* stack alignment lifted from stp.  must be a power of 2. */
#define MLTP_STKALIGN(sp, alignment) \
    ((void *)((((qt_word_t)(sp)) + (alignment) - 1) & ~((alignment) - 1)))

/* Round `v' to be `a'-aligned, assuming `a' is a power of two. */
#define ROUND(v, a) (((v) + (a) - 1) & ~((a)-1))

/* thread status */
enum
{
    mltpReady = 0,
    mltpRunning = 1,
    mltpBlock = 2,
    mltpDone = 3
};

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
static mltp_q_t mltp_global_runq;   /* queue of runable threads */

static int thr_num = 0;             /* number of threads created */
static volatile int uthreads = 0;   /* number of user threads alive */
static volatile int num_vps = 0;    /* number of virtual processes alive */

static mltp_lock_t start_lock;      /* prevent re-entering start */
static jksem *mltp_start_sem;       /* zero when all VPs are created */


/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
void *xmalloc(unsigned size);       /* malloc with error checking */

static void mltp_qdump(mltp_q_t *q);

static void mltp_only (void *pu, void *pt, qt_userf_t *f);
static void mltp_thread_start(void *pt);
static void mltp_thread_cleanup(void *pt, void *vuserf_retval);

static void *mltp_starthelp(qt_t *old, void *ignore0, void *ignore1);
static void *mltp_aborthelp(qt_t *sp, void *old, void *null);
static void *mltp_yieldhelp(qt_t *sp, void *old, void *blockq);
static void *mltp_yield_to_first_help(qt_t *sp, void *old, void *blockq);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : xmalloc
*   Description: This function is simply a call to malloc that verifies
*                success, otherwise it calls perror and aborts.
*   Parameters : size - size of memory to be allocated
*   Effects    : Memory is allocated from the heap
*   Returned   : Pointer to allocated memory
****************************************************************************/
void *xmalloc(unsigned size)
{
    void *sto;

    sto = malloc(size);

    if (!sto)
    {
        perror("malloc");
        exit(1);
    }

    return(sto);
}


/****************************************************************************
*   Function   : mltp_lock_init
*   Description: This function initializes the mutual exclusion lock passed
*                as a parameter.
*   Parameters : lock - mutual exclusion lock being initialized
*                class - class of lock being initialized.
*   Effects    : Lock is initialized so that next thread to attempt to obtain
*                will get it.  It is an error to call this routine on locks
*                that are already in use.
*   Returned   : None
*
*   NOTE: Only spin locks may be used with bound threads.
****************************************************************************/
void mltp_lock_init(mltp_lock_t *lock, mltp_lock_class_t class)
{
    lock->next_available = 0;
    lock->now_serving = 0;
    lock->lock_class = class;

    if (class == MLTP_LOCK_SEMAPHORE)
    {
        /* create semaphore */
        lock->sem = jksem_create();
    }
    else
    {
        lock->sem = NULL;
    }
}


/****************************************************************************
*   Function   : mltp_lock
*   Description: This function is used by threads attempting to gain access
*                to a mutual exclusion lock. For spin locks, threads will
*                obtain acces to the lock in the same order that they obtain
*                the next available ticket.  Once a ticket is obtain the
*                thread will spin until it the now waiting value matches the
*                ticket.  Block locks will try get the lock and if they fail,
*                the thread will yield to the end (or head) of the queue,
*                and try once more when it is dequeued.
*                NOTE: These locks do not support recursive acquisition!!!
*   Parameters : lock - mutual exclusion lock
*   Effects    : For spin locks, the lock's next available is incremented and
*                thread spins until the now serving value matches the ticket
*                value.  Block locks will yeild to the head of the queue if
*                a lock is being held.
*   Returned   : None
****************************************************************************/
extern void mltp_lock(mltp_lock_t *lock)
{
    switch (lock->lock_class)
    {
        case MLTP_LOCK_SPIN:
        {
            volatile unsigned int ticket;

            /* get ticket */
            for (ticket = lock->next_available;
                !(mltp_compare_and_swap(ticket, (ticket + 1),
                    &(lock->next_available)));
                ticket = lock->next_available)
#ifdef IDLE_SPIN
            {
                int i = 1 + (int)(10.0 * rand() / (RAND_MAX + 1.0));

                /* spin up to 10 cycles without locking the memory bus */
                while(i)
                {
                    i--;
                }
            }
#endif
                ;

            /* spin until ticket is being served */
            while (ticket != lock->now_serving);
            break;
        }

        case MLTP_LOCK_BLOCK:
        case MLTP_LOCK_BLOCK_FRONT:
            /* block on the run queue if lock is not available */
            while (!mltp_compare_and_swap(0, 1, &(lock->now_serving)))
            {
                if (lock->lock_class == MLTP_LOCK_BLOCK_FRONT)
                {
                    mltp_yield_to_first();
                }
                else
                {
                    mltp_yield();
                }
            }
            break;

        case MLTP_LOCK_SEMAPHORE:
            jksem_get(lock->sem);
            break;

        default:
            fprintf(stderr, "Accessing lock of unknown class.\n");
            break;
    }
}


/****************************************************************************
*   Function   : mltp_unlock
*   Description: This release a lock by incrementing the value of
*                now_serving or setting it to 0 depending on the lock class.
*   Parameters : lock - mutual exclusion lock
*   Effects    : Locks now serving is incremented or zeroed.
*   Returned   : None
****************************************************************************/
extern void mltp_unlock(mltp_lock_t *lock)
{
    switch (lock->lock_class)
    {
        case MLTP_LOCK_SPIN:
            /* let next ticket get served */
            lock->now_serving++;
            break;

        case MLTP_LOCK_BLOCK:
        case MLTP_LOCK_BLOCK_FRONT:
            /* release the lock */
            lock->now_serving = 0;
            break;

        case MLTP_LOCK_SEMAPHORE:
            jksem_release(lock->sem);
            break;

        default:
            fprintf(stderr, "Accessing lock of unknown class.\n");
            break;
    }
}


/****************************************************************************
*   Function   : mltp_qinit
*   Description: This function initializes the thread queue passed as a
*                parameter.
*   Parameters : q - pointer to queue to be initialized.
*   Effects    : Head, tail, and next position in the runqueue all point
*                to each other.
*   Returned   : None
*   NOTE: MLTP_LOCK_BLOCK and MLTP_LOCK_BLOCK_FRONT locks cannot be used on
*         queues, because blocking locks can start a vicious cycle where a
*         thread attempts to acquire a lock and fails, so it then attempts
*         to acquire a lock to the queue it's blocking on which then sends
*         it back to the same queue.
*
*         This code uses MLTP_LOCK_STD.  If MLTP_LOCK_BLOCK* is defined as
*         the standard lock this code will fail.
****************************************************************************/
static void mltp_qinit(mltp_q_t *q)
{
    if ((q->t.thrid == -32768) && (q->tail == &q->t))
    {
        /* unlikely combination just to avoid warning about mltp_qdump */
        mltp_qdump(q);
    }

    q->t.next = q->tail = &q->t;
    q->t.thrid = -32767;            /* give a thread ID for easy tracing */

    if ((MLTP_LOCK_STD != MLTP_LOCK_BLOCK) &&
        (MLTP_LOCK_STD != MLTP_LOCK_BLOCK_FRONT))
    {
        mltp_lock_init(&(q->lock), MLTP_LOCK_STD);
    }
    else
    {
        fprintf(stderr, "Error: Queues may not use BLOCK locks./n");
        fprintf(stderr, "Redefine MLTP_LOCK_STD or edit mltp_qinit.\n");
        exit(0);
    }
}


/****************************************************************************
*   Function   : mltp_qget
*   Description: This function removes the next thread from a queue and
*                returns a pointer to the thread.
*   Parameters : q - pointer to queue to being used.
*   Effects    : Next thread is removed from the queue.
*   Returned   : pointer to thread from queue head.
****************************************************************************/
static mltp_t *mltp_qget(mltp_q_t *q)
{
    mltp_t *t;                       /* next thread in queue */

    mltp_lock(&(q->lock));           /* aquire the queue lock */

    /* get next thread from queue and adjust queue */
    t = q->t.next;
    q->t.next = t->next;

    if (t->next == &q->t)
    {
        if (t == &q->t)
        {
            /* queue was already empty */
            t = NULL;
        }
        else
        {
            /* queue is empty now */
            q->tail = &q->t;
        }
    }

    mltp_unlock(&(q->lock));        /* release the queue lock */

    return t;
}


/****************************************************************************
*   Function   : mltp_qput
*   Thread Type: any
*   Description: This function puts a thread at the end of a queue.
*   Parameters : q - pointer to queue to being used.
*                t - pointer to thread
*   Effects    : Thread t is placed at the end of queue q.
*   Returned   : None
****************************************************************************/
static void mltp_qput(mltp_q_t *q, mltp_t *t)
{
    mltp_lock(&(q->lock));          /* aquire the queue lock */

    q->tail->next = t;
    t->next = &q->t;
    q->tail = t;

    mltp_unlock(&(q->lock));        /* release the queue lock */
}


/****************************************************************************
*   Function   : mltp_qput_second
*   Description: This function puts a thread second in queue.  This is used
*                to allow just one thread to go ahead of this one.
*   Parameters : q - pointer to queue to being used.
*                t - pointer to thread
*   Effects    : Thread t is placed second in queue q, unless there is no
*                first.
*   Returned   : None
****************************************************************************/
static void mltp_qput_second(mltp_q_t *q, mltp_t *t)
{
    mltp_lock(&(q->lock));          /* aquire the queue lock */

    if (q->t.next == &q->t)
    {
        /* queue is empty, it's just a regular put */
        q->tail->next = t;
        t->next = &q->t;
        q->tail = t;
    }
    else
    {
        /* place at second spot in queue */
        t->next = q->t.next->next;
        q->t.next->next = t;

        if (t->next == &q->t)
        {
            /* thread is the new tail */
            q->tail = t;
        }
    }

    mltp_unlock(&(q->lock));        /* release the queue lock */
}


/****************************************************************************
*   Function   : mltp_qdump
*   Description: This function dumps a list of all queued threads to stdout.
*   Parameters : q - pointer to queue being dumped
*   Effects    : A list of queued threads is dumped to stdout.
*   Returned   : None
****************************************************************************/
static void mltp_qdump(mltp_q_t *q)
{
    mltp_t *t;

    mltp_lock(&(q->lock));       /* aquire the queue lock */

    printf("Dump of Global Run Queue:\n");
    t = q->t.next;

    /* traverse queue printing results */
    while (t != &mltp_global_runq.t)
    {
        printf("\tThread %d\n", t->thrid);
        t = t->next;
    }

    mltp_unlock(&(q->lock));     /* release the queue lock */
}


/****************************************************************************
*   Function   : mltp_init
*   Description: This function initializes all the global thread
*                information.  This function must be called prior to thread
*                creation.  It is an error to call this routine while
*                threads are already running.
*   Parameters : None
*   Effects    : Global data is initialized
*   Returned   : None
****************************************************************************/
void mltp_init()
{
    jkthread_init();
    mltp_lock_init(&start_lock, MLTP_LOCK_STD);
    mltp_qinit(&mltp_global_runq);
}


/****************************************************************************
*   Function   : mltp_body
*   Description: This function is called by the virtual processors so that
*                they may execute any runnable threads.  For this initial
*                attempt any vp may run any thread.
*   Parameters : data - the virtural processor Id of this thread
*   Effects    : The virtual process will attempt to run a thread until
*                there are no threads left.
*   Returned   : None
****************************************************************************/
void mltp_body(void *data)
{
    mltp_t *next;
    mltp_vp_local_t *mltp_vp_local;
    volatile int new_vps;

    /* allocate local processor structure */
    mltp_vp_local =
        (mltp_vp_local_t *)jkthread_alloclocal(sizeof(mltp_vp_local_t *));

    if (mltp_vp_local == NULL)
    {
        perror("Failed to allocate local memory for VP");
        exit(1);
    }

    /* save vp_id */
    mltp_vp_local->vp_id = (int)data;

    /* give thread main thread a unique ID for easy tracing */
    mltp_vp_local->vp_main.thrid = -(mltp_vp_local->vp_id) - 1;

    /* wait for all vps to get started. last thread is id 0 */
    if (mltp_vp_local->vp_id != 0)
    {
        jksem_block(mltp_start_sem);
    }
    else
    {
        jksem_get(mltp_start_sem);
    }

    /* execute user level threads */
    for(;;)
    {
        next = mltp_qget(&mltp_global_runq);

        if (next != NULL)
        {
            /* We have a thread to run */
            mltp_vp_local->vp_curr = next;
            QT_BLOCK(mltp_starthelp, 0, 0, next->sp);
        }
        else
        {
            new_vps = num_vps - 1;

            if (new_vps >= uthreads)
            {
                /************************************************************
                * there are too many virtual processors.  Since it's
                * possible that more than one vp been in this section at a
                * time.  It's better to leave a vp running and get it next
                * pass than it is to terminate a vp that should be running.
                ************************************************************/
                if (mltp_compare_and_swap(num_vps, new_vps, &num_vps))
                {
                    /* let this vp die */
                    break;
                }
            }
        }
    }
}


/****************************************************************************
*   Function   : mltp_start
*   Description: This function runs all the threads in the global run queue
*                until there are no more threads left.  It is an error to
*                call this function before mltp_qinit.
*   Parameters : num_vp - number of virtual processes
*   Effects    : Creates the requested number of virtual processes and
*                runs the queued threads on top of them.
*   Returned   : None
****************************************************************************/
void mltp_start(int num_vp)
{
    int *vps;
    int i;

    /* prevent multiple starts*/
    mltp_lock(&start_lock);

    /* allocate handles for each virtual processor */
    vps = (int *)xmalloc(num_vp * sizeof(int));

    /* save the number of virtual processor */
    num_vps = num_vp;

    /* allocate semaphore for signaling start */
    mltp_start_sem = jksem_create();

    /* start a jkthread for each virtual processor */
    for (i = 0; i < num_vp; i++)
    {
        vps[i] = jkthread_create(mltp_body, (void *)(num_vp - i - 1), 0, NULL);
    }

    /* now wait for all the threads to join */
    for (i = 0; i < num_vp; i++)
    {
        jkthread_join(vps[i]);
    }

    /* clean-up */
    free(vps);
    jksem_kill(mltp_start_sem);
    mltp_unlock(&start_lock);
}


/****************************************************************************
*   Function   : mltp_starthelp
*   Description: This function is a helper function which is used for the
*                block which takes place when first starting a the main
*                thread of a vp.
*   Parameters : old - quick threads handle of main thread
*                ignore0 - unused parameter, needed for QT_BLOCK
*                ignore1 - unused parameter, needed for QT_BLOCK
*   Effects    : main thread is assigned to its vp.
*   Returned   : None
****************************************************************************/
static void *mltp_starthelp(qt_t *old, void *ignore0, void *ignore1)
{
    mltp_vp_local_t *mltp_vp_local;

    mltp_vp_local = (mltp_vp_local_t *)jkthread_getlocal();

    /* save the main process for this thread apart from the runq */
    mltp_vp_local->vp_main.sp = old;

    return NULL;
}


/****************************************************************************
*   Function   : mltp_create
*   Description: This function creates a single parameter thread, allocating
*                and initializing it's stack, state, and private storage.
*   Parameters : func - thread's main function
*                p0 - parameter to func
*   Effects    : creates thread and makes it runnable.
*   Returned   : pointer to thread
****************************************************************************/
mltp_t *mltp_create(mltp_userf_t *func, void *p0)
{
    mltp_t *t;
    void *sto;

    t = xmalloc(sizeof(mltp_t));

    /* assign thread next available ID */
    t->thrid = thr_num;
    thr_num++;
    uthreads++;
    t->type = MLTP_THREAD_UNBOUND;
    t->state = mltpReady;

    /* allocate stack */
    t->sto = xmalloc(MLTP_STKSIZE);
    sto = MLTP_STKALIGN(t->sto, QT_STKALIGN);

    /* allocate and zero private memory section */
    t->private = xmalloc(MLTP_PRIVATE_SIZE);
    memset(t->private, 0, MLTP_PRIVATE_SIZE);

    /* push arguments on stack and adjust stack pointer */
    t->sp = QT_SP(sto, MLTP_STKSIZE - QT_STKALIGN);
    t->sp = QT_ARGS(t->sp, p0, t, (qt_userf_t *)func, mltp_only);

    /* queue thread */
    mltp_qput (&mltp_global_runq, t);

    return t;
}


/****************************************************************************
*   Function   : mltp_vcreate
*   Description: This function creates a variable parameter thread,
*                allocating and initializing it's stack, state, and private
*                storage.
*   Parameters : func - thread's main function
*                nbytes - number of bytes of parameter
*   Effects    : creates thread and makes it runnable.
*   Returned   : pointer to thread
****************************************************************************/
mltp_t *mltp_vcreate(mltp_vuserf_t *func, int nbytes, ...)
{
    mltp_t *t;
    va_list ap;
    void *sto;

    t = xmalloc(sizeof(mltp_t));

    /* assign thread next available ID */
    t->thrid = thr_num;
    thr_num++;
    uthreads++;
    t->type = MLTP_THREAD_UNBOUND;
    t->state = mltpReady;

    /* allocate stack */
    t->sto = xmalloc(MLTP_STKSIZE);
    sto = MLTP_STKALIGN(t->sto, QT_STKALIGN);

    /* allocate and zero private memory section */
    t->private = xmalloc(MLTP_PRIVATE_SIZE);
    memset(t->private, 0, MLTP_PRIVATE_SIZE);

    /* adjust stack pointer */
    t->sp = QT_SP(sto, MLTP_STKSIZE - QT_STKALIGN);

    /* get arguements and push them on the stack */
    va_start(ap, nbytes);
    t->sp = QT_VARGS(t->sp, nbytes, ap, t, mltp_thread_start,
                     (qt_vuserf_t *)func, mltp_thread_cleanup);
    va_end(ap);

    mltp_qput(&mltp_global_runq, t);

    return t;
}


/****************************************************************************
*   Function   : mltp_create_bound
*   Description: This function creates a (kernel) thread bound to a process,
*                allocating and initializing it's stack, state, and private
*                storage.
*   Parameters : func - thread's main function
*                p0 - parameter used by func and term
*                stacksz - size of stack
*                term - function called upon thread termination.  This
*                       function runs in the context of the main thread's
*                       signal handler
*   Effects    : creates and starts bound thread.
*   Returned   : pointer to thread.  Only thrid field has any meaning.
****************************************************************************/
mltp_t *mltp_create_bound(mltp_buserf_t *func, void *p0, int stacksz,
                          mltp_buserf_t *term)
{
    mltp_t *t;

    t = (mltp_t *)xmalloc(sizeof(mltp_t));
    t->thrid = jkthread_create(func, p0, stacksz, term);
    t->type = MLTP_THREAD_BOUND;

    return t;
}


/****************************************************************************
*   Function   : mltp_join_bound
*   Description: This function joins a process with a bound thread.
*   Parameters : thread - thread structure containing ID of thread being
*                         joined with.
*   Effects    : Yields calling process until specified bound thread
*                terminates.
*   Returned   : None
*
*   NOTE: This funtion sleeps the calling process.  If it is called by an
*         unbound thread, the VP running the thread will sleep, and not
*         be able to run other threads.  A unbound thread friendly version
*         may be made.  It requires checking the list of active jkthreads
*         and blocking just the unbound thread.  Other tricks can be used
*         where the bound thread's terminate function calls conditional
*         broadcast which release all threads waiting on the join condition.
*   OTHER NOTE: If this function is to be called by a non-parent process,
*         replace jkthread_join with jkthread_join_any.
****************************************************************************/
void mltp_join_bound(mltp_t thread)
{
    jkthread_join(thread.thrid);
}


/****************************************************************************
*   Function   : mltp_only
*   Description: This function makes a thread runable, starts its main task,
*                waits for it's termination and stops the thread.
*   Parameters : func - thread's main function
*                pu - parameter to func
*                pt - pointer to thread
*   Effects    : Runs threads main function.
*   Returned   : This function never returns
****************************************************************************/
static void mltp_only(void *pu, void *pt, qt_userf_t *f)
{

    /* set thread's state to running */
    ((mltp_t*)pt)->state = mltpRunning;

    /* call thread's main function and store return value */
    ((mltp_t*)pt)->retval = (*(mltp_userf_t *)f)(pu);

    /* thread function must exit to get here */
    ((mltp_t*)pt)->state = mltpDone;

     mltp_abort();
    /* abort should replace return */
}


/****************************************************************************
*   Function   : mltp_thread_start
*   Description: This function makes a variable arguement thread runable.
*   Parameters : pt - pointer to thread
*   Effects    : Makes thread runnable
*   Returned   : This function never returns
****************************************************************************/
static void mltp_thread_start(void *pt)
{
    /* set thread's state to running */
    ((mltp_t*)pt)->state = mltpRunning;
}


/****************************************************************************
*   Function   : mltp_thread_cleanup
*   Description: The function waits for a thread to return from its main
*                function, marks it done, and saves the return value.
*   Parameters : pt - pointer to thread
*                vuser_retval - value returned
*   Effects    : handles a thread's termination.
*   Returned   : This function never returns
****************************************************************************/
static void mltp_thread_cleanup(void *pt, void *vuserf_retval)
{
    /* thread function must exit to get here */
    ((mltp_t*)pt)->state = mltpDone;

    /* save the return value */
    ((mltp_t*)pt)->retval = vuserf_retval;

     mltp_abort();
    /* abort should replace return */
}


/****************************************************************************
*   Function   : mltp_abort
*   Description: This function aborts the current thread.
*   Parameters : None
*   Effects    : The current running thread is aborted and the next thread
*                is made the current running thread.
*   Returned   : None
****************************************************************************/
void mltp_abort(void)
{
    mltp_t *old, *mainthread;
    mltp_vp_local_t *mltp_vp_local;
    volatile int newcount;

    mltp_vp_local = (mltp_vp_local_t *)jkthread_getlocal();

    /* make current thread done and switch to main thread */
    mainthread = &(mltp_vp_local->vp_main);
    old = mltp_vp_local->vp_curr;
    old->state = mltpDone;
    mltp_vp_local->vp_curr = mainthread;
    mainthread->state = mltpRunning;

    /* atomically decrement user thread count */
    newcount = uthreads - 1;
    while(!mltp_compare_and_swap(uthreads, newcount, &uthreads))
    {
        newcount = uthreads - 1;
    }

    /* abort old thread */
    QT_ABORT (mltp_aborthelp, old, (void *)NULL, mainthread->sp);
}


/****************************************************************************
*   Function   : mltp_aborthelp
*   Description: This function is a helper function which is used for an
*                abort call.
*   Parameters : sp - quick threads handle of main thread
*                old - the thread being aborted
*                null - unused parameter, needed for QT_ABORT
*   Effects    : old is deallocated
*   Returned   : None
****************************************************************************/
static void *mltp_aborthelp(qt_t *sp, void *old, void *null)
{
    free(((mltp_t *)old)->sto);     /* free stack */
    free(((mltp_t *)old)->private); /* free private section */

    return NULL;
}


/****************************************************************************
*   Function   : mltp_yield
*   Description: This function blocks the current thread.
*   Parameters : None
*   Effects    : The current running thread is blocked and put at the end of
*                the queue and next thread is made the current running
*                thread.
*   Returned   : None
****************************************************************************/
void mltp_yield()
{
    mltp_t *old, *mainthread;
    mltp_vp_local_t *mltp_vp_local;

    mltp_vp_local = (mltp_vp_local_t *)jkthread_getlocal();

    /* make current thread done and switch to main thread */
    mainthread = &(mltp_vp_local->vp_main);
    old = mltp_vp_local->vp_curr;
    old->state = mltpBlock;
    mltp_vp_local->vp_curr = mainthread;
    mainthread->state = mltpRunning;

    /* block old thread */
    QT_BLOCK(mltp_yieldhelp, old, &mltp_global_runq, mainthread->sp);
}


/****************************************************************************
*   Function   : mltp_yieldhelp
*   Description: This function handles the requeuing and stack save of a
*                yielding thread.
*   Parameters : None
*   Effects    : The yielding thread is placed at the end of the running
*                queue and its stack pointer is saved.
*   Returned   : None
****************************************************************************/
static void *mltp_yieldhelp(qt_t *sp, void *old, void *blockq)
{
  ((mltp_t *)old)->sp = sp;
  mltp_qput((mltp_q_t *)blockq, (mltp_t *)old);
  return (old);
}


/****************************************************************************
*   Function   : mltp_yield_to_first
*   Description: This function blocks the current thread putting it at the
*                front of the queue.
*   Parameters : None
*   Effects    : The current running thread is blocked and put at the head
*                of the queue and next thread is made the current running
*                thread.
*   Returned   : None
****************************************************************************/
void mltp_yield_to_first()
{
    mltp_t *old, *mainthread;
    mltp_vp_local_t *mltp_vp_local;

    mltp_vp_local = (mltp_vp_local_t *)jkthread_getlocal();

    /* make current thread done and switch to main thread */
    mainthread = &(mltp_vp_local->vp_main);
    old = mltp_vp_local->vp_curr;
    old->state = mltpBlock;
    mltp_vp_local->vp_curr = mainthread;
    mainthread->state = mltpRunning;

    /* block old thread */
    QT_BLOCK(mltp_yield_to_first_help, old, &mltp_global_runq,
        mainthread->sp);
}


/****************************************************************************
*   Function   : mltp_yield_to_first_help
*   Description: This function handles the requeuing and stack save of a
*                yielding thread.
*   Parameters : None
*   Effects    : The yielding thread is placed at the head of the running
*                queue and its stack pointer is saved.
*   Returned   : None
****************************************************************************/
static void *mltp_yield_to_first_help(qt_t *sp, void *old, void *blockq)
{
  ((mltp_t *)old)->sp = sp;
  mltp_qput_second((mltp_q_t *)blockq, (mltp_t *)old);
  return (old);
}


/****************************************************************************
*   Function   : mltp_barrier_init
*   Description: This function must be called prior to the first use of a
*                specific barrier.  It insures that the barrier will
*                function properly when used.
*   Parameters : barrier - barrier to be initialized
*   Effects    : zeros out all barrier fields
*   Returned   : None
****************************************************************************/
void mltp_barrier_init(mltp_barrier_t *barrier)
{
    barrier->waiters = 0;
    barrier->episode = 0;
    mltp_lock_init(&(barrier->lock), MLTP_LOCK_STD);
}


/****************************************************************************
*   Function   : mltp_barrier
*   Description: This function provides the functionality of a barrier
*                primative.  It blocks all threads that enter it, until the
*                specified number have entered it.
*   Parameters : barrier - barrier structure to block on
*                count - number of threads required to enter the barrier
*                        this episode.
*   Effects    : Blocks all threads that enter until the specified number
*                have entered the barrier.  barrier->episode will be
*                incremented when the last thread enters the barrier.
*   Returned   : None
****************************************************************************/
void mltp_barrier(mltp_barrier_t *barrier, unsigned int count)
{
    int episode;

    /* obtain barrier lock */
    mltp_lock(&(barrier->lock));    /* may want to make this a spin lock */
    episode = barrier->episode;

    /* increment number of threads waiting on this barrier */
    barrier->waiters++;

    if (barrier->waiters == count)
    {
        /* this is the last thread required to enter the barrier */
        /* start new episode */
        barrier->episode++;
        barrier->waiters = 0;

        mltp_unlock(&(barrier->lock));
    }
    else
    {
        mltp_unlock(&(barrier->lock));

        /* wait until each thread hits the barrier */
        while (episode == barrier->episode)
        {
            mltp_yield();
        }
    }
}


/****************************************************************************
*   Function   : mltp_cond_init
*   Description: This function initializes the queues used for conditional
*                waiting.
*   Parameters : cond - queue for threads waiting on a condition
*   Effects    : Head, tail, and next position in the conditional queue all
*                point to each other, and conditional lock is initialized.
*   Returned   : None
****************************************************************************/
void mltp_cond_init(mltp_cond_t *cond)
{
    mltp_qinit(&(cond->q));
}


/****************************************************************************
*   Function   : mltp_cond_wait
*   Description: This function places the currently executing user level
*                thread in a conditional wait queue where it will stay until
*                a signal has been provided to the conditional queue.  This
*                function is only valid when called from an unbound thread.
*   Parameters : cond - queue for threads waiting on a condition
*   Effects    : Currently executing user thread is blocked.  The next
*                runnable thread will take over use of executing VP.
*   Returned   : None
****************************************************************************/
void mltp_cond_wait(mltp_cond_t *cond)
{
    mltp_t *old, *mainthread;
    mltp_vp_local_t *mltp_vp_local;

    mltp_vp_local = (mltp_vp_local_t *)jkthread_getlocal();

    /* make current thread done and switch to main thread */
    mainthread = &(mltp_vp_local->vp_main);
    old = mltp_vp_local->vp_curr;
    old->state = mltpBlock;
    mltp_vp_local->vp_curr = mainthread;
    mainthread->state = mltpRunning;

    /* block old thread */
    QT_BLOCK(mltp_yieldhelp, old, &(cond->q), mainthread->sp);
}


/****************************************************************************
*   Function   : mltp_cond_signal
*   Description: This function signals that the first user level thread in a
*                conditional wait queue is runnable.
*   Parameters : cond - queue for threads waiting on a condition
*   Effects    : The head of the contional queue will be placed on the
*                global run queue.
*   Returned   : None
****************************************************************************/
void mltp_cond_signal(mltp_cond_t *cond)
{
    mltp_t *t;                       /* thread from conditional */

    /* get next thread from conditional queue */
    t = mltp_qget(&(cond->q));

    if (t != NULL)
    {
        mltp_qput(&mltp_global_runq, t);
    }
}


/****************************************************************************
*   Function   : mltp_cond_broadcast
*   Description: This function signals that all the user level threads in a
*                conditional wait queue are runnable.
*   Parameters : cond - queue for threads waiting on a condition
*   Effects    : All of the threads in the contional queue will be placed on
*                the global run queue.
*   Returned   : None
****************************************************************************/
void mltp_cond_broadcast(mltp_cond_t *cond)
{
    mltp_t *thread, *tail;                  /* head and tail of cond queue */

    /* lock cond and remove all threads */
    mltp_lock(&(cond->q.lock));             /* aquire the cond lock */

    /* head of queue and adjust queue mark queue empty */
    thread = cond->q.t.next;
    tail = cond->q.tail;
    cond->q.t.next = cond->q.tail = &(cond->q.t);

    mltp_unlock(&(cond->q.lock));           /* release the cond lock */

    /* now put all these threads in the run queue */
    mltp_lock(&mltp_global_runq.lock);      /* aquire run queue lock */

    /* add conditionals to the head of the queue */
    mltp_global_runq.tail->next = thread;
    tail->next = &mltp_global_runq.t;
    mltp_global_runq.tail = tail;

    mltp_unlock(&mltp_global_runq.lock);    /* release run queue lock */
}
