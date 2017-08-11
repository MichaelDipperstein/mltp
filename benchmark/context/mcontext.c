/***************************************************************************
*                     MLTP Context Switch Measurments
*
*   File    : mcontext.c
*   Purpose : measure context switching time for mltp threads.
*   Author  : Michael Dipperstein
*   Date    : June 8, 2000
*
***************************************************************************/

/*
 * $Id:$
 *
 * $Log:$
 *
 */

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <pcl.h>
#include "mltp.h"

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/
void *Test(void *args)
{
    PCL_CNT_TYPE i_result[2] = {0, 0};
    PCL_FP_CNT_TYPE fp_result[2];
    int counter_list[2], res, yields;
    unsigned int flags;

    /* get arguement */
    yields = (int)args;

    /* define events to be measured */
    counter_list[0] = PCL_INSTR;
    counter_list[1] = PCL_CYCLES;

/* define in what mode to count (only user mode or user and system) */
    if (yields < 0)
    {
        flags = PCL_MODE_USER_SYSTEM;
        yields = -(yields);
    }
    else
    {
        flags = PCL_MODE_USER;
    }

    /* query for functionality */
    if ((res = PCLquery(counter_list, 2, flags)) != PCL_SUCCESS)
    {
        printf("two events not possible\n");
    }
    else
    {
//        printf("Testing context switching from PID %d\n", getpid());

        /* start counting */
        if ((res = PCLstart(counter_list, 2, flags)) != PCL_SUCCESS)
        {
            printf("problem with starting two events\n");
            exit(1);
        }
    }

    while (yields)
    {
        yields--;
        mltp_yield();
    }

    if ((res = PCLstop(i_result, fp_result, 2)) != PCL_SUCCESS)
    {
        printf("problem with stopping two events\n");
    }
    else
    {
        printf("%15.0f instructions in %15.0f cycles\n",
           (double) i_result[0], (double) i_result[1]);
    }

    return(NULL);
}


void ShowUsage(char *program)
{
    fprintf(stderr, "Usage: %s <yields> [U | S]\n", program);
    fprintf(stderr, "\tyeilds is the number of context switching yields\n");
    fprintf(stderr, "\tU or S to force USER or USER_AND_SYSTEM counters\n");
}

/****************************************************************************
*   Function   : main
*   Description: This is the entry point for the mltp context switch expense
*                measuring program.
*   Parameters : argc - argument count
*                argv - arguments
*   Effects    : measures context switching time for mltp threads
*   Returned   : 0 for success, otherwise non-zero
****************************************************************************/
int main(int argc, char *argv[])
{
    mltp_t *thread;
    int yields;

    /* determine number of threads to run */
    if ((argc != 2) && (argc != 3))
    {
        ShowUsage(argv[0]);
        exit(1);
    }
    else
    {
        yields = atoi(argv[1]);

        if (yields < 1)
        {
            ShowUsage(argv[0]);
            exit(1);
        }
        else if (argc == 3)
        {
            if ((argv[2][0] == 's') || (argv[2][0] == 'S'))
            {
                yields =- yields;
            }
        }
    }

    /* create threads */
    mltp_init();

    thread = mltp_create((mltp_userf_t*)Test, (void *)yields);

    /* start threads */
    mltp_start(1);

    /* free thread structures */
    free(thread);

    return(0);
}

