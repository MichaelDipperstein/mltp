/***************************************************************************
*                 QuickThreads User-Level Threads For MLTP
*
*   File    : qt.c
*   Purpose : Provide user-level threads for mltp
*   Author  : David Keppel
*   Date    : 1993
*
*   $Id: qt.c,v 1.2 2000/08/21 20:17:17 mdipper Exp $
****************************************************************************
*
* Base on original work Copyright (c) 1993 by David Keppel
*
* QuickThreads -- Threads-building toolkit.
* Copyright (c) 1993 by David Keppel
*
* Permission to use, copy, modify and distribute this software and
* its documentation for any purpose and without fee is hereby
* granted, provided that the above copyright notice and this notice
* appear in all copies.  This software is provided as a
* proof-of-concept and for demonstration purposes; there is no
* representation about the suitability of this software for any
* purpose.
*
*
* Modifed and re-issued as part of MLTP
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
#include "qt.h"

#ifdef QT_VARGS_DEFAULT
    
/* If the stack grows down, `vargs' is a pointer to the lowest
   address in the block of arguments.  If the stack grows up, it is a
   pointer to the highest address in the block. */
qt_t *qt_vargs (qt_t *sp, int nbytes, void *vargs,
                void *pt, qt_startup_t *startup,
                qt_vuserf_t *vuserf, qt_cleanup_t *cleanup)
{
    int i;

    sp = QT_VARGS_MD0 (sp, nbytes);
#ifdef QT_GROW_UP
    for (i = nbytes / sizeof(qt_word_t); i > 0; --i)
    {
        QT_SPUT (QT_VARGS_ADJUST(sp), i, ((qt_word_t *)vargs)[-i]);
    }
#else
    for (i = nbytes / sizeof(qt_word_t); i > 0; --i)
    {
        QT_SPUT (QT_VARGS_ADJUST(sp), i - 1, ((qt_word_t *)vargs)[i - 1]);
    }
#endif

    QT_VARGS_MD1 (QT_VADJ(sp));
    QT_SPUT (QT_VADJ(sp), QT_VARGT_INDEX, pt);
    QT_SPUT (QT_VADJ(sp), QT_VSTARTUP_INDEX, startup);
    QT_SPUT (QT_VADJ(sp), QT_VUSERF_INDEX, vuserf);
    QT_SPUT (QT_VADJ(sp), QT_VCLEANUP_INDEX, cleanup);

    return ((qt_t *)QT_VADJ(sp));
}
#endif /* def QT_VARGS_DEFAULT */

void qt_null (void)
{
}

void qt_error (void)
{
    extern void abort(void);
    abort();
}
