/***************************************************************************
*                     JKThreads Reenterant Memory Allocation
*
*   File    : memalloc.h
*   Purpose : Headers for JKThreads reenterant memory allocation
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
*   $Id: memalloc.h,v 1.2 2000/08/21 20:00:07 mdipper Exp $
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

#ifndef __MEMALLOC_H                /* avoid multiple inclusions */
#define __MEMALLOC_H

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
void *memalign (size_t, size_t);

#ifdef __cplusplus
}
#endif

#endif /* def MEMALLOC_H */