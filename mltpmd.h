/***************************************************************************
*                   Multi-Level Primitives for Intel 80X86
*
*   File    : mltpmd.h
*   Purpose : 80X86 GCC headers for mltp primitives (Machine Dependent)
*   Author  : Michael Dipperstein
*   Date    : February 12, 2000
*
*   $Id: mltpmd.h,v 1.2 2000/08/18 14:51:39 mdipper Exp $
****************************************************************************
*   NOTE: Many of the functions here have been modified from asm/bitops.h
*   distributed with Linux and containing the following copyright notice.
*   "Copyright 1992, Linus Torvalds."
*
*   All operations are performed on longwords.  For all bit operations bit
*   0 is the LSB and bit 31 is the MSB.
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

#ifndef _I386_MLTP_H
#define _I386_MLTP_H

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/

/***************************************************************************
*                        CONFIGURATION INFORMATION
***************************************************************************/

/* use lock to avoid race conditions on SMP machines */
#ifdef __SMP__
#define LOCK_PREFIX "lock ; "
#else
#define LOCK_PREFIX ""
#endif

/***************************************************************************
*                            ATOMIC FUNCTIONS
***************************************************************************/

/* function prototypes to keep gcc -Wall happy */
extern int mltp_test_and_set_bit(int bit, volatile void *addr);
extern int mltp_test_and_clear_bit(int bit, volatile void *addr);
extern int mltp_test_and_change_bit(int bit, volatile void *addr);
extern int mltp_compare_and_swap(long oldval, long newval, volatile void *addr);


/* some hacks to defeat gcc over-optimizations */
struct __dummy { unsigned long a[100]; };
#define ADDR (*(volatile struct __dummy *)addr)

#endif /* _I386_MLTP_H */
