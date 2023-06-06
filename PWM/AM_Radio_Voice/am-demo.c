/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * AM Radio transmission with PWM
 * 
 * This demonstration uses a PWM channel
 * to generate an AM radio transmission modulated
 * by an ADC input. Tune your radio to 980kHz.
 * 
 * HARDWARE CONNECTIONS
 *   - GPIO 4 ---> PWM output
 *   - GPIO 25 --> ADC input
 * 
 * RESOURCES CONSUMED
 *   - ADC
 *   - 2 DMA channels
 *   - 1 PWM channel
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/adc.h"

// PWM wrap value and clock divide value
// For a CPU rate of 250 MHz, this gives
// a PWM frequency of ~980kHz
#define WRAPVAL 255
#define CLKDIV 1.0f

// ADC Mux input 0, on GPIO 26
// Sample rate of 10KHz, ADC clock rate of 48MHz
#define ADC_CHAN 0
#define ADC_PIN 26
#define Fs 10000.0
#define ADCCLK 48000000.0

// PWM pin
#define PWM_PIN 4


// Variable to hold PWM slice number
uint slice_num ;


// Array for shuffling ADC sample
unsigned char new_duty[4] = {64, 0, 0, 0} ;


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
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 128);

    // Start the channel
    pwm_set_mask_enabled((1u << slice_num));

    ///////////////////////////////////////////////////////////////////////////////
    // ============================== ADC CONFIGURATION ==========================
    //////////////////////////////////////////////////////////////////////////////
    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_gpio_init(ADC_PIN);

    // Initialize the ADC harware
    // (resets it, enables the clock, spins until the hardware is ready)
    adc_init() ;

    // Select analog mux input (0...3 are GPIO 26, 27, 28, 29; 4 is temp sensor)
    adc_select_input(ADC_CHAN) ;

    // Setup the FIFO
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        true     // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock. This is setup
    // to grab a sample at 10kHz (48Mhz/10kHz - 1)
    adc_set_clkdiv(ADCCLK/Fs);

    ///////////////////////////////////////////////////////////////////////
    // ============================== DMA CONFIGURATION ===================
    ///////////////////////////////////////////////////////////////////////
    
    // DMA channels for sampling ADC. Using 2 and 3 in case I want to add
    // the VGA driver to this project
    int sample_chan = 2 ;
    int control_chan = 3 ;

    // Channel configurations (start with the default)
    dma_channel_config c2 = dma_channel_get_default_config(sample_chan);
    dma_channel_config c3 = dma_channel_get_default_config(control_chan);

    // Setup the ADC sample channel
    // Reading from constant address, in 8-bit chunks, writing to constant address
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_8);
    channel_config_set_read_increment(&c2, false);
    channel_config_set_write_increment(&c2, false);
    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&c2, DREQ_ADC);
    // Chain to control channel
    channel_config_set_chain_to(&c2, control_chan);
    // Configure the channel
    dma_channel_configure(sample_chan,
        &c2,                // channel config
        &new_duty[0],       // dst
        &adc_hw->fifo,      // src
        1,                  // transfer count
        false               // don't start immediately
    );

    // Setup the control channel
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_32);  // 32-bit txfers
    channel_config_set_read_increment(&c3, false);            // no read incrementing
    channel_config_set_write_increment(&c3, false);           // no write incrementing
    channel_config_set_chain_to(&c3, sample_chan);            // chain to sample chan

    dma_channel_configure(
        control_chan,                  // Channel to be configured
        &c3,                           // The configuration we just created
        &pwm_hw->slice[slice_num].cc,  // Write address (PWM counter compare reg)
        &new_duty[0],                  // Read address (where the other DMA put the data)
        1,                             // Number of transfers
        false                          // Don't start immediately
    );

    // Start the DMA channel (before the ADC!)
    dma_start_channel_mask(1u << sample_chan) ;

    // Start the ADC
    adc_run(true) ;

    // Exit main :)

}
