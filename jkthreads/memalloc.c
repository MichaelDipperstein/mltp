/***************************************************************************
*                     JKThreads Reenterant Memory Allocation
*
*   File    : memalloc.c
*   Purpose : JKThreads reenterant memory allocation
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
*   $Id: memalloc.c,v 1.2 2000/08/21 19:58:28 mdipper Exp $
****************************************************************************
*
*   This file replaces the standard libc memory allocation functions
*   with a set of thread-safe reenterant wrappers.  At the present time
*   the functions provided use a semaphore to ensure that only one process
*   is in any of these routines at a given time.  Since all the thread
*   processes share the same heap, only one process may manipulate the
*   heap at a time.
*
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
#include "jkcthread.h"

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* libc memory allocation routines */
extern void *__libc_malloc(size_t);
extern void *__libc_calloc(size_t, size_t);
extern void *__libc_realloc(void *, size_t);
extern void __libc_free(void *);
extern void *__libc_memalign (size_t, size_t);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : malloc
*   Description: This function is simply a call to libc's malloc wrapped
*                inside a recursive semaphore to prevent other thread
*                processes from accessing a memory allocation routine at
*                the same time.  This needs to be done, because all
*                processes are manipulating the same heap for their memory
*                allocation.
*   Parameters : sz - size of memory to be allocated
*   Effects    : Memory is allocated from the heap
*   Returned   : Pointer to allocated memory
****************************************************************************/
void *malloc(size_t sz)
{
    void *ret;

    if(!jkthread_initcheck())
    {
        jkthread_init();
    }

    jkrsem_get(&_jk_alloc_sem);

    ret = __libc_malloc(sz);

    jkrsem_release(&_jk_alloc_sem);
    return ret;
}


/****************************************************************************
*   Function   : calloc
*   Description: This function is simply a call to libc's calloc wrapped
*                inside a recursive semaphore to prevent other thread
*                processes from accessing a memory allocation routine at
*                the same time.  This needs to be done, because all
*                processes are manipulating the same heap for their memory
*                allocation.
*   Parameters : a - number of array objects allocated
*                b - size of each object allocated
*   Effects    : Memory is allocated from the heap
*   Returned   : Pointer to allocated memory
****************************************************************************/
void *calloc(size_t a, size_t b)
{
    void * ret;

    if(!jkthread_initcheck())
    {
        jkthread_init();
    }

    jkrsem_get(&_jk_alloc_sem);

    ret = __libc_calloc(a, b);

    jkrsem_release(&_jk_alloc_sem);
    return ret;
}


/****************************************************************************
*   Function   : realloc
*   Description: This function is simply a call to libc's realloc wrapped
*                inside a recursive semaphore to prevent other thread
*                processes from accessing a memory allocation routine at
*                the same time.  This needs to be done, because all
*                processes are manipulating the same heap for their memory
*                allocation.
*   Parameters : p - pointer to existing memory allocation
*                a - size of reallocated object
*   Effects    : Memory is allocated from the heap
*   Returned   : Pointer to reallocated memory
****************************************************************************/
void *realloc(void *p, size_t a)
{
    void *ret;

    if(!jkthread_initcheck())
    {
        jkthread_init();
    }

    jkrsem_get(&_jk_alloc_sem);

    ret = __libc_realloc(p, a);

    jkrsem_release(&_jk_alloc_sem);
    return ret;
}


/****************************************************************************
*   Function   : free
*   Description: This function is simply a call to libc's free wrapped
*                inside a recursive semaphore to prevent other thread
*                processes from accessing a memory allocation routine at
*                the same time.  This needs to be done, because all
*                processes are manipulating the same heap for their memory
*                allocation.
*   Parameters : p - pointer to memory to be returned to the heap
*   Effects    : Memory is allocated from the heap
*   Returned   : none
****************************************************************************/
void free(void *p)
{
    if(!jkthread_initcheck())
    {
        jkthread_init();
    }

    jkrsem_get(&_jk_alloc_sem);

    __libc_free(p);

    jkrsem_release(&_jk_alloc_sem);
}


/****************************************************************************
*   Function   : memalign
*   Description: This function is simply a call to libc's memalign wrapped
*                inside a recursive semaphore to prevent other thread
*                processes from accessing a memory allocation routine at
*                the same time.  This needs to be done, because all
*                processes are manipulating the same heap for their memory
*                allocation.
*   Parameters : alignment - alignment of allocated memory.  Must be a
*                            power of 2.
*                size - number of byte allocated after alignment
*   Effects    : Memory is allocated from the heap
*   Returned   : Pointer to allocated memory
****************************************************************************/
void *memalign(size_t alignment, size_t size)
{
    void *ret;

    if(!jkthread_initcheck)
    {
        jkthread_init();
    }

    jkrsem_get(&_jk_alloc_sem);

    ret = __libc_memalign (alignment, size);

    jkrsem_release(&_jk_alloc_sem);
    return ret;
}
