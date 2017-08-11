//**************************************************************************
//               Intel 386 Assembly Support For QuickThreads
//
//  File    : qtmds.s
//  Purpose : Support QuickThreads on Intel Linux
//  Author  : David Keppel
//  Date    : 1993
//
//  $Id: qtmds.s,v 1.2 2000/08/21 20:18:05 mdipper Exp $
//**************************************************************************
//
// Base on original work Copyright (c) 1993 by David Keppel
//
// QuickThreads -- Threads-building toolkit.
// Copyright (c) 1993 by David Keppel
//
// Permission to use, copy, modify and distribute this software and
// its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notice and this notice
// appear in all copies.  This software is provided as a
// proof-of-concept and for demonstration purposes; there is no
// representation about the suitability of this software for any
// purpose.
//
//
// Modifed and re-issued as part of MLTP
//
// MLTP: Multi-Layer thread package for SMP Linux
// Copyright (C) 2000 by Michael Dipperstein (mdipper@cs.ucsb.edu)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//**************************************************************************

/* NOTE: double-labeled `_name' and `name' for System V compatability.  */
/* NOTE: Comment lines start with '/*' and '//' ONLY.  Sorry! */

/* Callee-save: %esi, %edi, %ebx, %ebp
// Caller-save: %eax, %ecx
// Can't tell: %edx (seems to work w/o saving it.)
//
// Assignment:
//
// See ``qtmd.h'' for the somewhat unconventional stack layout.  */


    .text
    .align 2

    .globl _qt_abort
    .globl qt_abort
    .globl _qt_block
    .globl qt_block
    .globl _qt_blocki
    .globl qt_blocki

/* These all have the type signature
//
//  void *blocking (helper, arg0, arg1, new)
//
// On procedure entry, the helper is at 4(sp), args at 8(sp) and
// 12(sp) and the new thread's sp at 16(sp).  It *appears* that the
// calling convention for the 8X86 requires the caller to save all
// floating-point registers, this makes our life easy.  */

/* Halt the currently-running thread.  Save it's callee-save regs on
// to the stack, 32 bytes.  Switch to the new stack (next == 16+32(sp))
// and call the user function (f == 4+32(sp) with arguments: old sp
// arg1 (8+32(sp)) and arg2 (12+32(sp)).  When the user function is
// done, restore the new thread's state and return.
//
// `qt_abort' is (currently) an alias for `qt_block' because most of
// the work is shared.  We could save the insns up to `qt_common' by
// replicating, but w/o replicating we need an inital subtract (to
// offset the stack as if it had been a qt_block) and then a jump
// to qt_common.  For the cost of a jump, we might as well just do
// all the work.
//
// The helper function (4(sp)) can return a void* that is returned
// by the call to `qt_blockk{,i}'.  Since we don't touch %eax in
// between, we get that ``for free''.  */

_qt_abort:
qt_abort:
_qt_block:
qt_block:
_qt_blocki:
qt_blocki:
    pushl %ebp          /* Save callee-save, sp-=4. */
    pushl %esi          /* Save callee-save, sp-=4. */
    pushl %edi          /* Save callee-save, sp-=4. */
    pushl %ebx          /* Save callee-save, sp-=4. */
    movl %esp, %eax     /* Remember old stack pointer. */

qt_common:
    movl 32(%esp), %esp /* Move to new thread. */
    pushl 28(%eax)      /* Push arg 2. */
    pushl 24(%eax)      /* Push arg 1. */
    pushl %eax          /* Push arg 0. */
    movl 20(%eax), %ebx /* Get function to call. */
    call *%ebx          /* Call f. */
    addl $12, %esp      /* Pop args. */

    popl %ebx           /* Restore callee-save, sp+=4. */
    popl %edi           /* Restore callee-save, sp+=4. */
    popl %esi           /* Restore callee-save, sp+=4. */
    popl %ebp           /* Restore callee-save, sp+=4. */
    ret                 /* Resume the stopped function. */
    hlt


/* Start a varargs thread. */

    .globl _qt_vstart
    .globl qt_vstart
_qt_vstart:
qt_vstart:
    pushl %edi          /* Push `pt' arg to `startup'. */
    call *%ebp          /* Call `startup'. */
    popl %eax           /* Clean up the stack. */

    call *%ebx          /* Call the user's function. */

    pushl %eax          /* Push return from user's. */
    pushl %edi          /* Push `pt' arg to `cleanup'. */
    call *%esi          /* Call `cleanup'. */

    hlt                 /* `cleanup' never returns. */
