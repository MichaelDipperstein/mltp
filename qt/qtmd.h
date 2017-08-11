//**************************************************************************
//               Intel 386 Assembly Support For QuickThreads
//
//  File    : qtmd.h
//  Purpose : Support QuickThreads on Intel Linux
//  Author  : David Keppel
//  Date    : 1993
//
//  $Id: qtmd.h,v 1.2 2000/08/21 20:17:51 mdipper Exp $
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
#ifndef QT_386_H
#define QT_386_H

typedef unsigned long qt_word_t;

/* Thread's initial stack layout on the i386:

   non-varargs:

   +---
   | arg[2]	=== `userf' on startup
   | arg[1]	=== `pt' on startup
   | arg[0]	=== `pu' on startup
   +---
   | ret pc === qt_error
   +---
   | ret pc	=== `only' on startup
   +---
   | %ebp
   | %esi
   | %edi
   | %ebx				<--- qt_t.sp
   +---

   When a non-varargs thread is started, it ``returns'' directly to
   the client's `only' function.

   varargs:

   +---
   | arg[n-1]
   | ..
   | arg[0]
   +---
   | ret pc	=== `qt_vstart'
   +---
   | %ebp	=== `startup'
   | %esi	=== `cleanup'
   | %edi	=== `pt'
   | %ebx	=== `vuserf'		<--- qt_t.sp
   +---

   When a varargs thread is started, it ``returns'' to the `qt_vstart'
   startup code.  The startup code calls the appropriate functions. */


/* What to do to start a varargs thread running. */
extern void qt_vstart (void);


/* Hold 4 saved regs plus two return pcs (qt_error, qt_start) plus
   three args. */
#define QT_STKBASE	(9 * 4)

/* Hold 4 saved regs plus one return pc (qt_vstart). */
#define QT_VSTKBASE	(5 * 4)


/* Stack must be 4-byte aligned. */
#define QT_STKALIGN	(4)


/* Where to place various arguments. */
#define QT_ONLY_INDEX	(QT_PC)
#define QT_USER_INDEX	(QT_ARG2)
#define QT_ARGT_INDEX	(QT_ARG1)
#define QT_ARGU_INDEX	(QT_ARG0)

#define QT_VSTARTUP_INDEX	(QT_EBP)
#define QT_VUSERF_INDEX		(QT_EBX)
#define QT_VCLEANUP_INDEX	(QT_ESI)
#define QT_VARGT_INDEX		(QT_EDI)


#define QT_EBX	0
#define QT_EDI	1
#define QT_ESI	2
#define QT_EBP	3
#define QT_PC	4
/* The following are defined only for non-varargs. */
#define QT_RPC	5
#define QT_ARG0	6
#define QT_ARG1	7
#define QT_ARG2	8


/* Stack grows down.  The top of the stack is the first thing to
   pop off (preincrement, postdecrement). */
#define QT_GROW_DOWN

extern void qt_error (void);

/* Push on the error return address. */
#define QT_ARGS_MD(sto) \
  (QT_SPUT (sto, QT_RPC, qt_error))


/* When varargs are pushed, allocate space for all the args. */
#define QT_VARGS_MD0(sto, nbytes) \
  ((qt_t *)(((char *)(sto)) - QT_STKROUNDUP(nbytes)))

#define QT_VARGS_MD1(sto) \
  (QT_SPUT (sto, QT_PC, qt_vstart))

#define QT_VARGS_DEFAULT

#endif /* QT_386_H */
