#ifndef LIBFIBER_H
#define LIBFIBER_H 1

#define	LF_NOERROR	0
#define	LF_MAXTHREADS	1
#define LF_MALLOCERROR	2
#define LF_CLONEERROR	3
#define	LF_INFIBER	4
#define LF_SIGNALERROR	5
#define LOCKED 1
#define UNLOCKED 0
typedef int THREADREQ;

/* Define a debugging output macro */
#ifdef LF_DEBUG

#include <stdio.h>
#include <stdlib.h>
#define LF_DEBUG_OUT( string ) fprintf( stderr, "libthread debug: " string "\n")
#define LF_DEBUG_OUT1( string, arg ) fprintf( stderr, "libthread debug: " string "\n", arg )

#else

#define LF_DEBUG_OUT( string )
#define LF_DEBUG_OUT1( string, arg )

#endif

/* The maximum number of threads that can be active at once. */
#define MAX_THREADS 10000
/* The size of the stack for each thread. */
#define FIBER_STACK (1024*1024)


/* Should be called before executing any of the other functions. */
extern void initThreads();

/* Creates a new thread, running the function that is passed as an argument. */
extern int my_pthread_create( void (*func)(void) );

/* Yield control to another execution context. */
extern void my_pthread_yield(int priority);

extern int get_numThreads();
extern void *myallocate(int size,char FILE[],int LINE, int type);
extern void mydeallocate(void *ptr, char FILE[],int LINE,int type);

/* Execute the threads until they all quit. */
extern int waitForAllThreads();

extern int test_and_set(int mutex);
extern void mutex_init(int *mutex);
extern void mutex_lock(int *mutex);
extern void mutex_unlock(int *mutex);
extern void mutex_destroy(int *mutex);

/* Define VALGRIND to include valgrind specific code */
#ifdef VALGRIND
#include <valgrind/valgrind.h>
#else
#define VALGRIND_STACK_REGISTER(start, end) (0)
#define VALGRIND_STACK_DEREGISTER(id)
#endif

#endif
