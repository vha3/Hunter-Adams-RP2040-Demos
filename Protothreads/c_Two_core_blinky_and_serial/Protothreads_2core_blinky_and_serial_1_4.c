
/*
 Protothreads 1.4 demo code:
 for more info see
 https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html

 TWO threads on ONE core:

 Core 0:
 -- blinky thread: turns LED on for rp2040 or rp2350, signals off-thread
 -- serial thread: sets the blink period for the other threads

 Core 1:
-- blinky thread: turns LED off for rp2040 or rp2350, , signals on-thread

The two blink threads are synced using two core-safe semaphores

 */

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <pico/multicore.h>
#include "hardware/sync.h"
#include "stdlib.h"
#include "pico/sync.h"

// ==========================================
// === protothreads globals
// ==========================================
// protothreads header
#include "pt_cornell_rp2040_v1_4.h"
# define LED_PIN 25
// the time (in micoseconds) that the two blinky threads yields
// defaults to 0.1 second
// this global will be set by the serial thread using user input
int32_t blink_time_on =100000 ;
int32_t blink_time_off =500000 ;
// semaphores to enforece alternation of LED on/off
// The thread that turns to LED on signals the thread
// to turn the LED off, and vice-versa
semaphore_t blink_on_go_s, blink_off_go_s ;
//

// ==================================================
// === toggle25 thread -- core 0
// ==================================================
//  
static PT_THREAD (protothread_toggle25_on(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
      while(1) {
        // wait for signal from thread taht turns off LED
        PT_SEM_SDK_WAIT(pt, &blink_on_go_s) ;
        // toggle gpio 25
       // LED_state = !LED_state ;
        gpio_put(LED_PIN, true);
        // yield the thread to the scheduler
        // to figure out what to do next
        // https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html
        // blink_time is set in the serial thread
        PT_YIELD_usec(blink_time_on) ;
        // signal the off-thread to execute
        PT_SEM_SDK_SIGNAL(pt, &blink_off_go_s) ;
      } // END WHILE(1)
      // every thread ends with PT_END(pt);
      PT_END(pt);
} // end blink thread

// ==================================================
// === toggle25 thread -- core 1
// ==================================================
//  
static PT_THREAD (protothread_toggle25_off(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
      while(1) {
        // wait for signal from thread taht turns on LED
        PT_SEM_SDK_WAIT(pt, &blink_off_go_s) ;
        // toggle gpio 25
       // LED_state = !LED_state ;
        gpio_put(LED_PIN, false);
        // yield the thread to the scheduler
        //  to figure out what to do next
        // see https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html
        // blink_time is set in the serial thread
        PT_YIELD_usec(blink_time_off) ;
        // signal the on-thread to execute
        PT_SEM_SDK_SIGNAL(pt, &blink_on_go_s) ;
        // NEVER exit WHILE in a thread
      } // END WHILE(1)
      // every thread ends with PT_END(pt);
      PT_END(pt);
} // end blink thread

// ==================================================
// === user's input serial thread
// ==================================================
// serial_read and serial_write do not block any thread
// except this one
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);  
      //
      while(1) {
        // prompt the user for input
         sprintf(pt_serial_out_buffer, "enter two numbers: blink time on and off (uSec)> ") ;
        // spawn a thread to do the non-blocking serial write
        // to print the pt_serial_out_buffer string
        // this output routine YIELDs between each character printed.
         serial_write ; 
        //
        // wait for the sloooow human to type a number
        // this input roiutine YIELDs between each character typed!
        // spawn a thread to do the non-blocking serial read
        // THIS thread blocks waiting for input, others can run.
        // serial_read returns when the <enter> key is pushed
        serial_read ;
        // convert input string to number
        // and store in global var so the other thread can see it
        sscanf(pt_serial_in_buffer,"%d %d", &blink_time_on, &blink_time_off) ;
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // serial thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main(){ 
    //  === add threads  ====================
    // for core 1
    pt_add_thread(protothread_toggle25_off);
    //
    // === initalize the scheduler ==========
    pt_sched_method = SCHED_ROUND_ROBIN ;
    pt_schedule_start ;
    // NEVER exits
    // ======================================
  }

// ========================================
// === core 0 main
// ========================================
int main(){
  // need sleep to let progammer finish
  sleep_ms(10);
  //===  start the serial i/o ==================
  stdio_init_all() ;
  // announce the threader version on system reset
  // if there is a seral terminal attached
  printf("\n\rProtothreads RP2040 v1.4 two-core, priority\n\r");

  // init the alternation semaphores before starting any threads
  // two safe semaphores, let the LED on thread go first
  // arguments are: pointer to semaphore, initial coun, t max count
  sem_init(&blink_on_go_s, 1, 1) ;
  sem_init(&blink_off_go_s, 0, 1 ) ;
  // since two threads use this pin, set it up globally
  gpio_init(LED_PIN) ;	
  gpio_set_dir(LED_PIN, GPIO_OUT) ;

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0 two threads
  pt_add_thread(protothread_toggle25_on);
  pt_add_thread(protothread_serial);
  
  // === initalize the scheduler ===============
  // method is either:
  //   SCHED_PRIORITY or SCHED_ROUND_ROBIN
  pt_sched_method = SCHED_ROUND_ROBIN ;
  pt_schedule_start ;
  // !!pt_schedule_start NEVER exits
  // ===========================================
} // end main
///////////
// end ////
///////////