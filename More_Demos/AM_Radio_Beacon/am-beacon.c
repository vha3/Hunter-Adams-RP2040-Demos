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

#include "hardware/pwm.h"

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

// Timer ISR
bool repeating_timer_callback(struct repeating_timer *t) {

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
    return true;
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
    // Create a repeating timer that calls repeating_timer_callback.
    // If the delay is > 0 then this is the delay between the previous callback ending and the next starting.
    // If the delay is negative (see below) then the next call to the callback will be exactly x us after the
    // start of the call to the last callback
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 25us (40kHz) later regardless of how long the callback took to execute
    add_repeating_timer_us(-1000, repeating_timer_callback, NULL, &timer);

    while(1) {

    }

}
