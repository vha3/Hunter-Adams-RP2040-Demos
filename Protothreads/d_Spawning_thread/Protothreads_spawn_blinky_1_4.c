
/*
 Protothreads 1.4 demo code:
 for more info see
 https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html

 TWO thread on ONE core
 BUT one of the threads is 'spawned', not scheduled!

 Core 0:
 -- blinky thread: just blinks on rp2040
 -- thread spawned by blinky

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

// ==================================================
// ===  thread spawned by protothread_toggle25
// ==================================================
//  === NOTE that this thread is NOT scheduled in main ===
// protothreads control structure define for the thread
// -- this structure is abstracted away for the scheduleer
// -- but must be specified for a spawned thread
static struct pt spawn_struct ;
// the actual thread to spawn:
// there is NO while(true) loop because this thread
// MUST EXIT so that the parent thread can continue running!
static PT_THREAD (protothread_spawn_blink (struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
    // always use static variables in a thread!!
    // if you eliminate the 'static' watch what happens
    static bool LED_state ;
    //
    // toggle gpio 25
    LED_state = !LED_state ;
    gpio_put(LED_PIN, LED_state);
    
    // this thread exits back to the thread that spawned it
    PT_EXIT(pt)
    // every thread ends with PT_END(pt);
    PT_END(pt);
} // end spawned  thread

// ==================================================
// === toggle25 thread 
// ==================================================
//  This thread sets up an i/o pin then just 
// spawns a thread that actually toggle the LED

static PT_THREAD (protothread_toggle25(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);//
    PT_BEGIN(pt);
     // set up LED gpio 25
     gpio_init(LED_PIN) ;	
     gpio_set_dir(LED_PIN, GPIO_OUT) ;
     gpio_put(LED_PIN, true);

      while(1) {
        // spawn a thread to do the actual toggle gpio 25
        PT_SPAWN(pt, &spawn_struct, protothread_spawn_blink(&spawn_struct)) ;
        
        // yield the thread to the scheduler
        // so to figure out what to do next
        // see https://people.ece.cornell.edu/land/courses/ece4760/RP2040/protothreads_1_4/index_Protothreads_1_4.html
        PT_YIELD_usec(100000) ;
        // NEVER exit WHILE in a thread
      } // END WHILE(1)
      // every thread ends with PT_END(pt);
      PT_END(pt);
} // end blink thread

// ========================================
// === core 0 main
// ========================================
int main(){
  sleep_ms(10);
  //===  start the serial i/o ==================
  stdio_init_all() ;
  // announce the threader version on system reset
  // if there is a seral terminal attached
  printf("\n\rProtothreads RP2040 v1.3 two-core, priority\n\r");

  // === config threads ========================
  // for core 0
  pt_add_thread(protothread_toggle25);
  
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