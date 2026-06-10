/* 
 * File:   pt_cornell_rp2040_v1.h
 * Author: brl4 Briuce Land
 * Bruce R Land, Cornell University
 * Created on Dec 10, 2018
 */

/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: pt.h,v 1.7 2006/10/02 07:52:56 adam Exp $
 */
/**
 * \addtogroup pt
 * @{
 */

/**
 * \file
 * Protothreads implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 *
 */

#ifndef __PT_H__
#define __PT_H__

////////////////////////
//#include "lc.h"
////////////////////////
/**
 * \file lc.h
 * Local continuations
 * \author
 * Adam Dunkels <adam@sics.se>
 *
 */

#ifdef DOXYGEN
/**
 * Initialize a local continuation.
 *
 * This operation initializes the local continuation, thereby
 * unsetting any previously set continuation state.
 *
 * \hideinitializer
 */
#define LC_INIT(lc)

/**
 * Set a local continuation.
 *
 * The set operation saves the state of the function at the point
 * where the operation is executed. As far as the set operation is
 * concerned, the state of the function does <b>not</b> include the
 * call-stack or local (automatic) variables, but only the program
 * counter and such CPU registers that needs to be saved.
 *
 * \hideinitializer
 */
#define LC_SET(lc)

/**
 * Resume a local continuation.
 *
 * The resume operation resumes a previously set local continuation, thus
 * restoring the state in which the function was when the local
 * continuation was set. If the local continuation has not been
 * previously set, the resume operation does nothing.
 *
 * \hideinitializer
 */
#define LC_RESUME(lc)

/**
 * Mark the end of local continuation usage.
 *
 * The end operation signifies that local continuations should not be
 * used any more in the function. This operation is not needed for
 * most implementations of local continuation, but is required by a
 * few implementations.
 *
 * \hideinitializer 
 */
#define LC_END(lc)

/**
 * \var typedef lc_t;
 *
 * The local continuation type.
 *
 * \hideinitializer
 */
#endif /* DOXYGEN */

//#ifndef __LC_H__
//#define __LC_H__


//#ifdef LC_INCLUDE
//#include LC_INCLUDE
//#else

/////////////////////////////
//#include "lc-switch.h"
/////////////////////////////

//#ifndef __LC_SWITCH_H__
//#define __LC_SWITCH_H__

/* WARNING! lc implementation using switch() does not work if an
   LC_SET() is done within another switch() statement! */

/** \hideinitializer */
/*
typedef unsigned short lc_t;

#define LC_INIT(s) s = 0;

#define LC_RESUME(s) switch(s) { case 0:

#define LC_SET(s) s = __LINE__; case __LINE__:

#define LC_END(s) }

#endif /* __LC_SWITCH_H__ */

/** @} */

//#endif /* LC_INCLUDE */

//#endif /* __LC_H__ */

/** @} */
/** @} */

/////////////////////////////
//#include "lc-addrlabels.h"
/////////////////////////////

#ifndef __LC_ADDRLABELS_H__
#define __LC_ADDRLABELS_H__

/** \hideinitializer */
typedef void * lc_t;

#define LC_INIT(s) s = NULL

#define LC_RESUME(s)				\
  do {						\
    if(s != NULL) {				\
      goto *s;					\
    }						\
  } while(0)

#define LC_CONCAT2(s1, s2) s1##s2
#define LC_CONCAT(s1, s2) LC_CONCAT2(s1, s2)

#define LC_SET(s)				\
  do {						\
    LC_CONCAT(LC_LABEL, __LINE__):   	        \
    (s) = &&LC_CONCAT(LC_LABEL, __LINE__);	\
  } while(0)

#define LC_END(s)

#endif /* __LC_ADDRLABELS_H__ */

//////////////////////////////////////////
struct pt {
  lc_t lc;
};

#define PT_WAITING 0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3

/**
 * \name Initialization
 * @{
 */

/**
 * Initialize a protothread.
 *
 * Initializes a protothread. Initialization must be done prior to
 * starting to execute the protothread.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PT_INIT(pt)   LC_INIT((pt)->lc)

/** @} */

/**
 * \name Declaration and definition
 * @{
 */

/**
 * Declaration of a protothread.
 *
 * This macro is used to declare a protothread. All protothreads must
 * be declared with this macro.
 *
 * \param name_args The name and arguments of the C function
 * implementing the protothread.
 *
 * \hideinitializer
 */
#define PT_THREAD(name_args) char name_args

/**
 * Declare the start of a protothread inside the C function
 * implementing the protothread.
 *
 * This macro is used to declare the starting point of a
 * protothread. It should be placed at the start of the function in
 * which the protothread runs. All C statements above the PT_BEGIN()
 * invokation will be executed each time the protothread is scheduled.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_BEGIN(pt) { char PT_YIELD_FLAG = 1; LC_RESUME((pt)->lc)

/**
 * Declare the end of a protothread.
 *
 * This macro is used for declaring that a protothread ends. It must
 * always be used together with a matching PT_BEGIN() macro.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_END(pt) LC_END((pt)->lc); PT_YIELD_FLAG = 0; \
                   PT_INIT(pt); return PT_ENDED; }

/** @} */

/**
 * \name Blocked wait
 * @{
 */

/**
 * Block and wait until condition is true.
 *
 * This macro blocks the protothread until the specified condition is
 * true.
 *
 * \param pt A pointer to the protothread control structure.
 * \param condition The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_UNTIL(pt, condition)	        \
  do {						\
    LC_SET((pt)->lc);				\
    if(!(condition)) {				\
      return PT_WAITING;			\
    }						\
  } while(0)

/**
 * Block and wait while condition is true.
 *
 * This function blocks and waits while condition is true. See
 * PT_WAIT_UNTIL().
 *
 * \param pt A pointer to the protothread control structure.
 * \param cond The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_WHILE(pt, cond)  PT_WAIT_UNTIL((pt), !(cond))

/** @} */

/**
 * \name Hierarchical protothreads
 * @{
 */

/**
 * Block and wait until a child protothread completes.
 *
 * This macro schedules a child protothread. The current protothread
 * will block until the child protothread completes.
 *
 * \note The child protothread must be manually initialized with the
 * PT_INIT() function before this function is used.
 *
 * \param pt A pointer to the protothread control structure.
 * \param thread The child protothread with arguments
 *
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))

/**
 * Spawn a child protothread and wait until it exits.
 *
 * This macro spawns a child protothread and waits until it exits. The
 * macro can only be used within a protothread.
 *
 * \param pt A pointer to the protothread control structure.
 * \param child A pointer to the child protothread's control structure.
 * \param thread The child protothread with arguments
 *
 * \hideinitializer
 */
#define PT_SPAWN(pt, child, thread)		\
  do {						\
    PT_INIT((child));				\
    PT_WAIT_THREAD((pt), (thread));		\
  } while(0)

/** @} */

/**
 * \name Exiting and restarting
 * @{
 */

/**
 * Restart the protothread.
 *
 * This macro will block and cause the running protothread to restart
 * its execution at the place of the PT_BEGIN() call.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_RESTART(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_WAITING;			\
  } while(0)

/**
 * Exit the protothread.
 *
 * This macro causes the protothread to exit. If the protothread was
 * spawned by another protothread, the parent protothread will become
 * unblocked and can continue to run.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_EXIT(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_EXITED;			\
  } while(0)

/** @} */

/**
 * \name Calling a protothread
 * @{
 */

/**
 * Schedule a protothread.
 *
 * This function shedules a protothread. The return value of the
 * function is non-zero if the protothread is running or zero if the
 * protothread has exited.
 *
 * \param f The call to the C function implementing the protothread to
 * be scheduled
 *
 * \hideinitializer
 */
#define PT_SCHEDULE(f) ((f) < PT_EXITED)
//#define PT_SCHEDULE(f) ((f))

/** @} */

/**
 * \name Yielding from a protothread
 * @{
 */

/**
 * Yield from the current protothread.
 *
 * This function will yield the protothread, thereby allowing other
 * processing to take place in the system.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
// modified 9/26/23 for priority scheduler
// this will be set to zero by the scheduler,
// and set to one, if a thread actually executes
int pt_executed, pt_executed1 ;
//
#define PT_YIELD(pt)				\
  do {						\
    PT_YIELD_FLAG = 0;				\
    LC_SET((pt)->lc);				\
    if(PT_YIELD_FLAG == 0) {			\
      return PT_YIELDED;			\
    }	 \
    if(get_core_num()==1){ \
    pt_executed1 = 1;;\
    }  else {\
      pt_executed = 1;\
    }\
  } while(0)

/**
 * \brief      Yield from the protothread until a condition occurs.
 * \param pt   A pointer to the protothread control structure.
 * \param cond The condition.
 *
 *             This function will yield the protothread, until the
 *             specified condition evaluates to true.
 *
 *
 * \hideinitializer
 */

#define PT_YIELD_UNTIL(pt, cond)		\
  do {						\
    PT_YIELD_FLAG = 0;				\
    LC_SET((pt)->lc);				\
    if((PT_YIELD_FLAG == 0) || !(cond)) {	\
      return PT_YIELDED;                  \
    }	\
    if(get_core_num()==1){ \
    pt_executed1 = 1;\
    }  else {\
      pt_executed = 1;\
    }\
  } while(0)

  /**/

/** @} */

#endif /* __PT_H__ */

#ifndef __PT_SEM_H__
#define __PT_SEM_H__

//#include "pt.h"

struct pt_sem {
  unsigned int count;
};

/**
 * Initialize a semaphore
 *
 * This macro initializes a semaphore with a value for the
 * counter. Internally, the semaphores use an "unsigned int" to
 * represent the counter, and therefore the "count" argument should be
 * within range of an unsigned int.
 *
 * \param s (struct pt_sem *) A pointer to the pt_sem struct
 * representing the semaphore
 *
 * \param c (unsigned int) The initial count of the semaphore.
 * \hide initializer
 */
// NOTE that the default semaphore is not
// multi-core safe, but is OK one one core

#define PT_SEM_INIT(s, c) (s)->count = c

/**
 * Wait for a semaphore
 *
 * This macro carries out the "wait" operation on the semaphore. The
 * wait operation causes the protothread to block while the counter is
 * zero. When the counter reaches a value larger than zero, the
 * protothread will continue.
 *
 * \param pt (struct pt *) A pointer to the protothread (struct pt) in
 * which the operation is executed.
 *
 * \param s (struct pt_sem *) A pointer to the pt_sem struct
 * representing the semaphore
 *
 * \hideinitializer
 */
#define PT_SEM_WAIT(pt, s)	\
  do {						\
    PT_YIELD_UNTIL(pt, (s)->count > 0);		\
    --(s)->count;				\
  } while(0)

/**
 * Signal a semaphore
 *
 * This macro carries out the "signal" operation on the semaphore. The
 * signal operation increments the counter inside the semaphore, which
 * eventually will cause waiting protothreads to continue executing.
 *
 * \param pt (struct pt *) A pointer to the protothread (struct pt) in
 * which the operation is executed.
 *
 * \param s (struct pt_sem *) A pointer to the pt_sem struct
 * representing the semaphore
 *
 * \hideinitializer
 */
//#define PT_SEM_SIGNAL(pt, s) ++(s)->count
#define PT_SEM_SIGNAL(pt,s) ++(s)->count

#endif /* __PT_SEM_H__ */

//=====================================================================
//=== BRL4 additions for rp2040 =======================================
//=====================================================================
// NOTE: modifed from version 1.1.1 !!!! for 64 bits
// macro to make a thread execution pause in usec
// max time of about 300,000 years
// uint64_t time_us_64 (void)

#define PT_YIELD_usec(delay_time)  \
    do { static uint64_t time_thread ;\
    time_thread = time_us_64() + (uint64_t)delay_time ; \
    PT_YIELD_UNTIL(pt, (time_us_64() >= time_thread)); \
    } while(0);

// macro to return system time
#define PT_GET_TIME_usec() (time_us_64())

// macros for interval yield
// attempts to make interval equal to specified value
#define PT_INTERVAL_INIT() static uint64_t pt_interval_marker
//
#define PT_YIELD_INTERVAL(interval_time)  \
    do { \
    PT_YIELD_UNTIL(pt, (uint32_t)(time_us_64() >= pt_interval_marker)); \
    pt_interval_marker = time_us_64() + (uint64_t)interval_time; \
    } while(0);
//
// =================================================================
// core-safe semaphore based on pico/sync library
// NEEDS SDK 1.1.1 or higher
// a hardware spinlock to force core-safe alternation
// NOTE that the default protothreads semaphore is not
// multi-core safe, but is OK one one core
// The SAFE versions work across cores, but have more overhead

#define PT_SEM_SDK_WAIT(pt,s)	do {	\
   PT_YIELD_UNTIL (pt, sem_try_acquire (s)); \
   if(get_core_num()==1){ \
      pt_executed1 = 1;\
    }  else {\
      pt_executed = 1;\
    }\
  } while(0) ;

// removed (pt, 
#define PT_SEM_SDK_SIGNAL(pt,s) do{ \
  sem_release (s) ; \
} while(0) ;


// ==================================================================
// core-safe mutex based on pico/sync library
// NEEDS SDK 1.1.1 or higher

#define PT_MUTEX_SDK_AQUIRE(pt,s)	do {	\
  PT_YIELD_UNTIL(pt, mutex_try_enter (s, NULL)); \
  if(get_core_num()==1){ \
      pt_executed1 = 1;;\
    }  else {\
      pt_executed = 1;\
    }\
} while(0)

#define PT_MUTEX_SDK_RELEASE(s) do{ \
  mutex_exit(s); \
} while(0)

//====================================================================
// Multicore communication via FIFO
#define PT_FIFO_WRITE(data) do{ \
    PT_YIELD_UNTIL(pt, multicore_fifo_wready()==true); \
    multicore_fifo_push_blocking(data) ; \
} while(0)

#define PT_FIFO_READ(fifo_out)  \
do{ \
    PT_YIELD_UNTIL(pt, multicore_fifo_rvalid()==true); \
    fifo_out = multicore_fifo_pop_blocking() ; \
} while(0) 

// clears OUTGOING FIFO for urrent core
#define PT_FIFO_FLUSH do{ \
    multicore_fifo_drain() ; \
} while(0)

//====================================================================
// IMPROVED SCHEDULER 
// === thread structures ===
// thread control structs

// A modified scheduler
static struct pt pt_sched ;
// second core
static struct pt pt_sched1 ;

// count of defined tasks
int pt_task_count = 0 ;
int pt_task_count1 = 0 ;

// The task structure
struct ptx {
	struct pt pt;              // thread context
	int num;                    // thread number
	char (*pf)(struct pt *pt); // pointer to thread function
};

// === extended structure for scheduler ===============
// an array of task structures
#define MAX_THREADS 10
static struct ptx pt_thread_list[MAX_THREADS];
// core 1
static struct ptx pt_thread_list1[MAX_THREADS];

// see https://github.com/edartuz/c-ptx/tree/master/src
// and the license above
// add an entry to the thread list
//struct ptx *pt_add( char (*pf)(struct pt *pt), int rate) {
int pt_add( char (*pf)(struct pt *pt)) {
	if (pt_task_count < (MAX_THREADS)) {
        // get the current thread table entry 
		struct ptx *ptx = &pt_thread_list[pt_task_count];
        // enter the tak data into the thread table
		ptx->num   = pt_task_count;
        // function pointer
		ptx->pf    = pf;
    //
		PT_INIT( &ptx->pt );
        // count of number of defined threads
		pt_task_count++;
        // return current entry
        return pt_task_count-1;
	}
	return 0;
}

// core 1 -- add an entry to the thread list
//struct ptx *pt_add( char (*pf)(struct pt *pt), int rate) {
int pt_add1( char (*pf)(struct pt *pt)) {
	if (pt_task_count1 < (MAX_THREADS)) {
        // get the current thread table entry 
		struct ptx *ptx = &pt_thread_list1[pt_task_count1];
        // enter the tak data into the thread table
		ptx->num   = pt_task_count1;
        // function pointer
		ptx->pf    = pf;
    //
		PT_INIT( &ptx->pt );
        // count of number of defined threads
		pt_task_count1++;
        // return current entry
        return pt_task_count1-1;
	}
	return 0;
}

/* Scheduler
Copyright (c) 2014 edartuz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// === Scheduler Thread =================================================
// update a 1 second tick counter
// schedulser code was almost copied from
// https://github.com/edartuz/c-ptx
// see license above

// choose schedule method
#define SCHED_ROUND_ROBIN 0
#define SCHED_PRIORITY    1
// default is round robin
int pt_sched_method = SCHED_ROUND_ROBIN ;

// =========================================
// If defined, accumulates execution stats, 
//    but slows down scheduler!!
#define sched_stats
int sched_thread_stats[MAX_THREADS], sched_thread_stats1[MAX_THREADS] ;
uint64_t sched_thread_time[MAX_THREADS], thread_time ;
uint64_t sched_thread_time1[MAX_THREADS], thread_time1 ;
int sched_count, sched_count1 ;
// =========================================

static PT_THREAD (protothread_sched(struct pt *pt))
{   
    PT_BEGIN(pt);
    static int i, rate;
    
    if (pt_sched_method==SCHED_ROUND_ROBIN){
        while(1) {
          // test stupid round-robin 
          // on all defined threads
          struct ptx *ptx = &pt_thread_list[0];
          // step thru all defined threads
          // -- loop can have more than one initialization or increment/decrement, 
          // -- separated using comma operator. But it can have only one condition.
          for (i=0; i<pt_task_count; i++, ptx++ ){
              // call thread function
              (pt_thread_list[i].pf)(&ptx->pt); 
          }
          // Never yields! 
          // NEVER exit while!
        } // END WHILE(1)
    } //end if (pt_sched_method==RR)     
    //  
    if (pt_sched_method==SCHED_PRIORITY){
        while(1) {
          // test stupid round-robin 
          // on all defined threads
          struct ptx *ptx = &pt_thread_list[0];

          #ifdef sched_stats
           sched_count++ ;
          #endif

          // step thru all defined threads
          // -- loop can have more than one initialization or increment/decrement, 
          // -- separated using comma operator. But it can have only one condition.
          for (i=0; i<pt_task_count; i++, ptx++ ){
              // zero execute flag
              pt_executed = 0;
              thread_time = time_us_64();
              // call thread function
              (pt_thread_list[i].pf)(&ptx->pt); 
              // if there was execution, then restart execution list
              if (pt_executed==1){
                #ifdef sched_stats
                  sched_thread_stats[i]++ ;
                  sched_thread_time[i] += (time_us_64()-thread_time);
                #endif
                break ;
              }
          }
          // Never yields! 
          // NEVER exit while!
        } // END WHILE(1)
    } //end if (pt_sched_method==priority) 
    
    PT_END(pt);
} // scheduler thread

// ================================================
// === second core scheduler
static PT_THREAD (protothread_sched1(struct pt *pt))
{   
    PT_BEGIN(pt);
    
    static int i, rate;
    
    if (pt_sched_method==SCHED_ROUND_ROBIN){
        while(1) {
          // test stupid round-robin 
          // on all defined threads
          struct ptx *ptx = &pt_thread_list1[0];
          // step thru all defined threads
          // -- loop can have more than one initialization or increment/decrement, 
          // -- separated using comma operator. But it can have only one condition.
          for (i=0; i<pt_task_count1; i++, ptx++ ){
              // call thread function
              (pt_thread_list1[i].pf)(&ptx->pt); 
          }
          // Never yields! 
          // NEVER exit while!
        } // END WHILE(1)
    } // end if(pt_sched_method==SCHED_ROUND_ROBIN)    
    //
    if (pt_sched_method==SCHED_PRIORITY){
        while(1) {
          // test stupid round-robin 
          // on all defined threads
          struct ptx *ptx = &pt_thread_list1[0];

          #ifdef sched_stats
           sched_count1++ ;
          #endif

          // step thru all defined threads
          // -- loop can have more than one initialization or increment/decrement, 
          // -- separated using comma operator. But it can have only one condition.
          for (i=0; i<pt_task_count1; i++, ptx++ ){
              // zero execute flag
              pt_executed1 = 0;
              thread_time1 = time_us_64();
              // call thread function
              (pt_thread_list1[i].pf)(&ptx->pt); 
              // if there was execution, then restart execution list
              if (pt_executed1==1){
                #ifdef sched_stats
                  sched_thread_stats1[i]++ ;
                  sched_thread_time1[i] += (time_us_64()-thread_time1);
                #endif
                break ;
              }
          }
          // Never yields! 
          // NEVER exit while!
        } // END WHILE(1)
    } //end if (pt_sched_method==priority)   
     
    PT_END(pt);
} // scheduler1 thread

// ========================================================
// === package the schedulers =============================
#define pt_schedule_start do{\
  if(get_core_num()==1){ \
    PT_INIT(&pt_sched1) ; \
    PT_SCHEDULE(protothread_sched1(&pt_sched1));\
  }  else {\
    PT_INIT(&pt_sched) ;\
    PT_SCHEDULE(protothread_sched(&pt_sched));\
  }\
} while(0) 

// === package the add thread ==========================
#define pt_add_thread(thread_name) do{\
  if(get_core_num()==1){ \
    pt_add1(thread_name);\
  }  else {\
    pt_add(thread_name);\
  }\
} while(0) 

// === serial input thread ================================
// serial buffers
#define pt_buffer_size 255
char pt_serial_in_buffer[pt_buffer_size];
char pt_serial_out_buffer[pt_buffer_size];
// thread pointers
static struct pt pt_serialin, pt_serialout ;
// uart
#define UART_ID uart0
//
#define pt_backspace 0x7f // make sure your backspace matches this!
//
static PT_THREAD (pt_serialin_polled(struct pt *pt)){
    PT_BEGIN(pt);
      static uint8_t ch ;
      static int pt_current_char_count ;
      // clear the string
      memset(pt_serial_in_buffer, 0, pt_buffer_size);
      pt_current_char_count = 0 ;
      // clear uart fifo
      while(uart_is_readable(UART_ID)){uart_getc(UART_ID);}
      // build the output string
      while(pt_current_char_count < pt_buffer_size) {   
        PT_YIELD_UNTIL(pt, (int)uart_is_readable(UART_ID)) ;
        //get the character and echo it back to terminal
        // NOTE this assumes a human is typing!!
        ch = uart_getc(UART_ID);
        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
        uart_putc(UART_ID, ch);
        // check for <enter> or <backspace>
        if (ch == '\r' ){
          // <enter>> character terminates string,
          // advances the cursor to the next line, then exits
          pt_serial_in_buffer[pt_current_char_count] = 0 ;
          PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
          uart_putc(UART_ID, '\n') ;
          break ; 
        }
        // check fo ,backspace>
        else if (ch == pt_backspace){
          PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
          uart_putc(UART_ID, ' ') ;
          PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
          uart_putc(UART_ID, pt_backspace) ;
          //uart_putc(UART_ID, ' ') ;
          // wipe a character from the output
          pt_current_char_count-- ;
          if (pt_current_char_count<0) {pt_current_char_count = 0 ;}
        }
        // must be a real character
        else {
          // build the output string
          pt_serial_in_buffer[pt_current_char_count++] = ch ;
        }
      } // END WHILe
      // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
  PT_END(pt);
} // serial input thread

// ================================================================
// === serial output thread
//
int pt_serialout_polled(struct pt *pt)
{
    static int num_send_chars ;
    PT_BEGIN(pt);
    num_send_chars = 0;
    while (pt_serial_out_buffer[num_send_chars] != 0){
        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID)) ;
        uart_putc(UART_ID, pt_serial_out_buffer[num_send_chars]) ;
        num_send_chars++;
    }
    // wait until all cha actually sent sent
    //uart_tx_wait_blocking (UART_ID) ;

    // kill this output thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}
// ================================================================
// package the spawn read/write macros to make them look better
#define serial_write do{PT_SPAWN(pt,&pt_serialout,pt_serialout_polled(&pt_serialout));}while(0)
#define serial_read  do{PT_SPAWN(pt,&pt_serialin,pt_serialin_polled(&pt_serialin));}while(0)
//
// ======
// END
// ======