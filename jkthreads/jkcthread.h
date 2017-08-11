/***************************************************************************
*                JKThreads Kerenel Level Threads for MLTP
*
*   File    : jkcthread.h
*   Purpose : Header file for mltp kernel level threads
*   Author  : J.D. Koftinoff
*   Date    : 1996
*
*   $Id: jkcthread.h,v 1.3 2000/08/21 20:00:07 mdipper Exp $
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
* This work has been released for use in and modified to support
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

#ifndef __JKCTHREAD_H
#define __JKCTHREAD_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <linux/limits.h>
#include <asm/atomic.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "memalloc.h"

#ifdef  __JKFILEIO
#include "fileio.h"
#else
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define JKMAX_THREADS   (128)
#define JKMAX_SEMS      (256)
#define JKMAX_OPEN      (OPEN_MAX)
#define JKMAX_MSGQS     (JKMAX_THREADS)
#define JKMIN_STACK     (16384)


/***************************************************************************
*                             TYPE DEFINITIONS
***************************************************************************/
typedef struct _jkthreadinfo
{
    pid_t   thr_id;
    int     thr_errno;
    int     thr_h_errno;
    void    (*start_func)(void *);
    void    (*term_func)(void *);
    void    *start_param;
    void    *thr_local;
} jkthreadinfo;

typedef struct _jksem
{
    volatile int    usage;
    volatile pid_t  owner;
    volatile int    sem_id;
} jksem;

typedef struct _jkbarrier
{
    volatile unsigned int   waiters;    /* threads currently waiting */
    volatile unsigned int   episode;    /* this barrier episode (odd/even) */
    jksem                   *lock;      /* prevents race conditions */
    jksem                   *barrier[2];/* semaphor used as barrier */
} jkbarrier;


/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/* globally visable thread package variables */
extern jkthreadinfo _jkthreadinfos[JKMAX_THREADS];
extern jkthreadinfo _jkdummythread;
extern jksem _jksems[JKMAX_SEMS];
#ifdef __JKFILEIO
extern jksem _jk_file_sems[JKMAX_SEMS];
#endif

/* semaphores used by the thread package */
extern jksem _jk_sys_sem;
extern jksem _jk_alloc_sem;
#ifdef __JKFILEIO
extern jksem _jk_fileio_sem;
#endif


/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/* initializations functions */
void jkthread_init(void);
int jkthread_initcheck(void);

/* thread control functions */
int jkthread_create(void (*fn)(void *), void *data, int stacksz,
                    void (*term_fn)(void *));
void jkthread_kill(pid_t id);
void jkthread_join_child(pid_t id);
void jkthread_join_any(pid_t id);
#define jkthread_join(X)    jkthread_join_child(X)

/* thread specific data */
void *jkthread_getlocal(void);
void *jkthread_alloclocal(size_t sz);

/* semaphore functions */
/* NOTE: recursive semaphores are not signal safe */
jksem *jksem_create(void);
jksem *jksem_create1(int initial_count);
void jksem_init(jksem *s);
void jksem_init1(jksem *s, int initial_count);
void jksem_kill(jksem *s);
int jksem_get(jksem *s);
int jksem_tryget(jksem *s);
void jksem_release(jksem *s);
void jksem_block(jksem *s);  /* block until other thread acquires semaphore */

/* semaphore functions allowing the owner multiple enterances */
int jkrsem_get(jksem *s);
int jkrsem_tryget(jksem *s);
void jkrsem_release(jksem *s);

/* barrier functions */
void jkbarrier_init(jkbarrier *b);
void jkbarrier_enter(jkbarrier *b, int count);
void jkbarrier_kill(jkbarrier *b);

unsigned int sleep(unsigned int);

#ifdef __cplusplus
}
#endif

#endif
