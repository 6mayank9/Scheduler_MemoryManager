#include "libthread.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
//#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
//#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
#define THREADREQ 1
//#include "queue.h"
int mutex;
void my_thread1()
{
    int *i = (int *)(myallocate(sizeof(int),THREADREQ));
    if(i==NULL)
	{  printf("\nOverflow");
	exit(0);
     }	
    for ( *i = 0; *i < 5; ++ *i )
    {
        mutex_lock(&mutex);
        printf( "Hey, I'm my_thread #1: %d\n", *i );
       //sleep(1);                
        mutex_unlock(&mutex);
      // run 3 threads for 50ms eac
    }

}

void fibonacchi()
{
    int *i = (int*)myallocate(sizeof(int),THREADREQ);
    
    int *thread= (int*)myallocate(2*sizeof(int),THREADREQ);
    *thread =0;
    *(thread+1) = 1;
    /*sleep( 2 ); */
    printf( "fibonacchi(0) = 0\nfibonnachi(1) = 1\n" );
    int *nextThread = (int*)myallocate(sizeof(int),THREADREQ);
    for( *i = 2; *i < 15; ++ *i )
    {          
        mutex_lock(&mutex);
        *nextThread = *thread + *(thread+1);
	//sleep(1);        
	printf( "fibonacchi(%d) = %d\n", *i, *nextThread );
        *thread = *(thread+1);
        *(thread+1) = *nextThread;    
        mutex_unlock(&mutex);
     // run 3 threads for 50ms each
 
      
    }
}

void squares()
{
    int *i = myallocate(sizeof(int),THREADREQ);
    
    /*sleep( 5 ); */
    for ( *i = 0; *i < 10; ++ *i )
    {       mutex_lock(&mutex);
        printf( "%d*%d = %d\n", *i, *i, (*i)*(*i) );
	//sleep(1);                
	mutex_unlock(&mutex);
      

    }
}
void scheduler()
{   int i=0,j=0;
    //struct Queue *q1 = createQueue();
    //for(i=0;i<get_numThreads();i++)
        //enQueue(q1, i);
        my_pthread_yield(0);
    while(1)
	{
          i=0;
		for(j=0;j<get_numThreads();j++)
		{
                    if(getActiveStatus(j)==1)
                       i=1;		
	  	}
	  if(i==0)
	     break;
    }
}
int main()
{
    initThreads();
    mutex_init(&mutex);
    my_pthread_create( &my_thread1 );   
    my_pthread_create( &fibonacchi );
    my_pthread_create( &squares );
    waitForAllThreads();
    //my_thread1();
    //fibonacchi();
    //squares();
    /* The program quits */
    return 0;
}
