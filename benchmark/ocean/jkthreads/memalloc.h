/***************************************************************************
*                     JKThreads Reenterant Memory Allocation
*
*   File    : memalloc.h
*   Purpose : Headers for JKThreads reenterant memory allocation
*   Author  : Michael Dipperstein
*   Date    : May 11, 2000
*
***************************************************************************/

/*
 * $Id: memalloc.h,v 1.2 2000/05/17 01:09:34 mdipper Exp mdipper $
 *
 * $Log: memalloc.h,v $
 * Revision 1.2  2000/05/17 01:09:34  mdipper
 * Clean Up
 *
 */

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