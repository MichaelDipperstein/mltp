#include "jkcthread.h"
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
#define WAIT_FOR_END(X) JoinAllThreads();

/* _jkthread_exit will brute force kill all locks upon exit */
#define LOCKDEC(X)      jksem *(X);
#define LOCKINIT(X)     (X) = jksem_create();
#define LOCK(X)         jksem_get(X);
#define UNLOCK(X)       jksem_release(X);

#define BARDEC(X)       jkbarrier X;
#define BARINIT(X)      jkbarrier_init(&X);
#define BARRIER(X, Y)   jkbarrier_enter(&X, Y);

#define G_MALLOC(X)     malloc(X);
#define CLOCK(X)        Clock(&X);

void MainInit(void);
void MainEnd(void);

void Create(int numThreads, void (*startRoutine)(void *));
void JoinAllThreads(void);

void Clock(unsigned int *time);
