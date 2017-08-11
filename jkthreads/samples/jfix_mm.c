/***************************************************************************
*          Matrix-to-Matrix Multiplication with Fixed Scheduling
*
*   File    : jdyn_mm.c
*   Purpose : Multiltiply two NxN floating point (double) matricies
*   Author  : Michael Dipperstein
*   Date    : Based on original code by Hong Tang, May. 1999
*
*   $Id: jfix_mm.c,v 1.2 2000/08/21 14:48:31 mdipper Exp $
****************************************************************************
*
*   This program solves the problem A x B = C, where A and B are n x n
*   matricies.  The multiplication is performed by threaded multiplication
*   of sub-matricies.  Each threaded multiplication is scheduled by the
*   thread itself.  For simplicity every element in the matrix A is assigned
*   the vlaue of its column, and every element in the matrix B  is assigned
*   the sum of its column and its row.
*
*   Required parameters are:
*       The size of each matrix n x n
*       The size of each submatrix
*       The number of threads
*
*   NOTE: The sub-matrix size must be an even divisor of n.
*
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "jkcthread.h"

#define MIN(a,b) (((a)>(b))?(b):(a))

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
static int n;        /* size of the matrix */
static int bsize;    /* size of the sub-matrix */
static int barea;    /* ==bsize*bsize */
static int bnum;     /* number of sub-matrix columns/rows in the matrix */
static int nproc;    /* number of POSIX threads */
static int blocks;   /* number of blocks to take */

static double *A, *B, *C;

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : gettime
*   Description: This function gets the current time of day in seconds.  The
*                accuracy is limited to micro seconds or machine resolution
*                gettimeofday().
*   Parameters : None
*   Effects    : None
*   Returned   : Time of day in seconds
****************************************************************************/
double gettime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec+t.tv_usec*0.000001;
}


/****************************************************************************
*   Function   : MAP_BLK_A
*   Description: This function gets the starting address of submatrix A[i][j]
*   Parameters : i - i of submatrix A[i][j]
*                j - j of submatrix A[i][j]
*   Effects    : None
*   Returned   : The starting address of submatrix A[i][j]
****************************************************************************/
double *MAP_BLK_A(int i, int j)
{
    return A+(i*bnum+j)*barea;
}


/****************************************************************************
*   Function   : MAP_BLK_B
*   Description: This function gets the starting address of submatrix B[i][j]
*   Parameters : i - i of submatrix B[i][j]
*                j - j of submatrix B[i][j]
*   Effects    : None
*   Returned   : The starting address of submatrix B[i][j]
****************************************************************************/
double *MAP_BLK_B(int i, int j)
{
    return B+(j*bnum+i)*barea;
}


/****************************************************************************
*   Function   : MAP_BLK_C
*   Description: This function gets the starting address of submatrix C[i][j]
*   Parameters : i - i of submatrix C[i][j]
*                j - j of submatrix C[i][j]
*   Effects    : None
*   Returned   : The starting address of submatrix C[i][j]
****************************************************************************/
double *MAP_BLK_C(int i, int j)
{
    return C+(i*bnum+j)*barea;
}


/****************************************************************************
*   Function   : MAP_ELEM_A
*   Description: This function gets the address of the element A[k][l]
*   Parameters : k - k of submatrix A[k][l]
*                l - l of submatrix A[k][l]
*   Effects    : None
*   Returned   : The starting address of the element A[k][l]
****************************************************************************/
double *MAP_ELEM_A(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_A(bi, bj)+blk_i*bsize+blk_j;
}


/****************************************************************************
*   Function   : MAP_ELEM_B
*   Description: This function gets the address of the element B[k][l]
*   Parameters : k - k of submatrix B[k][l]
*                l - l of submatrix B[k][l]
*   Effects    : None
*   Returned   : The starting address of the element B[k][l]
****************************************************************************/
double *MAP_ELEM_B(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_B(bi, bj)+blk_i*bsize+blk_j;
}


/****************************************************************************
*   Function   : MAP_ELEM_C
*   Description: This function gets the address of the element C[k][l]
*   Parameters : k - k of submatrix C[k][l]
*                l - l of submatrix C[k][l]
*   Effects    : None
*   Returned   : The starting address of the element C[k][l]
****************************************************************************/
double *MAP_ELEM_C(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_C(bi, bj)+blk_i*bsize+blk_j;
}


/****************************************************************************
*   Function   : tid2i
*   Description: This function returns the submatrix i for a task, which may
*                then be used in the MAP_BLK_X functions.
*   Parameters : tid - Task ID
*   Effects    : None
*   Returned   : i of submatrix for task tid
****************************************************************************/
int tid2i(int tid)
{
    return tid/bnum;
}


/****************************************************************************
*   Function   : tid2j
*   Description: This function returns the submatrix j for a task, which may
*                then be used in the MAP_BLK_X functions.
*   Parameters : tid - Task ID
*   Effects    : None
*   Returned   : j of submatrix for task tid
****************************************************************************/
int tid2j(int tid)
{
    return tid%bnum;
}


/****************************************************************************
*   Function   : do_sub_mm
*   Description: This function will multiply bsize submatrices starting at
*                a and b respectively and store the results starting at c.
*   Parameters : c - start of submatrix C[i][j]
*                a - start of submatrix A[i][j]
*                b - start of submatrix B[i][j]
*   Effects    : Address pointed to by C[i][j] = A[i][j] * B[i][j]
*   Returned   : j of submatrix for task tid
****************************************************************************/
void do_sub_mm(double *c, double *a, double *b)
{
    int i, j, k, t1=0;

    /* this is a slight better version than sam_mm.c */
    /* constant propogation and computation strenght reduction */
    /* are performed, just let you have a sense how strange */
    /* the code will look like after optimization. */

    for (i=0; i<bsize; i++)
    {
        /* t1==i*bsize */
        for (j=0; j<bsize; j++)
        {
            int t2=t1+j;
            /* t2==i*bsize+j */
            int t3=j;
            for (k=t1; k<t1+bsize; k++)
            {
                /* t3==j+bsize*(k-t1) */
                c[t2]+=a[k]*b[t3];
                t3+=bsize;
            }
        }
        t1+=bsize;
    }
}


/****************************************************************************
*   Function   : PrintC
*   Description: This function writes the contents of matrix C to stdout.
*   Parameters : None
*   Effects    : The contents of matrix C are written to stdout
*   Returned   : None
****************************************************************************/
void printC()
{
    int i, j;

    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
            printf("%4f  ", *MAP_ELEM_C(i,j));
        printf("\n");
    }
}


/****************************************************************************
*   Function   : do_task_ij
*   Description: This function computes C[i][j], where i and j ar submatrix
*                indicies.
*   Parameters : i - i of C[i][j]
*                j - j of C[i][j]
*   Effects    : C[i][j] will hold the procduct of A[i][-] and B[-][j]
*   Returned   : None
****************************************************************************/
void do_task_ij(int i, int j)
{
    /* computing C[i][j] sub-matrix needs A[i][-] and B[-][j] */
    int k;
    double *C, *A0, *B0;


    C=MAP_BLK_C(i,j);
    A0=MAP_BLK_A(i,0);
    B0=MAP_BLK_B(0,j);

    for (k=0; k<bnum; k++)
    {
        do_sub_mm(C, A0, B0);
        A0+=barea;
        B0+=barea;
    }
}


/****************************************************************************
*   Function   : work
*   Description: This function is the starting point for each thread.  It
*                gets a determines the group of task IDs the thread is
*                responsable and for then calls the routines to perform the
*                multiplication for those tasks.
*   Parameters : thread - current thread id
*   Effects    : Thread will schedule and complete its multiplication tasks
*   Returned   : None
****************************************************************************/
void work(void *thread)
{
    int tid_begin, tid_end, tid;

    tid_begin = blocks * (int)thread;
    tid_end = MIN(tid_begin + blocks - 1, bnum * bnum);

    if (tid_begin >= bnum*bnum)
        return;

    for (tid = tid_begin; tid <= tid_end; tid++)
    {
        int i=tid2i(tid), j=tid2j(tid);
        do_task_ij(i,j);
    }
}


/****************************************************************************
*   Function   : usage
*   Description: This function displays information on using this program.
*   Parameters : prog - name of this program
*   Effects    : Instructions for using this program are written to stdout.
*   Returned   : None
****************************************************************************/
void usage(char *prog)
{
    fprintf(stderr, "\n%s\n\n", prog);

    fprintf(stderr, "This program solves the problem A x B = C,");
    fprintf(stderr, " where A and B are n x n matricies.\n");
    fprintf(stderr, "The multiplication is performed by threaded ");
    fprintf(stderr, "multiplication of sub-matricies.\n");
    fprintf(stderr, "Each threaded multiplication is scheduled by the ");
    fprintf(stderr, "thread itself.\n");

    fprintf(stderr, "\nFor simplicity every element in the matrix A is ");
    fprintf(stderr, "assigned the vlaue of its\n");
    fprintf(stderr, "column, and every element in the matrix B ");
    fprintf(stderr, "is assigned the sum of its column\n");
    fprintf(stderr, "and its row.\n");

    fprintf(stderr, "\nRequired parameters are:\n");
    fprintf(stderr, "\tn\t\tThe size of each matrix n x n\n");
    fprintf(stderr, "\tsize\t\tThe size of each submatrix\n");
    fprintf(stderr, "\tnproc\t\tThe number of threads\n");

    fprintf(stderr,
        "\nNOTE: The sub-matrix size must be an even divisor of n.\n");
}


/****************************************************************************
*   Function   : main
*   Description: This function creates all the data structures and threads
*                required to solve the problem, then it starts the threads.
*                once the calculations are done, results are printed and the
*                program is exited.
*   Parameters : argc - arguement count
*                argv - command line arguements
*   Effects    : Data structures and threads are created
*   Returned   : 0 on success, otherwise failure
****************************************************************************/
int main(int argc, char *argv[])
{
    int i, j;
    double t1, t2;
    int *pid;

    if (argc!=4)
    {
        usage(argv[0]);
        return -1;
    }

    n=atoi(argv[1]);
    bsize=atoi(argv[2]);
    nproc=atoi(argv[3]);

    bnum=n/bsize;
    if (bnum*bsize!=n)
    {
        usage(argv[0]);
        return -1;
    }

    barea=bsize*bsize;
    blocks = ceil((double)(bnum * bnum) / (double)nproc);

    A=(double *)malloc(sizeof(double)*(n*n));
    B=(double *)malloc(sizeof(double)*(n*n));
    C=(double *)malloc(sizeof(double)*(n*n));
    pid = (int *)malloc((nproc - 1) * sizeof(int));

    if (!(A && B && C && pid))
    {
        fprintf(stderr, "Out of Memory.\n");
        return -1;
    }

    /* zero the memory used by C */
    memset((char *)C, 0, sizeof(double)*(n*n));

    printf("Initialization ...\n");

    /* initialize A and B, this is inefficient but easy to understand */
    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
        {
            *MAP_ELEM_A(i,j)=j;
            *MAP_ELEM_B(i,j)=i+j;
        }
    }

    printf("Done!\n\n");
    printf("Multiplication ...\n");

    t1 = gettime();

    /* Make a thread for each process */
    for (i=0; i<nproc-1; i++)
        pid[i] = jkthread_create(work, (void *)i, 0, NULL);

    work((void *)i);

    for (i=0; i<nproc-1; i++)
        jkthread_join(pid[i]);

    t2 = gettime();

    printf("Done!\n\n");

    if (n<=10)
    {
        printf("Printing Matrix C ...\n");
        printC();
        printf("Done!\n");
    }

    printf("Matrix Multiplication takes %4f seconds\n",t2 - t1);

    return 0;
}
