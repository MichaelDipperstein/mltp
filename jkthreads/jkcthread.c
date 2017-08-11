/***************************************************************************
*                JKThreads Kerenel Level Threads for MLTP
*
*   File    : jkcthread.c
*   Purpose : MLTP kernel level threads
*   Author  : J.D. Koftinoff
*   Date    : 1996
*
*   $Id: jkcthread.c,v 1.3 2000/08/21 19:58:28 mdipper Exp $
****************************************************************************
*
* Original work Copyright 1996 by J.D. Koftinoff Software Ltd,
* under the KPL (Koftinoff Public License)
*
*   Jeff Koftinoff
*   J.D. Koftinoff Software, Ltd.
*   #5 - 1131 Burnaby St.
*   Vancouver, B.C.
*   Canada
*   V6E-1P3
*
*   email: jeffk@jdkoftinoff.com
*
* This Work has been released for use in and modified to support
* the Multi-Layer Thread Package for SMP Linux
*
* MLTP: Multi-Layer Thread Package for SMP Linux
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <asm/atomic.h>
#include "jkcthread.h"


/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#if 0
#define DBG(XXX)     printf XXX
#else
#define DBG(XXX)
#endif

/* use lock to avoid race conditions on SMP machines */
#ifdef __SMP__
#define LOCK_PREFIX "lock ; "
#else
#define LOCK_PREFIX ""
#endif

#define CSIGNAL         0x000000ff      /* signal mask to be sent at exit */
#define CLONE_VM        0x00000100      /* set if VM shared between processes */
#define CLONE_FS        0x00000200      /* set if fs info shared between processes */
#define CLONE_FILES     0x00000400      /* set if open files shared between processes */
#define CLONE_SIGHAND   0x00000800      /* set if signal handlers shared */


/***************************************************************************
*                             TYPE DEFINITIONS
***************************************************************************/
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun
{
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif


/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/* global data for keeping track of threads and semaphores */
jkthreadinfo _jkthreadinfos[JKMAX_THREADS];
jkthreadinfo _jkdummythread;
jksem _jksems[JKMAX_SEMS];

jksem _jk_sys_sem;
jksem _jk_alloc_sem;
#ifdef __JKFILEIO
jksem _jk_fileio_sem;
#endif

int _jkthread_inited = 0;

jkthreadinfo *_jkthread_kludge[32768];

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
extern int sched_yield(void);

/* used to get around semaphore usage in reenterant fprintf */
extern int _IO_fprintf(_IO_FILE*, const char*, ...);

/* internal routines */
void _jkthread_SIGCHLD (int signum);
void _jkthread_exit(void);
void _jkthread_SIGINT(int);
void _jkthread_SIGTERM(int);
int _linux_thread_create(void (*fn)(void *), void *data, int stacksz);
void _jkthread_starter(void *p);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : __errno_location
*   Description: This function is kludge to get around the non-reentrant
*                errno functions provided by Linux.  It provides a unique
*                error number location for each process based thread.
*   Parameters : None
*   Effects    : None
*   Returned   : Pointer to current thread's errno location
****************************************************************************/
int *__errno_location(void)
{
    return &_jkthread_kludge[getpid()]->thr_errno;
}


/****************************************************************************
*   Function   : __h_errno_location
*   Description: This function is kludge to get around the non-reentrant
*                errno functions provided by Linux.  It provides a unique
*                h error number location for each process based thread.
*   Parameters : None
*   Effects    : None
*   Returned   : Pointer to current thread's h errno location
****************************************************************************/
int *__h_errno_location(void)
{
    return &_jkthread_kludge[getpid()]->thr_h_errno;
}


/****************************************************************************
*   Function   : jkthread_getlocal
*   Description: This function returns a pointer to the calling thread's
*                thread specific memory
*   Parameters : None
*   Effects    : None
*   Returned   : Pointer to current thread's local memory
****************************************************************************/
void *jkthread_getlocal(void)
{
    return _jkthread_kludge[getpid()]->thr_local;
}


/****************************************************************************
*   Function   : jkthread_alloclocal
*   Description: This function allocates a block of thread specific memory.
*                Calling this function after local memory has already been
*                allocated is an error.  To change memory size, use
*                jkthread_getlocal and realloc that memory.
*   Parameters : sz - size (in bytes) of memory to be allocated
*   Effects    : A block of memory is allocates and attached to the thread
*                specific memory area for the calling thread.
*   Returned   : Pointer to current thread's local memory, or NULL
*                when a failure occurs
****************************************************************************/
void *jkthread_alloclocal(size_t sz)
{
    void *ret;

    ret = malloc(sz);
    _jkthread_kludge[getpid()]->thr_local = ret;

    return ret;
}


/****************************************************************************
*   Function   : sleep
*   Description: This is a reentrant version of sleep that uses select() to
*                make thread sleep for the specified number of seconds.
*   Parameters : a - number of seconds to sleep
*   Effects    : Calling thread will sleep for 'a' seconds by calling select
*                on a NULL file descriptor.
*   Returned   : 0
****************************************************************************/
unsigned int sleep(unsigned int a)
{
    struct timeval tv;

    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    tv.tv_sec = a;
    tv.tv_usec=0;

    do
    {
        select(0, 0, 0, 0, &tv);
    } while( tv.tv_sec != 0 || tv.tv_usec != 0 );

    return 0;
}


/****************************************************************************
*   Function   : jkthread_init
*   Description: This function initializes all the variables, structures,
*                and signals used by JKThreads.  It must be called before
*                any other thread function and may not be called more than
*                once per any application.
*   Parameters : None
*   Effects    : Package variables and structures are initialized and
*                signal handlers are installed.
*   Returned   : None
****************************************************************************/
void jkthread_init(void)
{
    int i;

    if(_jkthread_inited)
    {
        return;
    }

    for(i = 0; i < JKMAX_THREADS; ++i)
    {
        _jkthreadinfos[i].thr_id = 0;
        _jkthreadinfos[i].thr_errno = 0;
        _jkthreadinfos[i].thr_h_errno = 0;
    }

    for(i = 0; i < JKMAX_SEMS; ++i)
    {
        _jksems[i].sem_id = 0;
        _jksems[i].usage = 0;
        _jksems[i].owner = 0;
    }

#ifdef __JKFILEIO
    for(i = 0; i < JKMAX_OPEN; ++i)
    {
        _jk_file_sems[i].sem_id = 0;
        _jk_file_sems[i].usage = 0;
        _jk_file_sems[i].owner = 0;
    }
#endif

    for(i = 0; i < 32768; ++i)
    {
        /* dummythread is required so errno works temporarily while we wait
         * for kludge to be updated after thread creation
         */

        _jkthread_kludge[i] = &_jkdummythread;
    }

    _jkthread_inited = 1;

    /* _jk_sys_sem is the semaphore for our thread managment routines */
    jksem_init(&_jk_sys_sem);

    /* _jk_alloc_sem is the semaphore for the non-reentrant libc stuff */
    jksem_init(&_jk_alloc_sem);

#ifdef __JKFILEIO
    /* _jk_stdout_sem is the semaphore for the non-reentrant libio stuff */
    jksem_init(&_jk_fileio_sem);
#endif

    /* The first thread is the main thread */
    _jkthreadinfos[0].thr_id = getpid();

    _jkthread_kludge[getpid()] = &_jkthreadinfos[0];

    atexit(_jkthread_exit);

    signal(SIGINT, _jkthread_SIGINT);
    signal(SIGTERM, _jkthread_SIGTERM);

    {
#if 0
        struct sigaction a;
        a.sa_handler = _jkthread_SIGCHLD;
        a.sa_mask = 0;
        a.sa_flags = SA_RESTART;

        sigaction(SIGCHLD, &a, 0);
#else
        signal(SIGCHLD, _jkthread_SIGCHLD);
#endif
    }

    atexit(_jkthread_exit);
}


/****************************************************************************
*   Function   : jkthread_initcheck
*   Description: This function returns true iff jkthread_init has already
*                been called.
*   Parameters : None
*   Effects    : None
*   Returned   : True if the jkthread_init has already been called,
*                otherwise false
****************************************************************************/
__inline__ int jkthread_initcheck(void)
{
    return _jkthread_inited;
}


/****************************************************************************
*   Function   : _jkthread_exit
*   Description: This function is called at application exit.  It releases
*                all the semaphores held by the thread package.
*   Parameters : None
*   Effects    : All semaphores held by the thread package are released.
*   Returned   : None
****************************************************************************/
void _jkthread_exit(void)
{
    int i;

    /* destroy the _jk_sys_sem */
    jksem_kill(&_jk_sys_sem);

    /* destroy the _jk_alloc_sem */
    jksem_kill(&_jk_alloc_sem);

#ifdef __JKFILEIO
    /* destroy the _jk_fileio_sem */
    jksem_kill(&_jk_fileio_sem);
#endif

    /* destroy all other semaphores */
    for(i = 0; i < JKMAX_SEMS; ++i)
    {
        if(_jksems[i].sem_id != 0)
        {
            jksem_kill(&_jksems[i]);
        }
    }

#ifdef __JKFILEIO
    /* destroy all file semaphores */
    for(i = 0; i < JKMAX_OPEN; ++i)
    {
        if(_jk_file_sems[i].sem_id != 0)
        {
            jksem_kill(&_jk_file_sems[i]);
        }
    }
#endif

    /* kill all threads */
    for(i = 1; i < JKMAX_THREADS; ++i)
    {
        if(_jkthreadinfos[i].thr_id != 0)
        {
            kill(_jkthreadinfos[i].thr_id, SIGTERM);
        }
    }
}


/****************************************************************************
*   Function   : _jkthread_SIGINT
*   Description: This function is executed in the context of the signal
*                handler when a SIGINT (interactive attention) is thrown.
*   Parameters : None
*   Effects    : _jkthread_exit is called
*   Returned   : 0
****************************************************************************/
void _jkthread_SIGINT(int s)
{
    DBG( ("pid %d got SIGINT\n", getpid()) );
    _jkthread_exit();
    exit(0);
}


/****************************************************************************
*   Function   : _jkthread_SIGTERM
*   Description: This function is executed in the context of the signal
*                handler when a SIGTERM (termination request) is thrown.
*   Parameters : None
*   Effects    : _jkthread_exit is called
*   Returned   : 0
****************************************************************************/
void _jkthread_SIGTERM(int s)
{
    DBG( ("pid %d got SIGTERM\n", getpid()) );
    _jkthread_exit();
    exit(0);
}


/****************************************************************************
*   Function   : _jkthread_SIGCHLD
*   Description: This function is executed in the context of the signal
*                handler when a SIGCHLD is thrown do to termination of a
*                child thread.
*   Parameters : signum - unhandled or thrown signal number
*   Effects    : Releases all semaphores held by the child thread and calls
*                a ternination function on behalf of the child thread.
*   Returned   : 0
****************************************************************************/
void _jkthread_SIGCHLD (int signum)
{
    int pid;
    int status;
    int i;
    jkthreadinfo *thr_info;

    while (1)
    {
        pid = waitpid(WAIT_ANY, &status, WNOHANG);
        if (pid < 0)
        {
            break;
        }


        /* %%% combine with previous if */
        if (pid == 0)
        {
            break;
        }


        /* a child exited - we need to reset the kludge array for that PID */
        thr_info = _jkthread_kludge[pid];
        _jkthread_kludge[pid] = &_jkdummythread;

        /* release any semaphores that were owned by dead thread */
        for(i = 0; i < JKMAX_SEMS; ++i)
        {
            if(_jksems[i].sem_id != 0 && _jksems[i].owner == pid)
            {
                /* change ownership of the semphore to us */
                _jksems[i].owner = getpid();

                /* now release it */
                jksem_release(&_jksems[i]);
            }
        }

#ifdef __JKFILEIO
        /* release any files that are locked by dead thread */
        for(i = 0; i < JKMAX_OPEN; ++i)
        {
            if(_jk_file_sems[i].sem_id != 0 && _jk_file_sems[i].owner == pid)
            {
                /* change ownership of the semphore to us */
                _jk_file_sems[i].owner = getpid();

                /* now release it */
                jksem_release(&_jk_file_sems[i]);
            }
        }
#endif

        if (_jk_sys_sem.owner == pid)
        {
            _IO_fprintf(stderr, "sys_sem held by dead thread, pid %d.\n",
                pid);
            /* change ownership of the semphore to us */
            _jk_sys_sem.owner = getpid();

            /* now release it */
            jksem_release(&_jk_sys_sem);
        }

        if (_jk_alloc_sem.owner == pid)
        {
            _IO_fprintf(stderr, "alloc_sem held by dead thread, pid %d.\n",
                pid);
            /* change ownership of the semphore to us */
            _jk_alloc_sem.owner = getpid();

            /* now release it */
            jksem_release(&_jk_alloc_sem);
        }

#ifdef __JKFILEIO
        if (_jk_fileio_sem.owner == pid)
        {
            _IO_fprintf(stderr, "fileio_sem held by dead thread, pid %d.\n",
                pid);
            /* change ownership of the semphore to us */
            _jk_fileio_sem.owner = getpid();

            /* now release it */
            jksem_release(&_jk_fileio_sem);
        }
#endif

        /* now call the termination function, if needed */
        if(thr_info->term_func)
        {
            thr_info->term_func(thr_info->start_param);
        }

        thr_info->thr_id = 0;

        DBG( ("%d: Child %d died\n", getpid(), pid) );
    }

    signal(SIGCHLD, _jkthread_SIGCHLD);
}


/****************************************************************************
*   Function   : _linux_thread_create
*   Description: This function creates a child thread and its stack space.
*   Parameters : fn - main function of child thread
*                data - pointer to data passed to fn
*                stacksz - the stack size for the cloned process
*   Effects    : A new thread process and its stack are allocated.  If
*                successful, the new processes will begin to execute.
*   Returned   : The pid of the new process or -errno if there is an error
****************************************************************************/
int _linux_thread_create(void (*fn)(void *), void *data, int stacksz)
{
    long retval;
    void **newstack;

    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    if (stacksz < JKMIN_STACK)
    {
        stacksz = JKMIN_STACK;
    }

    /*
     * allocate new stack for subthread
     */
    newstack = (void **)malloc(stacksz);
    if (!newstack)
    {
        return -1;
    }

    /*
     * Set up the stack for child function, put the (void *)
     * argument on the stack.
     */
    newstack = (void **)(stacksz + (char *)newstack);
    *--newstack = data;

    /*
     * Do clone() system call. We need to do the low-level stuff
     * entirely in assembly as we're returning with a different
     * stack in the child process and we couldn't otherwise guarantee
     * that the program doesn't use the old stack incorrectly.
     *
     * Parameters to clone() system call:
     *      %eax - __NR_clone, clone system call number
     *      %ebx - clone_flags, bitmap of cloned data
     *      %ecx - new stack pointer for cloned child
     *
     * In this example %ebx is CLONE_VM | CLONE_FS | CLONE_FILES |
     * CLONE_SIGHAND which shares as much as possible between parent
     * and child. (We are in the signal to be sent on child termination
     * into clone_flags: SIGCHLD makes the cloned process work like
     * a "normal" unix child process)
     *
     * The clone() system call returns (in %eax) the pid of the newly
     * cloned process to the parent, and 0 to the cloned process. If
     * an error occurs, the return value will be the negative errno.
     *
     * In the child process, we will do a "jsr" to the requested function
     * and then do a "exit()" system call which will terminate the child.
     */
    __asm__ __volatile__(
        "int $0x80\n\t"         /* Linux/i386 system call */
        "testl %0,%0\n\t"       /* check return value */
        "jne 1f\n\t"            /* jump if parent */
        "call *%3\n\t"          /* start subthread function */
        "movl %2,%0\n\t"
        "int $0x80\n"           /* exit system call: exit subthread */
        "1:\t"
        :"=a" (retval)
        :"0" (__NR_clone), "i" (__NR_exit),
        "r" (fn),
        "b" (CLONE_VM |
             CLONE_FS |
             CLONE_FILES |
             /* uncomment if all threads will use common signal handler */
             /* CLONE_SIGHAND | */
             SIGCHLD),
        "c" (newstack));

    if (retval < 0)
    {
        errno = -retval;
        retval = -1;
    }

    return retval;
}



/****************************************************************************
*   Function   : jkthread_create
*   Description: This function handles all the pre and post creation
*                processing for new thread processes
*   Parameters : fn - main function of child thread
*                data - pointer to data passed to fn
*                stacksz - the stack size for the cloned process
*                term - termination function
*   Effects    : The next available threadinfo for a new thread is made to
*                contain information for a newly created child thread.
*   Returned   : The pid of the new thread or -errno if there is an error
****************************************************************************/
int jkthread_create(void (*fn)(void *), void *data, int stacksz,
                    void (*term_fn)(void*))
{
    int ret;
    int i;
    jkthreadinfo *thread_info = 0;


    if(!_jkthread_inited)
    {
        jkthread_init();
    }


    jksem_get(&_jk_sys_sem);

    for(i = 1; i < JKMAX_THREADS; ++i)
    {
        if(_jkthreadinfos[i].thr_id == 0)
        {
            thread_info = &_jkthreadinfos[i];
            break;
        }
    }

    if(!thread_info)
    {
        /* can't find one. Release the semaphore and fail */
        errno = ENOMEM;
        jksem_release(&_jk_sys_sem);

        DBG( ("Can't find free thread space\n" ) );
        return -1;
    }

    thread_info->start_func = fn;
    thread_info->start_param = data;
    thread_info->term_func = term_fn;
    thread_info->thr_local = NULL;

    ret=_linux_thread_create(_jkthread_starter, thread_info, stacksz);

    if (ret > 0)
    {
        /* we're in parent, update the thread info slot */
        thread_info->thr_id = ret;
        thread_info->thr_errno = 0;
        thread_info->thr_h_errno = 0;

        _jkthread_kludge[ret] = thread_info;

        DBG( ("In Parent, child=%d\n", ret) );
    }


    /* %%% should there be a check for ret != here? */
    jksem_release(&_jk_sys_sem );
    return ret;
}


/****************************************************************************
*   Function   : _jkthread_starter
*   Description: This function is where a new child thread starts.  It sets
*                up the signal handler for the thread and calls the user
*                provided function as an entry point to the thread.
*   Parameters : p - threadinfo with the stating function information for
*                    this thread
*   Effects    : The signal handler for the currecnt thread is set-up and
*                the entry point for this thread is entered.
*   Returned   : None
****************************************************************************/
void _jkthread_starter(void *p)
{
    jkthreadinfo *i = (jkthreadinfo *)p;

    /* wait for original create thread call to finish updating kludge       */
    /* so errno and h_errno and thread local storage will work immediately  */
    /* in the meantime we are using the _jkdummythread space                */
    jksem_get(&_jk_sys_sem);
    jksem_release(&_jk_sys_sem);

    /* set up our signal handlers */
    signal(SIGCHLD, _jkthread_SIGCHLD);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    DBG( ("In child %d, param =0x%08lx\n", getpid(), i->start_param ) );

    i->start_func(i->start_param);

    DBG( ("Child ended %d\n", getpid() ) );
}


/****************************************************************************
*   Function   : jkthread_kil
*   Description: This function kills a process thread
*   Parameters : id - the process id of the thread being killed
*   Effects    : SIGTERM is sent to the thread being killed
*   Returned   : None
****************************************************************************/
void jkthread_kill(pid_t id)
{
    /* free thread-specific data */
    if(_jkthread_kludge[id]->thr_local)
    {
        free(_jkthread_kludge[id]->thr_local);
    }

    /* kill process */
    jksem_get(&_jk_sys_sem);
    kill(id, SIGTERM);
    jksem_release(&_jk_sys_sem);
}


/****************************************************************************
*   Function   : jkthread_join_child
*   Description: This function causes a parent process to sleep until its
*                child terminates.  It works, because a child thread will
*                caues a SIGCHLD to be thrown, this will cause the select to
*                be aborted.
*   Parameters : id - the process id of the thread being joined to
*   Effects    : Current process sleeps until child terminates
*   Returned   : None
****************************************************************************/
void jkthread_join_child(pid_t id)
{
    struct timeval tv;

    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    /* initialize wait time value to 60 seconds just in case we miss it */
    tv.tv_sec = 60;
    tv.tv_usec = 0;

    if(_jkthread_kludge[id] == &_jkdummythread)
    {
        return;     /* Thread id not running */
    }

    /* sleep until signal (usuallay SIGCHLD) or time-out */ 
    do
    {
         select(0, 0, 0, 0, &tv);
         tv.tv_sec = 60;
         tv.tv_usec = 0;
    } while (_jkthread_kludge[id] != &_jkdummythread);
}


/****************************************************************************
*   Function   : jkthread_join_any
*   Description: This function repeatedly yields a process until the process
*                it is attempting to join to terminates.  This is rather
*                expensive because yielding causes the scheduler to
*                scheduilding goodness of all processes, and there is still
*                no assurance that this thread won't come back right away.
*   Parameters : id - the process id of the thread being joined to
*   Effects    : Thread stops execution until joined thread id terminates
*   Returned   : None
*
*   NOTE: future implementations might add a semaphore to the threadinfo
*         for a thread.  A joining thread will then block on that semaphore.
*         The semaphore will be gotten upon thread termination causing the
*         blocked thread to run.  The question then arises as to how you
*         determine when to kill the semaphore.
****************************************************************************/
void jkthread_join_any(pid_t id)
{
    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    /* yield until there is no thread with matching pid */
    while(_jkthread_kludge[id] != &_jkdummythread)
    {
        sched_yield();
    }
}


/****************************************************************************
*   Function   : jksem_create
*   Description: This function creates and initialize a semaphore
*   Parameters : None
*   Effects    : Initializes and creates a semphore with a count of 1
*   Returned   : Pointer to created semaphore
****************************************************************************/
jksem *jksem_create(void)
{

    return jksem_create1(1);
}


/****************************************************************************
*   Function   : jksem_create1
*   Description: This function creates and initialize a semaphore.
*   Parameters : initial_count - value semaphore is initialized to
*   Effects    : Creates a semphore and initializes it with initial_count
*   Returned   : Pointer to created semaphore
****************************************************************************/
jksem *jksem_create1(int initial_count)
{
    jksem *s = 0;
    int i;

    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    jksem_get(&_jk_sys_sem);

    for(i = 0; i < JKMAX_SEMS; ++i)
    {
        if(_jksems[i].sem_id == 0)
        {
            s = &_jksems[i];
            jksem_init1(s, initial_count);
            break;
        }
    }

    jksem_release( &_jk_sys_sem );
    return s;
}


/****************************************************************************
*   Function   : jksem_init
*   Description: This function initialize a semaphore an existing semaphore.
*   Parameters : None
*   Effects    : An existing semaphore is initialized with a count of 1
*   Returned   : None
****************************************************************************/
void jksem_init(jksem *s)
{
    jksem_init1(s, 1);
}


/****************************************************************************
*   Function   : jksem_init1
*   Description: This function initialize a semaphore an existing semaphore.
*   Parameters : initial_count - value semaphore is initialized to
*   Effects    : An existing semaphore is initializes it with initial_count
*   Returned   : None
****************************************************************************/
void jksem_init1(jksem *s, int initial_count)
{
    union semun arg;

    if(!_jkthread_inited)
    {
        jkthread_init();
    }

    s->owner = 0;
    s->usage = 0;

    /* create semaphore */
    s->sem_id = semget(IPC_PRIVATE, 1, 0664 | IPC_CREAT | IPC_EXCL);
    if(s->sem_id == -1)
    {
        perror( "Couldn't create semaphore" );
        exit(1);
    }

    /* set semaphore to initial_count */
    arg.val = initial_count;
    if(semctl(s->sem_id, 0, SETVAL, arg) == -1)
    {
        perror( "Can't set semaphore value");
        exit(1);
    }

}


/****************************************************************************
*   Function   : jksem_kill
*   Description: This function kills (removes) an existing semaphore
*   Parameters : s - pointer to semaphore to be removed
*   Effects    : Semaphore s, is removed and no longer usable
*   Returned   : None
****************************************************************************/
void jksem_kill( jksem *s )
{
    union semun arg;

    /* remove semaphore */
    if(semctl( s->sem_id, IPC_RMID, 0, arg) == -1)
    {
#if 0
        perror ("Can't kill semaphore");
#endif
    }

    s->sem_id = 0;
}


/****************************************************************************
*   Function   : jksem_tryget
*   Description: This function makes a non-blocking attempt to get a
*                semaphore.
*   Parameters : s - pointer to semaphore to be obtained
*   Effects    : If semaphore is obtained, its count will be decremented
*   Returned   : 1 for success, 0 for failure
****************************************************************************/
int jksem_tryget(jksem *s)
{
    struct sembuf sops[1];
    int ret;

    /* try to decrement the semaphore without waiting */
    sops[0].sem_num = 0;
    sops[0].sem_op = -1;
    sops[0].sem_flg = IPC_NOWAIT;

    /* try to decrement the semaphore.  retry if signal interrupts us */
    do
    {
        ret=semop(s->sem_id, sops, 1);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1 && errno == EAGAIN)
    {
        /* we didn't get the semaphore */
        return 0;
    }
    else
    {
        /* ok, we got the semaphore. We Own it now! */
        /* %%% Is there a race condition here? */
        /* %%% what happens to the jkrsem functions */
        s->owner = getpid();
        s->usage = 1;
        return 1;
    }
}


/****************************************************************************
*   Function   : jksem_get
*   Description: This function causes the calling thread to block until the
*                requested semaphore is obtained.
*   Parameters : s - pointer to semaphore to be obtained
*   Effects    : Thread blocks until semaphore is obtained and decrements
*                semaphore's count.
*   Returned   : 1
****************************************************************************/
int jksem_get(jksem *s)
{
    struct sembuf   sops[1];
    int ret;

    /* try to decrement the semaphore waiting */
    sops[0].sem_num = 0;
    sops[0].sem_op = -1;
    sops[0].sem_flg = 0;

    /* try to decrement the semaphore.  retry if signal interrupts us */
    do
    {
        ret = semop(s->sem_id, sops, 1);
    } while (ret == -1 && errno == EINTR);

    /* ok, we got the semaphore. We Own it now! */
    /* %%% Is there a race condition here? */
    /* %%% what happens to the jkrsem functions */
    s->owner = getpid();
    s->usage = 1;

    return 1;
}


/****************************************************************************
*   Function   : jksem_release
*   Description: This function is used by a thread to release a semaphore
*                which it holds.
*   Parameters : s - pointer to semaphore to be released
*   Effects    : If the calling thread owns the semaphore, the semaphore's
*                count will be incremented
*   Returned   : None
****************************************************************************/
void jksem_release(jksem *s)
{
    struct sembuf sops[1];
    int owner = s->owner;
    int ret;

    /* only allow release of semaphores we own or those unowned */
    if ((owner != 0) && (owner != getpid()))
    {
        return;
    }

    /* we dont own it anymore */
    s->owner = 0;

    /* try to increment the semaphore */
    sops[0].sem_num = 0;
    sops[0].sem_op = 1;
    sops[0].sem_flg = 0;

    /* try to increment the semaphore.  retry if signal interrupts us */
    do
    {
        ret = semop(s->sem_id, sops, 1);
    } while (ret == -1 && errno == EINTR);
}


/****************************************************************************
*   Function   : jksem_block
*   Description: This function causes the calling thread to block until the
*                requested semaphore is obtained by another thread (the
*                semaphore's count is 0).
*   Parameters : s - pointer to semaphore used for a block
*   Effects    : Thread blocks until the semaphore's count is 0.
*   Returned   : None
****************************************************************************/
void jksem_block(jksem *s)
{
    struct sembuf   sops[1];
    int ret;

    /* check for 0 operation */
    sops[0].sem_num = 0;
    sops[0].sem_op = 0;
    sops[0].sem_flg = 0;

    /* wait for the semaphore to reach 0.  retry if signal interrupts us */
    do
    {
        ret = semop(s->sem_id, sops, 1);
    } while (ret == -1 && errno == EINTR);
}


/****************************************************************************
*   Function   : jkrsem_tryget
*   Description: This function is similar to jksem_tryget in that it makes a
*                non-blocking attempt to get a semaphore.  The difference is
*                that if the thread already holds the semaphore, the attempt
*                will be successful.
*   Parameters : s - pointer to semaphore to be obtained
*   Effects    : If semaphore is obtained, its count will be decremented,
*                unless it is already owned by the calling thread, then the
*                usage count will be incremented.
*   Returned   : 1 for success, 0 for failure
*
*   NOTE: This is function is not signal safe
****************************************************************************/
int jkrsem_tryget(jksem *s)
{
    if (s->owner == getpid())
    {
        /* we own it. increase usage count */
        s->usage++;
        return 1;
    }
    else
    {
        /* we don't own it, try to get it */
        return jksem_tryget(s);
    }
}


/****************************************************************************
*   Function   : jkrsem_get
*   Description: This function is similar to jksem_try in that it makes a
*                blocking attempt to get a semaphore.  The difference is
*                that if the thread already holds the semaphore, the attempt
*                will be successful.
*   Parameters : s - pointer to semaphore to be obtained
*   Effects    : If semaphore is obtained, its count will be decremented,
*                unless it is already owned by the calling thread, then the
*                usage count will be incremented.
*   Returned   : 1
*
*   NOTE: This is function is not signal safe
****************************************************************************/
int jkrsem_get(jksem *s)
{
    if (s->owner == getpid())
    {
        /* we own it. decrease usage count */
        ++s->usage;
        return 1;
    }
    else
    {
        /* we don't own it, try to get it */
        return jksem_get(s);
    }
}


/****************************************************************************
*   Function   : jkrsem_release
*   Description: This function is used by a thread to release a semaphore
*                which it holds.  Unlike jksem_release, the usage count will
*                be decremented to 0 before the semaphore is released.
*   Parameters : s - pointer to semaphore to be released
*   Effects    : If the calling thread owns the semaphore, the usage count
*                will be decremented.  When the count reaches 0 the
*                semaphore's count will be incremented
*   Returned   : None
*
*   NOTE: This is function is not signal safe
****************************************************************************/
void jkrsem_release(jksem *s)
{
    if (s->owner == getpid())
    {
        /* we own it. decrease usage count */
        if (--s->usage == 0)
        {
            /* we're done with semaphore */
            jksem_release(s);
        }
    }
}


/****************************************************************************
*   Function   : jkbarrier_init
*   Description: This function initializes a barrier structure for usage
*   Parameters : b - barrier to be initialized
*   Effects    : The barrier's counters are set to zero and it's semaphores
*                and locks are created
*   Returned   : None
****************************************************************************/
void jkbarrier_init(jkbarrier *b)
{
    /* zero waiter count and episode */
    b->waiters = 0;
    b->episode = 0;

    /* create lock for barrier */
    b->lock = jksem_create();
    b->barrier[0] = jksem_create1(1);
    b->barrier[1] = jksem_create1(0);
}


/****************************************************************************
*   Function   : jkbarrier_enter
*   Description: This function causes the calling thread to block until all
*                threads have entered the barrier.
*   Parameters : b - barrier to be used
*                count - number of threads to be held at the barrier
*   Effects    : All threads, execpt the last are blocked at the barrier.
*                The last thread releases all blocked threads from the
*                barrier.
*   Returned   : None
****************************************************************************/
void jkbarrier_enter(jkbarrier *b, int count)
{
    int episode;
    
    /* obtain barrier lock */
    jksem_get(b->lock);
    episode = b->episode;

    /* increment number of threads waiting on this barrier */
    b->waiters++;
    
    if (b->waiters == count)
    {
        /* this is the last thread required to enter the barrier */
        /* start new episode */ 
        b->episode = (b->episode + 1) % 2; /* either odd or even */
        b->waiters = 0;

        /* reset other barrier */
        b->barrier[b->episode]->owner = 0;
        jksem_release(b->barrier[b->episode]);

        jksem_release(b->lock); /* let others in */

        /* free all in this barrier */
        jksem_get(b->barrier[episode]);
    }
    else
    {
        jksem_release(b->lock);

        /* wait on barrier until set to zero by last in */
        jksem_block(b->barrier[episode]);
    }
}


/****************************************************************************
*   Function   : jkbarrier_init
*   Description: This function frees a barrier structure's allocations
*   Parameters : b - barrier to be killed
*   Effects    : The barrier's counters are set to zero and it's semaphores
*                and locks are killed
*   Returned   : None
****************************************************************************/
void jkbarrier_kill(jkbarrier *b)
{
    /* zero waiter count and episode */
    b->waiters = 0;
    b->episode = 0;

    /* kill lock for barrier */
    jksem_kill(b->lock);
    jksem_kill(b->barrier[0]);
    jksem_kill(b->barrier[1]);
}
