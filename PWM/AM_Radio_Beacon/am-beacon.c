/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * AM Radio beacon with PWM
 * 
 * This demonstration uses a PWM channel
 * to generate a 1KHz AM radio beacon.
 * Tune your SDR to 41.667MHz.
 * 
 * HARDWARE CONNECTIONS
 *   - GPIO 4 ---> PWM output
 * 
 * RESOURCES CONSUMED
 *   - 1 repeating timer
 *   - 1 PWM channel
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

// Interface library to sys_clock
#include "hardware/clocks.h"
#include "hardware/pwm.h"

// Low-level alarm infrastructure we'll be using
#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0
#define DELAY 1000 // 1/Fs (in microseconds) - 1KHz tone

// PWM wrap value and clock divide value
// For a CPU rate of 250 MHz, this gives
// a PWM frequency of 50MHz
#define WRAPVAL 5
#define CLKDIV 1.0f
#define DEFAULT_DUTY 3

// PWM pin
#define PWM_PIN 4

// Variable to hold PWM slice number
uint slice_num ;

// DMA channel
int control_chan = 3 ;

// Some state information for use in ISR
unsigned int counter ;
unsigned int state ;

// Alarm ISR
static void alarm_irq(void) {

    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    // Reset the alarm register
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY ;

    // Generate 1KHz tone
    if (counter < 1000){
        if (state) {
            pwm_set_chan_level(slice_num, PWM_CHAN_A, DEFAULT_DUTY) ;
            state = 0 ;
        }
        else {
            pwm_set_chan_level(slice_num, PWM_CHAN_A, 0) ;
            state = 1 ;
        }
    }
    // Generate carrier wave
    else if (counter < 2000) {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, DEFAULT_DUTY) ;
    }
    // Reset
    else {
        counter = 0 ;
    }
    counter += 1 ;
}


int main() {

    // Overclock to 250MHz
    set_sys_clock_khz(250000, true);

    // Initialize stdio
    stdio_init_all();

    ////////////////////////////////////////////////////////////////////////
    ///////////////////////// PWM CONFIGURATION ////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    // Tell GPIO 4 that it is allocated to the PWM, max slew rate and 
    // drive strength
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_PIN, 3);
    gpio_set_slew_rate(PWM_PIN, 1);

    // Find out which PWM slice is connected to GPIO 4 (it's slice 2)
    slice_num = pwm_gpio_to_slice_num(PWM_PIN);

    // This section configures the period of the PWM signals
    pwm_set_wrap(slice_num, WRAPVAL) ;
    pwm_set_clkdiv(slice_num, CLKDIV) ;

    // This sets duty cycle. Will be modified by the DMA channel
    pwm_set_chan_level(slice_num, PWM_CHAN_A, DEFAULT_DUTY);

    // Start the channel
    pwm_set_mask_enabled((1u << slice_num));

    ////////////////////////////////////////////////////////////////////////
    /////////////////////////// REPEATING TIMER ////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    // Enable the interrupt for the alarm (we're using Alarm 0)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM) ;
    // Associate an interrupt handler with the ALARM_IRQ
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq) ;
    // Enable the alarm interrupt
    irq_set_enabled(ALARM_IRQ, true) ;
    // Write the lower 32 bits of the target time to the alarm register, arming it.
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY ;

    while(1) {

    }

}
