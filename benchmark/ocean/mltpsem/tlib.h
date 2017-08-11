#include "mltp.h"
#include <sys/time.h>

/* Replacements for some of the macros used by parmacs */

/* Won't work with CPP #include must manually edit
#define MAIN_ENV        #include "mainenv.h"
*/
#define MAIN_ENV
#define MAIN_INITENV(X, Y)    MainInit();
#define MAIN_END        MainEnd();

/* Won't work with CPP #include must manually edit
#define EXTERN_ENV      #include "extenv.h"
*/
#define EXTERN_ENV

#define CREATE(X)       Create(nprocs, &X);

/* In mltp the main program won't get control until all threads are done */
#define WAIT_FOR_END(X) mltp_start(nvps);

#define LOCKDEC(X)      mltp_lock_t X;
#define LOCKINIT(X)     mltp_lock_init(&X, MLTP_LOCK_SEMAPHORE);
#define LOCK(X)         mltp_lock(&X);
#define UNLOCK(X)       mltp_unlock(&X);

#define BARDEC(X)       mltp_barrier_t X;
#define BARINIT(X)      mltp_barrier_init(&X);
#define BARRIER(X, Y)   mltp_barrier(&X, Y);

#define G_MALLOC(X)     malloc(X);
#define CLOCK(X)        Clock(&X);

void MainInit(void);
void MainEnd(void);

void Create(int numThreads, mltp_userf_t *startRoutine);

void Clock(unsigned int *time);
