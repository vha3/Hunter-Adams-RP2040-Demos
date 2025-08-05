
/*
 Protothreads 1.4 demo code:
 for more info see
 https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html

 TWO threads on ONE core:

 Core 0:
 -- blinky thread: just blinks on rp2040 or rp2350
 -- serial thread: sets the blink period for the other thread

 Core 1:
-- no threads

 */

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <pico/multicore.h>
#include "stdlib.h"

// ==========================================
// === protothreads globals
// ==========================================
// protothreads header
#include "pt_cornell_rp2040_v1_4.h"
# define LED_PIN 25
// the time (in micoseconds) that the blinky thread yields
// defaults to 0.1 second
// this global will be set by the serial thread using user input
int32_t blink_time =100000 ;

// ==================================================
// === toggle25 thread 
// ==================================================
//  
static PT_THREAD (protothread_toggle25(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
    // always use static variables in a thread!!
    // if you eliminate the 'static' watch what happens
    static bool LED_state = false ;
    //
     // set up LED gpio 25
     gpio_init(LED_PIN) ;	
     gpio_set_dir(LED_PIN, GPIO_OUT) ;
     gpio_put(LED_PIN, true);

      while(1) {
        // toggle gpio 25
        LED_state = !LED_state ;
        gpio_put(LED_PIN, LED_state);
        // yield the thread to the scheduler
        // to figure out what to do next
        // https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html
        PT_YIELD_usec(blink_time) ;
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
         sprintf(pt_serial_out_buffer, "enter blink time (uSec)> ") ;
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
        sscanf(pt_serial_in_buffer,"%d", &blink_time) ;
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // serial thread

// ========================================
// === core 0 main
// ========================================
int main(){
  sleep_ms(10);
  //===  start the serial i/o ==================
  stdio_init_all() ;
  // announce the threader version on system reset
  // if there is a seral terminal attached
  printf("\n\rProtothreads RP2040 v1.4 two-core, priority\n\r");

  // === config threads ========================
  // for core 0 two threads
  pt_add_thread(protothread_toggle25);
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