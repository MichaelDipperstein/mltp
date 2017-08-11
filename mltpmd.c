/***************************************************************************
*                   Multi-Level Primitives for Intel 80X86
*
*   File    : mltpmd.c
*   Purpose : 80X86 functions for mltp primitives. (Machine Dependent)
*   Author  : Michael Dipperstein
*   Date    : February 12, 2000
*
*   $Id: mltpmd.c,v 1.2 2000/08/18 14:51:15 mdipper Exp $
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

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "mltpmd.h"

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : mltp_test_and_set_bit
*   Description: This function will atomically set a bit in a word.
*   Parameters : bit  - bit to set (0 = lsb, 31 = msb)
*                addr - pointer to word being changed.
*   Effects    : Specified bit in word will be set to 1.
*   Returned   : Value of bit prior to execution of this function.
****************************************************************************/
__inline__ int mltp_test_and_set_bit(int bit, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btsl %2, %1\n\t"  /* set bit, store old in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit),"=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

/****************************************************************************
*   Function   : mltp_test_and_clear_bit
*   Description: This function will atomically clear a bit in a word.
*   Parameters : bit  - bit to set (0 = lsb, 31 = msb)
*                addr - pointer to word being changed.
*   Effects    : Specified bit in word will be set to 0.
*   Returned   : Value of bit prior to execution of this function.
****************************************************************************/
__inline__ int mltp_test_and_clear_bit(int bit, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btrl %2, %1\n\t"  /* clear bit, store old in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit), "=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

/****************************************************************************
*   Function   : mltp_test_and_change_bit
*   Description: This function will atomically change a bit in a word.
*   Parameters : bit  - bit to set (0 = lsb, 31 = msb)
*                addr - pointer to word being changed.
*   Effects    : Specified bit in word will be set to 0 if it is a 1 and to
*                1 if it is a 0.
*   Returned   : Value of bit prior to execution of this function.
****************************************************************************/
__inline__ int mltp_test_and_change_bit(int bit, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btcl %2, %1\n\t"  /* change bit, store in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit), "=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

/****************************************************************************
*   Function   : mltp_compare_and_swap
*   Description: This function will swap values in a word if the current
*                value matches the value passed as a parameter.
*   Parameters : oldval - what is believed to be the current value.
*                newval - desired value
*                addr - pointer to word being changed.
*   Effects    : Specified word will be set to newval if it is currently
*                equal to oldval.
*   Returned   : 1 for success and 0 for failure.
****************************************************************************/
__inline__ int mltp_compare_and_swap(long oldval, long newval,
                                volatile void *addr)
{
    char ret;

    __asm__ __volatile__(LOCK_PREFIX
                         "cmpxchg %2, %1\n\t"   /* swap values if current value
                                                   is known */
                         "sete %0"              /* read success/fail from EF */
                         : "=q" (ret), "=m" (ADDR)
                         : "r" (newval), "a" (oldval));
    return (int)ret;
}
