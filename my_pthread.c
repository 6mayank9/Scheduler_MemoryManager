/* On Mac OS X, _XOPEN_SOURCE must be defined before including ucontext.h.
Otherwise, getcontext/swapcontext causes memory corruption. See:

http://lists.apple.com/archives/darwin-dev/2008/Jan/msg00229.html */
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif
#include <signal.h>
#include "libthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
char c[1024*1024*8];
char *memory = c;
int start_index = 0;
int current_index = 0;

char d[1024*1024*16];
char *swap_memory = d;
int start_index_swap = 0;
int current_index_swap = 0;
int i =0;


/* The Thread Structure
*  Contains the information about individual threads.
*/
typedef struct
{
	ucontext_t context; /* Stores the current context */
	int active; /* A boolean flag, 0 if it is not active, 1 if it is */
	/* Original stack pointer. On Mac OS X, stack_t.ss_sp is changed. */
	void* stack; 
	int priority;
	void *start_address;
	void *end_address;
	int current_index;
	int swapped;
	int page_count;
	void *start_address_swap;
	void *end_address_swap;
} myThread;
/* The thread "queue" */
static myThread threadList[ MAX_THREADS ];

/* The index of the currently executing thread----change it to -1 once tested */
static int currentThread = 0;
/* A boolean flag indicating if we are in the main process or if we are in a thread */
static int inThread = 0;
/* The number of active threads */
static int numThreads = 0;
/* The number of active threads with priority 0*/
static int num_high_priority_threads = 0;
/* The number of active threads with priority 1*/
static int num_low_priority_threads = 0;

/* The "main" execution context */
static ucontext_t mainContext;
static int alarm_count = 0;

/*Structure for Malloc*/
struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
  int owner_thread;
};

void *global_base = NULL;
struct block_meta *temp;
void *tempSwap ;

#define META_SIZE sizeof(struct block_meta)

void *getswapmemory(int size)
{
	int i,j,position,m=0;
	long int startadd[numThreads],endadd[numThreads],c=0;
	long int temp;
	//printf("%p %lu\n",swap_memory, (long int)swap_memory );
	for(i=0;i<numThreads;i++)
	{ 
		if(threadList[i].swapped == 1)
		{ m=1;
          startadd[c] = (long int)threadList[i].start_address_swap;
          //printf("startadd: %lu\n",startadd[c]);
          endadd[c] = (long int)threadList[i].end_address_swap;
          //printf("endadd: %lu\n",endadd[c] );
          c++;
		}
	}
	temp= startadd[0];
    for(i=0;i<c;i++)
   	{ 
   		position = i;
       for(j=i+1;j<c;j++)
       {
       	if(startadd[j]<startadd[position])
       	{
       		position = j;
       	}
       }
       if(position!=i)
       {
       	temp = startadd[i];
       	startadd[i] = startadd[position];
       	startadd[position] = temp;
       }
   }
  
   if(startadd[0] - (long int)swap_memory >= (long int)size || m==0 )
   	return (void *)swap_memory;
   startadd[c] = (long int)swap_memory +(1024*1024*16);
   c++;
  for(i=0;i<c;i++)
  {
    if(startadd[i+1] - endadd[i]>(long int)size)
    	return ((void *)endadd[i]+1);
  }
return NULL;
}


int swapout()
{
	void *dest;
	struct block_meta *temp;
	int size;
	int thread;
	thread = threadToBeSwapped();
	if(thread == -1)
		return 0;
	size = threadList[thread].end_address - threadList[thread].start_address;
	dest = getswapmemory(size);
	//printf("getswapmemory: %p\n",dest);
	if(dest == NULL)
		return 0;
	memcpy(dest, threadList[thread].start_address, size);
	threadList[thread].swapped = 1;
	threadList[thread].start_address_swap = dest;
	//printf("start_address_swap: %lu \n",(long int)threadList[thread].start_address_swap );
	threadList[thread].end_address_swap = dest + size;
    //printf("end_address_swap: %lu \n",(long int)threadList[thread].end_address_swap );
	current_index_swap = current_index_swap + size;
	temp = global_base;
    while(temp != NULL) 
    {
    
       	if(temp->owner_thread == thread)
    	temp->free = 1;
       	temp = temp->next;
    }
	printf("\nThread %d swapped out\n", thread);
   return 1;
}

int threadToBeSwapped()
{
	struct block_meta *block = global_base;
	while(block!=NULL && block->owner_thread == currentThread && block->free == 0)
		block = block->next;
	if(block == NULL)return -1;
	return block->owner_thread;

}

int swapin()
{
	printf("Swap In called\n");
	int size;
	int thread;
    tempSwap = swap_memory+((1024*1024*16) - 4096);
	size = threadList[currentThread].end_address - threadList[currentThread].start_address;
	
	//printf("\ntempswap address:%p\n", &tempSwap);

	memcpy (tempSwap,threadList[currentThread].start_address,size); // block at C copied to tempSwap memory
	
	thread = findowner();
	if (thread == -1)
	{
		printf("\n Nothing to swap back in");
		return -1;
	}
	
	threadList[thread].swapped = 1;
	memcpy (threadList[thread].start_address,threadList[currentThread].start_address_swap,size); // swapping back in
	threadList[currentThread].swapped = 0;
	memcpy (threadList[currentThread].start_address_swap,tempSwap,size); //copying tempSwap to swap memory

	printf("\nThread %d swapped back in\n",currentThread);
	return 1;
}

int findowner()
{
	int i=0;
	for(i=0;i<numThreads;i++)
	{
		if(threadList[i].end_address == threadList[currentThread].end_address)
			return i;
	}
	return -1;
}

void *sbrk1(int nbytes)
{	
	void *base;
	int index;
	if(nbytes==0)
		return (void*)memory;
	if((current_index+nbytes)>8388608)
		return NULL;
	index = current_index;
	current_index = current_index + nbytes;
	base = (void*)(memory+index);
	return base;
}
struct block_meta *find_free_block(struct block_meta **last, size_t size) {
  struct block_meta *current = global_base;
  while (current != NULL) 
  {
    *last = current;
    if((*last)->free == 1)
    	break;
    current = current->next;
  }
  return current;
}

struct block_meta *request_space(struct block_meta* last, size_t size) {
  struct block_meta *block;
  block = (struct block_meta *)sbrk1(size + META_SIZE);
  if (block == NULL) {
    return NULL; // sbrk failed.
  }

  if (last) { // NULL on first request.
    last->next = block;
  }
  block->size = size;
  block->next = NULL;
  block->free = 0;
  return block;
}

struct block_meta *get_block_ptr(void *ptr) {
  return (struct block_meta*)ptr - 1;
}
void *myallocate_thread(int size)
{	
	void *base;int index;
	struct block_meta *block, *temp;
	struct block_meta *current = global_base;
	if(size+threadList[currentThread].current_index>4095)
	{
  		block = (struct block_meta *)sbrk1(4095 + META_SIZE);
  		if(block == NULL)
  			return NULL;
  		block->size = 4095;
		block->next = NULL;
		block->free = 0;
		block->owner_thread = currentThread;
		while(current->owner_thread != currentThread)
			current = current->next;
		temp = current->next;
		while(temp!=NULL && temp->owner_thread == currentThread)
		{
			current = temp;
			temp = temp->next;
		}
		current->next = block;
		block->next = temp;
		threadList[currentThread].end_address = (void *)(block + 1) + 4095;
		threadList[currentThread].current_index = 0;
	}
	index = threadList[currentThread].current_index;
	threadList[currentThread].current_index = threadList[currentThread].current_index + size;
	base = (void*)(threadList[currentThread].end_address + index - 4095);
	//base = (void *)(threadList[currentThread].start_address);
	return base;

}
void *myallocate_self(int size)
{	
	struct block_meta *block;
	struct block_meta *last = global_base;
	
	
	
	if (!global_base) 
	{ // First call.
	   
	   	block = request_space(NULL, size);
	    if (!block) 
	    {
	      return NULL;
	    }
	    global_base = block;
    } 
  else {
    block = find_free_block(&last, size);
    if (!block) 
    { // Failed to find free block.
      block = request_space(last, size);
      if (!block) 
      {
        return NULL;
      }
    } 
    else 
    {      // Found free block
      // TODO: consider splitting block here.
      block->free = 0;
    }
  }
  return block;
}
void *myallocate(int size,char FILE[],int LINE, int type) {
  
  struct block_meta *block;
  int i;
  if (size <= 0 || size >= 4096) {
    return NULL;
  }
  if(type)
  {
  	block = myallocate_thread(size);
  	return block;
  }
  else
  {
  	printf("Size requeted: %d\n",size );
  	block = myallocate_self(size);
  	if(block==NULL)
  	{	
  		printf("Entered\n");
  		if(current_index_swap + size > ((1024*1024*16)-4096))
  			return NULL;
  		else
  		{
	  		if(swapout())
	  		{
	  			block = myallocate(size,__FILE__,__LINE__, type);
	  			block->owner_thread = numThreads;
	  			return block;
	  		}
	  		else
	  			return NULL;
  		}
  	}
  }
  block->owner_thread = numThreads;
  return(block + 1);
}

void mydeallocate(void *ptr, char FILE[],int LINE,int type) {
  if (!ptr) {
    return;
  }

  // TODO: consider merging blocks once splitting blocks is implemented.
  struct block_meta* block_ptr = get_block_ptr(ptr);
  assert(block_ptr->free == 0);
  block_ptr->free = 1;
}


/* Sets all the threads to be initially inactive */
void alarmhandler(int signum)
{
	
	
int d;
alarm_count++;
d =alarm_count%10;
if(d<5)    
{
	//printf("Time slice %d running for Q1\n", d);
	my_pthread_yield(0);
}   
else if(d%2==1)
{		
		//printf("Time slice %d running for Q2\n", d/3);
		if(d==9)alarm_count=0;
		my_pthread_yield(1);
}

}

void initThreads()
{
	int i;
	for ( i = 0; i < MAX_THREADS; ++ i )
	{
		threadList[i].active = 0;
		threadList[i].priority = 0;
		threadList[i].swapped = 0;
		threadList[i].page_count = 0;
	}
		
	return;
}

int getActiveStatus(int id)
{
return threadList[id].active;
}
/* Records when the thread has started and when it is done
so that we know when to free its resources. It is called in the thread's
context of execution. */
static void threadStart( void (*func)(void) )
{
	threadList[currentThread].active = 1;
	//printf("Thread %d is running with priority %d\n",currentThread,threadList[currentThread].priority );
	func();
	threadList[currentThread].active = 0;
	
	/* Yield control, but because active == 0, this will free the thread */
	my_pthread_yield(2);
}
/* Switches from a thread to main or from main to a thread */
void my_pthread_yield(int priority_thread)
{	
	if(num_high_priority_threads==0 && priority_thread==0){
		alarm_count=4;
		return;
	}
	if(num_low_priority_threads==0 && priority_thread==1)
		return;
	/* If we are in a thread, switch to the main process */
	if ( inThread )
	{
		//printf("inThread\n");
		/* Switch to the main context */
		if(threadList[currentThread].priority ==1)
		{	
			//printf( "Thread %d yielding the processor...\n", currentThread );
			swapcontext( &threadList[currentThread].context, &mainContext );
		}
		if(threadList[currentThread].priority ==0)
		{	
			threadList[currentThread].priority = 1;
			num_low_priority_threads++;
			num_high_priority_threads--;
			//printf( "Thread %d yielding the processor...\n", currentThread );
			swapcontext( &threadList[currentThread].context, &mainContext );
		}
	}
	/* Else, we are in the main process and we need to dispatch a new thread */
	else
	{
		if ( numThreads == 0 ) 
			return;
		/* Saved the state so call the next thread */
		if(priority_thread==2)
			currentThread = (currentThread + 1) % numThreads;
		else
		{	if(alarm_count==5)
			currentThread=-1;
			if (priority_thread==0 && num_high_priority_threads>0)
			{
				do
				{
					currentThread = (currentThread + 1) % numThreads;
				}while(threadList[currentThread].priority!=priority_thread);
			}
			else
			{
				if (priority_thread==1 && num_low_priority_threads>0)
				{
					do
					{
						currentThread = (currentThread + 1) % numThreads;
					}while(threadList[currentThread].priority!=priority_thread);
				}
				else
					return;
			}

		}
		
		inThread = 1;
		swapcontext( &mainContext, &threadList[ currentThread ].context );
		inThread = 0;
		
		if(threadList[currentThread].swapped == 1)
			swapin();

		if ( threadList[currentThread].active == 0 )
		{
			printf( "Thread %d is finished. Cleaning up.\n", currentThread );
			/* Free the "current" thread's stack */
			free( threadList[currentThread].stack );
			

            temp = global_base;
            while(temp != NULL) 
            {
    
                if(temp->owner_thread == currentThread)
    	              temp->free = 1;
                temp = temp->next;
            }
 			//printf("Freed\n");

			/* Swap the last thread with the current, now empty, entry */
			-- numThreads;
			if(threadList[currentThread].priority ==0)
				num_high_priority_threads--;
			if(threadList[currentThread].priority ==1)
				num_low_priority_threads--;
			if ( currentThread != numThreads )
			{
				threadList[ currentThread ] = threadList[ numThreads ];
			}
			threadList[ numThreads ].active = 0;		
		}
		
	}
	return;
}

int get_numThreads(){
	return numThreads;
}


int my_pthread_create( void (*func)(void) )
{
	if ( numThreads == MAX_THREADS ) return LF_MAXTHREADS;
	num_high_priority_threads++;
	/* Add the new function to the end of the thread list */
	getcontext( &threadList[numThreads].context );

	/* Set the context to a newly allocated stack */
	threadList[numThreads].context.uc_link = 0;
	threadList[numThreads].stack = malloc( FIBER_STACK );
	threadList[numThreads].context.uc_stack.ss_sp = threadList[numThreads].stack;
	threadList[numThreads].context.uc_stack.ss_size = FIBER_STACK;
	threadList[numThreads].context.uc_stack.ss_flags = 0;
	threadList[numThreads].start_address = (void*)myallocate(4095,__FILE__,__LINE__,0);
	threadList[numThreads].end_address = threadList[numThreads].start_address + 4095;
	threadList[numThreads].current_index = 0;
	
	if ( threadList[numThreads].stack == 0 || threadList[numThreads].start_address == NULL)
	{
		LF_DEBUG_OUT( "Error: Could not allocate stack. or heapspace" );
		return LF_MALLOCERROR;
	}
	
	/* Create the context. The context calls threadStart( func ). */
	makecontext( &threadList[ numThreads ].context, (void (*)(void)) &threadStart, 1, func );
	++ numThreads;
	
	return LF_NOERROR;
}
int waitForAllThreads()
{	
	int threadsRemaining = 0;
	struct sigaction sa;
    struct itimerval timer;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = &alarmhandler;
    sigaction(SIGALRM, &sa, NULL);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 700000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 700000;
    setitimer(ITIMER_REAL, &timer, NULL);  
	while(1){
		if(numThreads==0)
			break;
	}
	return LF_NOERROR;
}

int my_pthread_join(void * t, void ** vp)
{
    if (vp)
        *vp = t;
    return 0;
}


/*Mutex Implementation*/


int test_and_set(int mutex) {
   int tmp=mutex;
   mutex = LOCKED;
   return tmp;
}

void mutex_init(int *mutex) {
   *mutex = UNLOCKED;
   // printf("unlocked\n");
}

void mutex_lock(int *mutex) {
   while(test_and_set(*mutex));
   // printf("locked\n");
   *mutex =LOCKED;
}

void mutex_unlock(int *mutex) {
   *mutex = UNLOCKED;
   // printf("unlocked\n");
}
void mutex_destroy(int *mutex){
}

