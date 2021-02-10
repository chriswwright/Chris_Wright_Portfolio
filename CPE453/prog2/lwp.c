#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "fp.h"
#include "lwp.h"
#include "rr.h"

/*Macros for next valid thread in a linked list fassion*/
#define next_thread lib_one
#define prev_thread lib_two

/*global variables for scheduler and thread structures*/
static scheduler sched = NULL;
static rfile *original = NULL;
static thread lwp = NULL;
static thread running = NULL;


/* This function and macro is helpful to set up the LWP in x86_64 architectures 
You call it from inside lwp_create(), and it should be defined in lwp.c. 
The input "sp" is the current stack pointer, which 
at the beginning is just the number of (unsigned long) spaces AFTER the base of the stack.
It returns the new stack pointer which you should save in your LWP structure. */
/* useful for "intuitively" building stacks */
#define push(sp,val) (*(--sp)=(unsigned long)(val))

static unsigned long *new_intel_stack64(unsigned long *sp,lwpfun func){
  /* mock up a stack for the INTEL architecture
   * First, a frame that returns to lwp_exit() should our function actually
   * return.
   */
  unsigned long *ebp;

  push(sp,lwp_exit);        /* just in case this lwp tries to return */
  push(sp,func);            /* push the function's return address */
  push(sp,0);               /* push a "saved" base pointer */

  ebp=sp;                   /* note the location for use later... */

  return ebp;
}
  
/* Creates a new lightweight process (lwp) which executes the given
 * function func with argument arg. Allocates a stack of stacksize
 * words for use by the created lwp.
 * 
 * returns:
 * -1 if the new thread cannot be created,
 * thread->tid of the created thread.
 */
tid_t lwp_create(lwpfun func, void * arg, size_t stacksize){
  thread target;
  if(!lwp){
    /*if lwp does not exist (i.e. this is the first lightweight process),
      malloc a structure to contain this process */
    lwp = calloc(1, sizeof(context));
    if(!lwp){
      perror("lwp creation");
      return -1;
    }
    lwp->next_thread = NULL;
    lwp->prev_thread = NULL;
    target = lwp;
  }else{
    /*If lwp exists find the end of the linked list of leightweight processes*/
    target = lwp;
    while(target->next_thread){
      target = target->next_thread;
    }
    target->next_thread = calloc(1, sizeof(context));
    if(!target->next_thread){
      perror("new lwp error");
      return -1;
    }
    target->next_thread->prev_thread = target;
    target = target->next_thread;
  }
  
  /**/
  if(target == lwp){
    target->tid = 1;
  }else{
    target->tid = target->prev_thread->tid + 1;
  }

  target->stack = malloc(sizeof(unsigned long) * stacksize);
  target->stacksize = stacksize;

  target->state.rbp = (unsigned long)new_intel_stack64(target->stack + (stacksize), func);
  target->state.rdi = (unsigned long)arg;
  target->state.fxsave = FPU_INIT;
  
  return target->tid;
}

/* Terminates the current lightweight process that is being executed
 * and frees its reasources. Calls sched->next() to start execution
 * of the next lwp thread.
 * If there is no other thread, this function should restore the original
 * system thread: the process which generated the threads in the first
 * place.
 */
void lwp_exit(void){
  
  thread old = running;
  sched->remove(old);
  running = sched->next();
  if(old == lwp){
    lwp = old->next_thread;
    //lwp->prev_thread = NULL;
  } else{
    if(old->next_thread != NULL){
      old->prev_thread->next_thread = old->next_thread;
      old->next_thread->prev_thread = old->prev_thread;
    } else{
      old->prev_thread->next_thread = NULL;
    }
  }
  free(old->stack);
  free(old);

  if(running){
    load_context(&running->state);
  }else{
    load_context(original);  
  }
}

/* returns the thread id of the calling lightweight process,or NO_THREAD
 * if not called by a process
 */
tid_t lwp_gettid(void){
  /*if thread->tid exists, return that value, else return NO_THREAD*/
  if(running){
  
    return running->tid;
  }
  return NO_THREAD;
}

/* Tells the current lightweight processs to stop executing and yield control
 * to the next lwp. Saves the current lwp, picks the next one, restores that
 * threads context and returns.
 */
void lwp_yield(void){
  save_context(&running->state);
  thread old = running;
  running = sched->next();
  if(old != running){
  
    load_context(&running->state);
  }
}

/* starts the lwp system. first saves the original context for lwp_stop() to use
   later, then picks an lwp and starts it running.
 */
void lwp_start(void){
  if(!original){
    original = calloc(1, sizeof(rfile));
  }
  save_context(original);
  lwp_set_scheduler(NULL);
  running = sched->next();
  load_context(&running->state);
}

/* stops the  lwp system, and restores the original context (the one which called
 * lwp_start())
 * does not destroy any existing context, just suspends them for later reactivation
 * by lwp_start() 
 */
void lwp_stop(void){
  swap_rfiles(&running->state, original);
}

/* Causes the LWP package to use the given scheduler in order to choose the next
 * process to run. If the provided pointer is NULL, resets the lwp to round robin
 * scheduling.
 */ 
void lwp_set_scheduler(scheduler fun){
  thread cur = lwp;
  
  /*if fun is NULL, set fun equal to the Round Robin scheduler*/
  if(!fun){
    fun = RoundRobin;
  }
  /*if init exists, run it before adding threads to the scheduler*/
  if(fun->init){
    fun->init();
  }

  while(cur){
    fun->admit(cur);
    if(sched){
      sched->remove(cur);
    }
    cur = cur->next_thread;
  }

  if(sched){
    if(sched->shutdown){
      sched->shutdown();
    }
  }
  sched = fun;
}

/*returns the pointer to the current scheduler*/
scheduler lwp_get_scheduler(void){
  return sched;
}

/* returns the thread pointer associated with the given thread ID, or Null if the ID
 * is inavalid
 */
thread tid2thread(tid_t tid){
  thread target = lwp;
  /* starting at the first thread in the library, cycle through each checking if there
   * exists a thread with the id tid.
   */
  while(target->tid != tid){
    target = target->next_thread;
  }
  return target;
}
