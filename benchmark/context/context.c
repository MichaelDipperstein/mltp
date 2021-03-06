/***************************************************************************
*                     Process Context Switch Measurments
*
*   File    : context.c
*   Purpose : measure context switching time for a single.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pcl.h>

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
        sched_yield();
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
    fprintf(stderr, "Usage: %s <yields>\n", program);
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

    Test((void *)yields);

    return(0);
}

