/***************************************************************************
*                   Sample Usage of Atomic MLTP Functions
*
*   File    : atomic.c
*   Purpose : Demo usage of mltp atomic functions
*   Author  : Michael Dipperstein
*   Date    : April 27, 2000
*
*   $Id: atomic.c,v 1.2 2000/08/18 14:49:55 mdipper Exp $
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
#include "mltp.h"

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : main
*   Description: This is the only function for this application.  It executes
*                each atomic mltp function and outputs the results.
*                Atomicity is not test by this function, just the operation
*                that is intended to be atomic.
*   Parameters : argc - not used
*                argv - not used
*   Effects    : None
*   Returned   : 0 upon completion
****************************************************************************/
int main (int argc, char **argv)
{
    volatile unsigned myValue;
    int retValue, value;
    mltp_lock_t myLock;

    myValue = 0;

    printf("Test and Set with value = %d\n", myValue);
    retValue = mltp_test_and_set_bit(0, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Test and Set with value = %d\n", myValue);
    retValue = mltp_test_and_set_bit(0, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Test and Clear with value = %d\n", myValue);
    retValue = mltp_test_and_clear_bit(0, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Test and Clear with value = %d\n", myValue);
    retValue = mltp_test_and_clear_bit(0, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    myValue = 0;

    printf("Value = %d\tCompare with 0 and Swap for 1.\n", myValue);
    retValue = mltp_compare_and_swap(0, 1, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Value = %d\tCompare with 1 and Swap for 2.\n", myValue);
    retValue = mltp_compare_and_swap(1, 2, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Value = %d\tCompare with 1 and Swap for 3.\n", myValue);
    retValue = mltp_compare_and_swap(1, 3, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    printf("Value = %d\tCompare with 2 and Swap for 3.\n", myValue);
    retValue = mltp_compare_and_swap(2, 3, &myValue);
    printf("Returned: %d\tValue: %d\n\n", retValue, myValue);

    mltp_lock_init(&myLock, MLTP_LOCK_STD);
    printf("Lock initialized, myLock.now_serving = %u.\n", myLock.now_serving);
    mltp_lock(&myLock);
    printf("Lock acquired, myLock.now_serving = %u.\n", myLock.now_serving);
    mltp_unlock(&myLock);
    printf("Lock released, myLock.now_serving = %u.\n", myLock.now_serving);

    /* do a compare and swap until argc is found */
    if (argc == 2)
    {
        value = atoi(argv[1]);

        for (myValue = 0;
             !(mltp_compare_and_swap(myValue, 0, &value));
             myValue++);
             
        printf("The user entered %d.\n", myValue);
    }

    return(0);
}
