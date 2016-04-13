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
    void *c,*e;
    char *d,*f;
    int i;
    c = (void*)myallocate(2000,1);
    if(c==NULL)
    {
        printf("Overflow\n");
    }
    d = (char*)c;
    d = "Thread0-Block1";
    printf("%s\n", d);             
    //mutex_unlock(&mutex);
    e = (void*)myallocate(3000,1); 
    if(e==NULL)
    {
        printf("Overflow\n");
    } 
    f ="Thread0-Block2";
    printf("%s\n", f);    
}

void fibonacchi()
{
    void *c,*e;
    char *d,*f;
    int i;
    c = (void*)myallocate(2000,1);
    if(c==NULL)
    {
        printf("Overflow\n");
    }
    d = (char*)c;
    d = "Thread2-Block1";
    printf("%s\n", d);             
    //mutex_unlock(&mutex);
    e = (void*)myallocate(2000,1); 
    if(e==NULL)
    {
        printf("Overflow\n");
    } 
    f ="Thread2-Block2";
    printf("%s\n", f); 
}

void squares()
{
    void *c,*e;
    char *d,*f;
    int i;
    c = (void*)myallocate(2000,1);
    if(c==NULL)
    {
        printf("Overflow\n");
    }
    d = (char*)c;
    d = "Thread3-Block1";
    printf("%s\n", d);   
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
    my_thread1();
    my_pthread_create( &squares );
    //waitForAllThreads();
    
    //fibonacchi();
    squares();
    /* The program quits */
    return 0;
}
