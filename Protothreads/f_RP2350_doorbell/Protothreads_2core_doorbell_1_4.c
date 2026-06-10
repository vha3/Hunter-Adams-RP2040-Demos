
/*
 Inter-core iSR the "doorbell"
 
 Core 0:
 -- blinky thread: the usual
 -- doorbell set thread ~100/sec and toggle gpio2

 Core 1:
-- turn off gpio3
-- doorbell irq turns on gpio3

ISR latency on core 1 is measured from the
rising edge of gpio2 to the rising edge of gpio3

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
// the locknumber chosen by system
int doorbell_number ;

// ==================================================
// === toggle25 thread -- core 0 //
// ==================================================
//  blinky
static PT_THREAD (protothread_toggle25(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
    static bool LED_state ;

      while(1) {
        LED_state = !LED_state ;
        gpio_put(LED_PIN, LED_state);       
        PT_YIELD_usec(100000) ;
      } // END WHILE(1)
      // every thread ends with PT_END(pt);
      PT_END(pt);
} // end blink thread

// ==================================================
// === clear gpio3 thread -- core 1
// ==================================================
//  This thread just clears a gpio pin set by an ISR
static PT_THREAD (protothread_gpio3_off(struct pt *pt))
{
    // every thread begins with PT_BEGIN(pt);
    PT_BEGIN(pt);
      while(1) {
        PT_YIELD_usec(2) ;
        // turn off the pin set by the doorbell IRQ
        gpio_put(3, false);
        
        // NEVER exit WHILE in a thread
      } // END WHILE(1)
      // every thread ends with PT_END(pt);
      PT_END(pt);
} // end blink thread

// ==================================================
// === signal doorbell thread -- core 0
// ==================================================
// set core1 IRQ flag
static PT_THREAD (protothread_signal_doorbell(struct pt *pt))
{
    PT_BEGIN(pt);  
      //
      while(1) {
        PT_YIELD_usec(10000);
        // set a timer pin
        gpio_put(2, true);
        // set the core 1 doorbell IRQ flag
        multicore_doorbell_set_other_core(doorbell_number);
        // delay a short time so my crummy scope can see the pulse
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        asm("nop") ;
        gpio_put(2, false);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // serial thread

// ==================================================
// === core 1 doorbell IRQ -- core 1
// ==================================================
// Handle the doorbell request from core0
void core1_doorbell_irq() {
  // turn on pin
  gpio_put(3, true);
  // clear the flag
  multicore_doorbell_clear_current_core(doorbell_number);
}

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main(){ 
    // Setup an interrupt handler for doorbells 
    // - there's actually only one for them all
    // from: pico-examples/multicore/multicore_doorbell/multicore_doorbell.c
    uint32_t irq = multicore_doorbell_irq_num(doorbell_number);
    irq_set_exclusive_handler(irq, core1_doorbell_irq);
    irq_set_enabled(irq, true);

    //  === add threads  ====================
    // for core 1
    pt_add_thread(protothread_gpio3_off);
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

  // set up reporting pins
  gpio_init(LED_PIN) ;	
  gpio_set_dir(LED_PIN, GPIO_OUT) ;
  gpio_init(2) ;	
  gpio_set_dir(2, GPIO_OUT) ;
  gpio_init(3) ;	
  gpio_set_dir(3, GPIO_OUT) ;

  // set up the doorbell number
  #define doorbell_for_core1 0x10
  #define panic_if_no_available_doorbell true
  // second parameter means "panic if none available"
  doorbell_number = multicore_doorbell_claim_unused(doorbell_for_core1, panic_if_no_available_doorbell);

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0 two threads
  pt_add_thread(protothread_toggle25);
  pt_add_thread(protothread_signal_doorbell);
  
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