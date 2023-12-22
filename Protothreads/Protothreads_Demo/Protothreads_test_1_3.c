
/*
 -- Multicore --
 Protothreads test code: Bruce Land

 Three threads on core0:
 0 - cycle hog -- kills other theads in priority mode
 ---- if the sleep time is big enough
 1- serial i/o test 
 ---- read two numbers, adds, print the sum
 ---- sends numbers to core 1 (which sums them) then prints result
 ---- also prints error count for semaphone and lock
 ---- also prints scheduler stats for priority schedulter
 ---- assumes uart0 on gpio 0 and 1
 ---- supports <enter> and <backspace>
 2- One of Two threads with two semaphore alternations
 ---- note that this code is testing core-safe semaphore
 (other is on core 1)

 Three threads on core1
 1- toggle an gpio quicklt
 2- One of Two threads with two semaphore alternations
 ---- note that this code is testing core-safe semaphore
 (other is on core 0)
 3- wait for core 0 to send two numbers, adds them , sends back the sum

The two alternating threads increment and decrement a counter, so that if they 
truely alternate the counter should be either 0 or 1. The blinky thread on core 0
watches the count and prints a messaage if there is an error. The code was 
tested for over 300 million thread swaps with no errors.

The syntax for useing the safe semaphores is the same
as the default semaphores, except that the tag SAFE is added
into the macro definition

The LOCK macros use spinlock 24 to insure atomic access
(plus the specific spin-LOCK that you specify)
The SEM macros use spinlock 25 to insure atomic access
Do not use ANY spin-lock below number 26!

 With just two alernating threads, thread rate
 is about 1.25 million/sec usinig the default (unsafe) semaphores
 and on one core
 is about 240 thousand/sec with safe semaphores and locked counter
 on two cores
 */

#include "hardware/gpio.h"
#include "hardware/timer.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "stdio.h"
#include <string.h>
#include <pico/multicore.h>
#include "hardware/sync.h"
#include "stdlib.h"
//

// ==========================================
// === protothreads globals
// ==========================================
// protothreads header
#include "pt_cornell_rp2040_v1_3.h"

// === global thread communicaiton
// locks to force two threads to alternate execution
// (useful for timing a thread context switch)
// uses low level hardware spin lock 
struct pt_sem thread2_go_s, thread3_go_s ;
//
// counters for detecting thread swap lock errors
// the difference between two threads that should alternate
int delta_count ;
// total times delta_count was greater than 1 or less than zero
int error_count ;
//  total number of times thru each  of the threads
// core_thread
int thread_count ;
// data protections
spin_lock_t * lock_delta_count ;
spin_lock_t * lock_error_count ;

// ==================================================
// === user's serial input thread
// ==================================================
// serial_read an serial_write do not block any thread
// except this one
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
      static int test_in1, test_in2, sum ;
      //
      while(1) {
        // print prompt
        sprintf(pt_serial_out_buffer, "input two numbers: ");
        // spawn a thread to do the non-blocking write
        serial_write ;

        // spawn a thread to do the non-blocking serial read
         serial_read ;
        // convert input string to number
        sscanf(pt_serial_in_buffer,"%d %d", &test_in1, &test_in2) ;

        // add and convert back to sring
        sprintf(pt_serial_out_buffer,"sum = %d\r\n", test_in1 + test_in2);
        // spawn a thread to do the non-blocking serial write
         serial_write ;

         // send data to a thread on core 1 to be added
         // these macros block this thread until read/write is possible
         PT_FIFO_WRITE(test_in1);
         PT_FIFO_WRITE(test_in2);
         PT_FIFO_READ(sum) ;
         sprintf(pt_serial_out_buffer,"core1 sum = %d\r\n", sum);
        // spawn a thread to do the non-blocking serial write
         serial_write ; 

        // the semaphore error reporting variabless
        // NOTE --  PT_GET_TIME_usec() returns a uint64_t
        // print with %lld  (long long int)
         sprintf(pt_serial_out_buffer,
          "delta=%d, error=%d total=%d time=%lld\n\r",
           delta_count, error_count, thread_count, PT_GET_TIME_usec()/1000000);
        // spawn a thread to do the non-blocking serial write
         serial_write ; 

         printf("\n\r");
         // scheduler stats
         sprintf(pt_serial_out_buffer, "core0 thread execution counts: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count; i++){
            printf("T%d=%d ",i,  sched_thread_stats[i]);
          }
          printf("\n\r");
          //
          sprintf(pt_serial_out_buffer, "core0 thread summed execution time: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count; i++){
            printf("T%d=%lld ",i,  sched_thread_time[i]);
          }
          printf("\n\r\n\r");
          //
          sprintf(pt_serial_out_buffer, "core1 thread execution counts: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count1; i++){
            printf("T%d=%d ",i,  sched_thread_stats1[i]);
          }
          printf("\n\r");

          //
          sprintf(pt_serial_out_buffer, "core1 thread summed execution time: ") ;
        // spawn a thread to do the non-blocking serial write
         serial_write ; 
          for(int i=0; i < pt_task_count1; i++){
            printf("T%d=%lld ",i,  sched_thread_time1[i]);
          }
          printf("\n\r");
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// ==================================================
// === cycle hog thread 
// ==================================================
// tie up the cpu if it is high priority
// just a 'pt_yield' kills all other threads of lower priority.
// yield for less than a few microseconds slows down
// the fast toggle2 thread and makes it irregular
// 
//
static PT_THREAD (protothread_cycle_hog(struct pt *pt))
{
    PT_BEGIN(pt);

      while(1) {
        // yield time 
        PT_YIELD_usec(100) ;
        // waste time to check for thread starvation
        sleep_us(50);
        // NEVER exit while
      } // END WHILE(1)

  PT_END(pt);
} // cycle hog thread


// ==================================================
// === toggle2 thread 
// ==================================================
// toggle gpio 2  while alternating with
// another thread
static PT_THREAD (protothread_toggle2(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    //
     // set up LED gpio 2
     gpio_init(2) ;	
     gpio_set_dir(2, GPIO_OUT) ;
     gpio_put(2, true);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        // lock thread2_go is signaled from 
        // thread toggle3
        PT_SEM_SAFE_WAIT(pt, &thread2_go_s) ;
        //
        // toggle gpio 2
        LED_state = !LED_state ;
        gpio_put(2, LED_state);
        //
        PT_YIELD_INTERVAL(50000) ;

        // alternation error detection
        PT_LOCK_WAIT(pt, lock_delta_count) ;
        delta_count++;
        PT_LOCK_RELEASE(lock_delta_count);

        // total thread count
        thread_count++;
        //
        // tell toggle3 thread to run
        PT_SEM_SAFE_SIGNAL(pt, &thread3_go_s) ;
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ==================================================
// === toggle3 thread -- RUNNING on core 1
// ==================================================
// toggle gpio 3 while alternating with
// another thread
static PT_THREAD (protothread_toggle3(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    //
     // set up gpio 3
     gpio_init(3) ;	
     gpio_set_dir(3, GPIO_OUT) ;
     gpio_put(3, false);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        // lock thread3_go is signaled from 
        // thread toggle2
        PT_SEM_SAFE_WAIT(pt, &thread3_go_s) ;

        // toggle gpio 3
        LED_state = !LED_state ;
        gpio_put(3, LED_state);
        // interval
        PT_YIELD_INTERVAL(50) ;
        //
        PT_LOCK_WAIT(pt, lock_delta_count) ;
        delta_count--;
        PT_LOCK_RELEASE(lock_delta_count);

        // tell toggle2 to run
        PT_SEM_SAFE_SIGNAL(pt, &thread2_go_s) ;
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ==================================================
// === toggle_fast thread -- RUNNING on core 1
// ==================================================
// toggle gpio 4 
static PT_THREAD (protothread_toggle_fast(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    //
     // set up LED gpio 4 to blink
     gpio_init(4) ;	
     gpio_set_dir(4, GPIO_OUT) ;
     gpio_put(4, true);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        //
        PT_YIELD_INTERVAL(20) ;
        // toggle gpio 4
        LED_state = !LED_state ;
        gpio_put(4, LED_state);
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ==================================================
// === sum thread -- RUNNING on core 1
// ==================================================
// fifo test example
static PT_THREAD (protothread_sum(struct pt *pt))
{
    PT_BEGIN(pt);
      static int in1, in2  ;
      while(1) {
        // wait for core 0 to write two items
        PT_FIFO_READ(in1) ;
        PT_FIFO_READ(in2) ;
        // send back the sum
        PT_FIFO_WRITE(in1 + in2);
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main(){ 
  // reset FIFO sending data to core 0
  PT_FIFO_FLUSH ;
  //  === add threads  ====================
  // for core 1
  // NOTE first added is HIGHEST priority
  pt_add_thread(protothread_toggle_fast);
  pt_add_thread(protothread_toggle3);
  pt_add_thread(protothread_sum) ;
  //
  // === initalize the scheduler ==========
  pt_sched_method = SCHED_PRIORITY ;
  pt_schedule_start ;
  // NEVER exits
  // ======================================
}

// ========================================
// === core 0 main
// ========================================
int main(){
  // set the clock
  //set_sys_clock_khz(250000, true); // 171us
  // start the serial i/o
  stdio_init_all() ;
  // announce the threader version on system reset
  printf("\n\rProtothreads RP2040 v1.3 two-core\n\r");

  // init global locks
  // to enforce alernation between threads
  // BEFORE starting any threads
  // use locks 31 downto 22
  
  // lock 31 init
   PT_LOCK_INIT(lock_error_count, 31, UNLOCKED) ;
   // lock 30 init 
   PT_LOCK_INIT(lock_delta_count, 30, UNLOCKED) ;

   // two safe semaphores
   PT_SEM_SAFE_INIT(&thread2_go_s, 1) ;
   PT_SEM_SAFE_INIT(&thread3_go_s, 0) ;

   // reset FIFO sending data to core 1
  PT_FIFO_FLUSH ;

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0
  // NOTE first added is HIGHEST priority
  pt_add_thread(protothread_cycle_hog);
  pt_add_thread(protothread_toggle2);
  pt_add_thread(protothread_serial);
  //
  // === initalize the scheduler ===============
  // method is either:
  //   SCHED_PRIORITY or SCHED_ROUND_ROBIN
  pt_sched_method = SCHED_PRIORITY ;
  pt_schedule_start ;
  // NEVER exits
  // ===========================================
} // end main
///////////
// end ////
///////////