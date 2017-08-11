/***************************************************************************
*                    Multi-layering and User Primitives
*
*   File    : mltp.h
*   Purpose : Multi-Layering and user primitives
*   Author  : Michael Dipperstein
*   Date    : February 19, 2000
*
*   $Id: mltp.h,v 1.4 2000/08/21 20:26:33 mdipper Exp $
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

#ifndef MLTP_H
#define MLTP_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "qt/qt.h"
#include "mltpmd.h"
#include "jkthreads/jkcthread.h"
#include <sched.h>

/***************************************************************************
*                              THREAD TYPES
***************************************************************************/
typedef enum
{
    MLTP_THREAD_UNBOUND,    /* Unbound threads (truely user-level) */
    MLTP_THREAD_BOUND       /* Threads bound to a process */
} mltp_type_t;

/***************************************************************************
*                          THREAD CONTROL TYPES
***************************************************************************/

/***************************************************************************
* The notion of a thread is merged with the notion of a queue.
* Thread stuff: thread status (sp) and stuff to use during
* (re)initialization.
* Queue stuff: next thread in the queue (next).
***************************************************************************/
typedef struct mltp_t
{
    short thrid;            /* Thread Id */
    short state;            /* thread state */
    qt_t *sp;               /* QuickThreads handle */
    void *sto;              /* malloc allocated stack */

    /***********************************************************************
    * The positions of the fields above matters to ensure that the calls to
    * quick thread derived functions will work.  All added fields must be
    * below this comment.
    ***********************************************************************/
    mltp_type_t type;       /* bound or unbound thread */
    void *retval;           /* pointer to the user handle */
    void *private;          /* thread-specific private data area */
    struct mltp_t *next;
} mltp_t;

/***************************************************************************
* Each thread starts by calling a user-supplied function of either one of
* these types.  mltp_userf_t only allows for a single argument to be
* passed, though it is generally quicker.  mltp_vuserf_t functions may have
* multiple arguements and will usually uses vargs to parse parse them.
***************************************************************************/
typedef void *(mltp_userf_t)(void *p0);
typedef void *(mltp_vuserf_t)(int arg0, ...);
typedef void (mltp_buserf_t)(void *p0);

/***************************************************************************
*                           VIRTUAL PROCESSORS
***************************************************************************/

/***************************************************************************
* Data structure local to each virtual processor.  This structure replaces
* the notion of the current gloabl process, with that of a current local
* process for each virtual processor.
***************************************************************************/
typedef struct
{
    int vp_id;
    mltp_t vp_main;     /* main thread for virtual processor */
    mltp_t *vp_curr;    /* thread currntly executing for virtual processor */
} mltp_vp_local_t;


/***************************************************************************
*                           LOCKS AND BARRIERS
***************************************************************************/

/* Lock classes.  BLOCK locks may not be used by bound processes. */
typedef enum
{
    MLTP_LOCK_SPIN,
    MLTP_LOCK_BLOCK,
    MLTP_LOCK_BLOCK_FRONT,
    MLTP_LOCK_SEMAPHORE
} mltp_lock_class_t;

/* Standard lock type. If blocking lock, mltp_qinit must be changed */
#define MLTP_LOCK_STD   MLTP_LOCK_SPIN

/* define if spin locks should do some spinning without bus locking */
#define IDLE_SPIN

typedef struct
{
    volatile unsigned int   next_available; /* next number for waiting */
    volatile unsigned int   now_serving;    /* value required to enter lock */
    jksem                   *sem;           /* semaphore for semaphore lock */
    mltp_lock_class_t       lock_class;     /* class of lock */

    /***********************************************************************
    * NOTE: Blocking locks use the now_serving field to hold lock
    *       lock availability information.
    ***********************************************************************/  
} mltp_lock_t;

typedef struct
{
    volatile unsigned int   waiters;    /* threads currently waiting */
    volatile unsigned int   episode;    /* episode of this barrier */
    mltp_lock_t             lock;       /* prevents miscounts */
} mltp_barrier_t;


/***************************************************************************
*                           QUEUES AND CONDITIONALS
***************************************************************************/

/***************************************************************************
* A queue is a linked list of threads.  The queue head is a designated
* dummy list element.  The threads in the queue chain themselves, a pointer
* to the tail is kept in the queue, so the tail may be easily accessed.
***************************************************************************/
typedef struct
{
    mltp_t t;           /* thread */
    mltp_t *tail;       /* end of queue */
    mltp_lock_t lock;   /* queue lock */
} mltp_q_t;

typedef struct
{
    mltp_q_t q;         /* queue to wait on */
} mltp_cond_t;


/***************************************************************************
* This macro returns a pointer to the thread-specific data area for the
* currently executing thread.
***************************************************************************/
#define mltp_get_private() (mltp_vp_local->vp_curr->private)

/***************************************************************************
* This macro returns the ID of the currently executing thread.
***************************************************************************/
#define mltp_get_myid() (mltp_vp_local->vp_curr->thrid)

/***************************************************************************
*                        THREAD CONTROL FUNCTIONS
***************************************************************************/

/***************************************************************************
* Initialize the thread package.  This function must Call this before any
* other primitives are used.
***************************************************************************/
extern void mltp_init();

/***************************************************************************
* When one or more threads are created by the main thread, the system goes
* multithread when this is called.  When this returns, it is done, there
* are no more runable threads.  The parameter specifies the number of
* kernel threads to be used.
***************************************************************************/
extern void mltp_start(int num_vp);

/***************************************************************************
* Create a thread and make it runable.  When the thread starts running it
* will call `func' with arguments `p0' or `...'.  The thread ID is returned.
*
* mltp_create  - creates a single parameter thread that may execute in the
*                context of any virtual processor.  Threads created by this
*                function do not begin their execution until mltp_start is
*                called.
* mltp_vcreate - like mltp_create, except 'func' may have a variable number
*                of arguements.
* mltp_bcreate - creates a thread that is bound to a process.  Unlike the
*                other threads created bound threads begin their execution
*                after they are created.
***************************************************************************/
extern mltp_t *mltp_create(mltp_userf_t *func, void *p0);
extern mltp_t *mltp_vcreate(mltp_vuserf_t *func, int nbytes, ...);
extern mltp_t *mltp_create_bound(mltp_buserf_t *func, void *p0, int stacksz,
                                 mltp_buserf_t *term);

/***************************************************************************
* Join bound threads with current point of execution.
* NOTE: If join is attempted by an unbound thread the whole virtual process
* for the unbound thread will be blocked.  A special join function may be
* See function source for notes on a how that function could work.
* OTHER NOTE: Current join function may only be called from the parent of
* the bound thread.  A less optimal implementation may be used to allow
* any thread to join to a bound thread.
***************************************************************************/
extern void mltp_join_bound(mltp_t thread);


/***************************************************************************
* The current thread stops running but stays runable.  It is an error to
* call mltp_yield before mltp_start is called or after mltp_start returns.
***************************************************************************/
extern void mltp_yield(void);
#define mltp_yield_bound()      sched_yield()

/***************************************************************************
* The current thread stops running but stays runable and is placed at the
* head of the run queue.  It is an error to call mltp_yield_to_first before
* mltp_start is called or after mltp_start returns.
***************************************************************************/
extern void mltp_yield_to_first(void);

/***************************************************************************
* Like mltp_yield but the thread is discarded.  Any intermediate state is
* lost.  The thread can also terminate by simply returning.
***************************************************************************/
extern void mltp_abort(void);
#define mltp_abort_bound()      return

/***************************************************************************
*                            LOCKING FUNCTIONS
***************************************************************************/

/***************************************************************************
* mltp_lock_init is used to initialize locks.  It must be called prior to
* using a lock.  The same initialization call may be used for any of the
* lock classes (MLTP_LOCK_STD, MLTP_LOCK_SPIN, MLTP_LOCK_BLOCK,
* MLTP_LOCK_BLOCK_FRONT).
*
* NOTE: Only spin locks may be used with bound processes.
***************************************************************************/
extern void mltp_lock_init(mltp_lock_t *lock, mltp_lock_class_t class);

/***************************************************************************
* The class of the lock passed as a parameter will determine the behavior
* of the lock.  Currently only standard and spin locks may be used with
* bound threads.
*
* %%% Need to look in to adding a back off and also a check to make sure
* %%% that unbound threads are not allowed to take part in a block lock.
***************************************************************************/
extern void mltp_lock(mltp_lock_t *lock);

/***************************************************************************
* The class of the lock passed as a parameter will determine the behavior
* of the unlocking.  Currently only standard and spin locks may be used
* with bound threads.
*
* %%% If lock ownership is included, a check must be made to ensure that
* the current thread is the lock's owner.
***************************************************************************/
extern void mltp_unlock(mltp_lock_t *lock);

/***************************************************************************
*                            BARRIER FUNCTIONS
***************************************************************************/

/***************************************************************************
* mltp_barrier_init is used to initialize barriers.  It must be called prior
* to using a barrier.
***************************************************************************/
extern void mltp_barrier_init(mltp_barrier_t *barrier);

/***************************************************************************
* This function is used to synchronize a number of threads insuring that
* they have reached a common point of execution.
***************************************************************************/
extern void mltp_barrier(mltp_barrier_t *barrier, unsigned int count);

/***************************************************************************
*                      CONDITIONAL WAITING FUNCTIONS
***************************************************************************/

/***************************************************************************
* The functions below support conditional waiting.  Only unbound threads may
* wait on a conditional event, however both bound and unbound threads may
* signal or broadcast waiting threads.
***************************************************************************/
extern void mltp_cond_init(mltp_cond_t *cond);
extern void mltp_cond_wait(mltp_cond_t *cond);
extern void mltp_cond_signal(mltp_cond_t *cond);
extern void mltp_cond_broadcast(mltp_cond_t *cond);

#endif /* _MLTP_H */
