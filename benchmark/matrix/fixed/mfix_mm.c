/**
 * File Name: mfix_mm.c
 * Sample matrix multiplication code using mltp threads and  fixed scheduling.
 *
 * To compile:
 * cc mfix_mm.c -o mfix_mm -lmltp -lqt -ljkt
 *
 * Base on dynamic scheduling code from Hong Tang, May. 1999
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "mltp.h"

double gettime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec+t.tv_usec*0.000001;
}

#define MIN(a,b) (((a)>(b))?(b):(a))

static int n;        /* size of the matrix */
static int bsize;    /* size of the sub-matrix */
static int barea;    /* ==bsize*bsize */
static int bnum;     /* number of sub-matrix columns/rows in the matrix */
static int nproc;    /* number of POSIX threads */
static int blocks;   /* number of blocks to take */

static double *A, *B, *C;

/* get the starting address of submatrix A[i][j] */
double *MAP_BLK_A(int i, int j)
{
    return A+(i*bnum+j)*barea;
}

/* get the starting address of submatrix B[i][j] */
double *MAP_BLK_B(int i, int j)
{
    return B+(j*bnum+i)*barea;
}

/* get the starting address of submatrix C[i][j] */
double *MAP_BLK_C(int i, int j)
{
    return C+(i*bnum+j)*barea;
}

/* get the address of element A[k][l] */
double *MAP_ELEM_A(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_A(bi, bj)+blk_i*bsize+blk_j;
}

/* get the address of element B[k][l] */
double *MAP_ELEM_B(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_B(bi, bj)+blk_i*bsize+blk_j;
}

/* get the address of element C[k][l] */
double *MAP_ELEM_C(int k, int l)
{
    int bi=k/bsize, bj=l/bsize;
    int blk_i=k%bsize, blk_j=l%bsize;

    return MAP_BLK_C(bi, bj)+blk_i*bsize+blk_j;
}

/* the following two functions map a tid to a corresponding (i, j) pair */
int tid2i(int tid)
{
    return tid/bnum;
}

int tid2j(int tid)
{
    return tid%bnum;
}

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

/*
void printA()
{
    int i, j;

    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
            printf("%4f  ", *MAP_ELEM_A(i,j));
        printf("\n");
    }
}

void printB()
{
    int i, j;

    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
            printf("%4f  ", *MAP_ELEM_B(i,j));
        printf("\n");
    }
}
*/

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
    fprintf(stderr, "\tnumvp\t\tThe number of virtual processes\n");

    fprintf(stderr,
        "\nNOTE: The sub-matrix size must be an even divisor of n.\n");
}


int main(int argc, char *argv[])
{
    int i, j, numvp;
    mltp_t **threads;
    double t1, t2;

    if (argc!=5)
    {
        usage(argv[0]);
        return -1;
    }

    n=atoi(argv[1]);
    bsize=atoi(argv[2]);
    nproc=atoi(argv[3]);
    numvp = atoi(argv[4]);

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
    threads = (mltp_t **)malloc(nproc * sizeof(mltp_t *));

    if (!(A && B && C && threads))
    {
        fprintf(stderr, "Out of Memory.\n");
        return -1;
    }

    mltp_init();

    /* zero the memory used by C */
    memset((char *)C, 0, sizeof(double)*(n*n));

//    printf("Initialization ...\n");

    /* initialize A and B, this is inefficient but easy to understand */
    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
        {
            *MAP_ELEM_A(i,j)=j;
            *MAP_ELEM_B(i,j)=i+j;
        }
    }

//    printf("Done!\n\n");
//    printf("Multiplication ...\n");

    t1 = gettime();

    /* Make a thread for each process */
    for (i = 0; i < nproc; i++)
    {
        threads[i] = mltp_create(
            (mltp_userf_t*)work, (void *)i);
    }

    /* Run the threads */
    mltp_start(numvp);
    
    t2 = gettime();

    /* Free the threads */
    for (i = 0; i < nproc; i++)
    {
        free(threads[i]);
    }
    free(threads);

//    printf("Done!\n\n");

    if (n<=10)
    {
        printf("Printing Matrix C ...\n");
        printC();
        printf("Done!\n");
    }

    printf("Matrix Multiplication takes %4f seconds\n",t2 - t1);

    return 0;
}
