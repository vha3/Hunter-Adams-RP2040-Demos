/**
 * V. Hunter Adams
 * vha3@cornell.edu
 * September, 2021
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "stepper.pio.h"
#include "pacer.pio.h"
#include "counter.pio.h"

// Some macros for motor direction
#define COUNTERCLOCKWISE 2
#define CLOCKWISE 1
#define STOPPED 0

// Forward, reverse, and stopped pulse sequences
unsigned char pulse_sequence_forward [8]  = {0x9, 0x8, 0xc, 0x4, 0x6, 0x2, 0x3, 0x1} ;
unsigned char pulse_sequence_backward [8] = {0x1, 0x3, 0x2, 0x6, 0x4, 0xc, 0x8, 0x9} ;
unsigned char pulse_sequence_stationary [8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} ;

// Unsigned ints to hold pulse length and no. of pulses
unsigned int pulse_length_motor1 = (1u << 17) - 1 ;
unsigned int pulse_count_motor1 = 1024 ;
unsigned int pulse_length_motor2 = (1u << 17) - 1 ;
unsigned int pulse_count_motor2 = 1024 ;

// Pointers to the addresses of the above objects (motor 1)
unsigned char * address_pointer_motor1 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor1_address_pointer = &pulse_length_motor1 ;
unsigned int * pulse_count_motor1_address_pointer = &pulse_count_motor1 ;

// Pointers to the addresses of the above objects (motor 2)
unsigned char * address_pointer_motor2 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor2_address_pointer = &pulse_length_motor2 ;
unsigned int * pulse_count_motor2_address_pointer = &pulse_count_motor2 ;

// Macros for setting motor steps, speed, and direction (motor1)
#define MOVE_STEPS_MOTOR_1(a) pulse_count_motor1=a; dma_channel_start(dma_chan_4)
#define SET_SPEED_MOTOR_1(a) pulse_length_motor1=a
#define SET_DIRECTION_MOTOR_1(a) address_pointer_motor1 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]

// Macros for setting motor steps, speed, and direction (motor2)
#define MOVE_STEPS_MOTOR_2(a) pulse_count_motor2=a; dma_channel_start(dma_chan_9)
#define SET_SPEED_MOTOR_2(a) pulse_length_motor2=a
#define SET_DIRECTION_MOTOR_2(a) address_pointer_motor2 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]

// Choose pio0 or pio1 (we'll have a motor on each)
PIO pio_0 = pio0;
PIO pio_1 = pio1;

// Select state machines
int pulse_sm = 0;
int pacer_sm = 1;
int count_sm = 2;

// DMA channels
// 0 sends pulse train data to motor 1, 1 reconfigures and restarts 0
// 2 sends pulse length data to motor 1, 3 reconfigures and restarts 2
// 4 sends step count to motor 1 (channel started in software)
//
// 5 sends pulse train data to motor 2, 6 reconfigures and restarts 0
// 7 sends pulse length data to motor 2, 8 reconfigures and restarts 2
// 9 sends step count to motor 2 (channel started in software)
int dma_chan_0 = 0;
int dma_chan_1 = 1;
int dma_chan_2 = 2;
int dma_chan_3 = 3;
int dma_chan_4 = 4;
int dma_chan_5 = 5;
int dma_chan_6 = 6;
int dma_chan_7 = 7;
int dma_chan_8 = 8;
int dma_chan_9 = 9;


void setupMotor1(unsigned int in1, irq_handler_t handler) {
    // Load PIO programs onto PIO0
    uint pio0_offset_0 = pio_add_program(pio_0, &stepper_program);
    uint pio0_offset_1 = pio_add_program(pio_0, &pacer_program);
    uint pio0_offset_2 = pio_add_program(pio_0, &counter_program);

    // Initialize PIO programs
    stepper_program_init(pio_0, pulse_sm, pio0_offset_0, in1);
    pacer_program_init(pio_0, pacer_sm, pio0_offset_1);
    counter_program_init(pio_0, count_sm, pio0_offset_2) ;

    // Start the PIO programs
    pio_sm_set_enabled(pio_0, pulse_sm, true);
    pio_sm_set_enabled(pio_0, pacer_sm, true);
    pio_sm_set_enabled(pio_0, count_sm, true);

    // Setup interrupts
    pio_interrupt_clear(pio_0, 0) ;
    pio_set_irq0_source_enabled(pio_0, PIO_INTR_SM0_LSB, true) ;
    irq_set_exclusive_handler(PIO0_IRQ_0, handler) ;
    irq_set_enabled(PIO0_IRQ_0, true) ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel Zero (sends pulse train data to PIO stepper machine)
    dma_channel_config c0 = dma_channel_get_default_config(dma_chan_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c0, true);                        // no read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX0) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    channel_config_set_chain_to(&c0, dma_chan_1);                        // chain to other channel

    dma_channel_configure(
        dma_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio_0->txf[pulse_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor1,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, dma_chan_0);                         // chain to other channel

    dma_channel_configure(
        dma_chan_1,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[dma_chan_0].read_addr,  // Write address (channel 0 read address)
        &address_pointer_motor1,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // ------

    // Channel 2 (sends pulse length data to PIO pacer machine)
    dma_channel_config c2 = dma_channel_get_default_config(dma_chan_2);  // default configs
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c2, false);                        // no read incrementing
    channel_config_set_write_increment(&c2, false);                      // no write incrementing
    channel_config_set_dreq(&c2, DREQ_PIO0_TX1) ;                        // DREQ_PIO0_TX1 pacing (FIFO)
    channel_config_set_chain_to(&c2, dma_chan_3);                        // chain to other channel

    dma_channel_configure(
        dma_chan_2,                 // Channel to be configured
        &c2,                        // The configuration we just created
        &pio_0->txf[pacer_sm],        // write address (pacer PIO TX FIFO)
        pulse_length_motor1_address_pointer,
        1,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 3 (reconfigures the second channel)
    dma_channel_config c3 = dma_channel_get_default_config(dma_chan_3);   // default configs
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c3, false);                        // no read incrementing
    channel_config_set_write_increment(&c3, false);                       // no write incrementing
    channel_config_set_chain_to(&c3, dma_chan_2);                         // chain to other channel

    dma_channel_configure(
        dma_chan_3,                         // Channel to be configured
        &c3,                                // The configuration we just created
        &dma_hw->ch[dma_chan_2].read_addr,  // Write address (channel 2 read address)
        &pulse_length_motor1_address_pointer,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Channel 4 (sends pulse count to PIO counter machine)
    dma_channel_config c4 = dma_channel_get_default_config(dma_chan_4);  // default configs
    channel_config_set_transfer_data_size(&c4, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c4, false);                        // no read incrementing
    channel_config_set_write_increment(&c4, false);                      // no write incrementing

    dma_channel_configure(
        dma_chan_4,                 // Channel to be configured
        &c4,                        // The configuration we just created
        &pio_0->txf[count_sm],        // write address (pacer PIO TX FIFO)
        pulse_count_motor1_address_pointer,
        1,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_0) | (1u << dma_chan_2)) ;
}

void setupMotor2(unsigned int in1, irq_handler_t handler) {

    // Load PIO programs onto PIO1
    uint pio1_offset_0 = pio_add_program(pio_1, &stepper_program);
    uint pio1_offset_1 = pio_add_program(pio_1, &pacer_program);
    uint pio1_offset_2 = pio_add_program(pio_1, &counter_program);

    stepper_program_init(pio_1, pulse_sm, pio1_offset_0, in1);
    pacer_program_init(pio_1, pacer_sm, pio1_offset_1);
    counter_program_init(pio_1, count_sm, pio1_offset_2) ;

    pio_sm_set_enabled(pio_1, pulse_sm, true);
    pio_sm_set_enabled(pio_1, pacer_sm, true);
    pio_sm_set_enabled(pio_1, count_sm, true);

    pio_interrupt_clear(pio_1, 0) ;
    pio_set_irq0_source_enabled(pio_1, PIO_INTR_SM0_LSB, true) ;
    irq_set_exclusive_handler(PIO1_IRQ_0, handler) ;
    irq_set_enabled(PIO1_IRQ_0, true) ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel 5 (sends pulse train data to PIO stepper machine)
    dma_channel_config c5 = dma_channel_get_default_config(dma_chan_5);  // default configs
    channel_config_set_transfer_data_size(&c5, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c5, true);                        // no read incrementing
    channel_config_set_write_increment(&c5, false);                      // no write incrementing
    channel_config_set_dreq(&c5, DREQ_PIO1_TX0) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    channel_config_set_chain_to(&c5, dma_chan_6);                        // chain to other channel

    dma_channel_configure(
        dma_chan_5,                 // Channel to be configured
        &c5,                        // The configuration we just created
        &pio_1->txf[pulse_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor2,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c6 = dma_channel_get_default_config(dma_chan_6);   // default configs
    channel_config_set_transfer_data_size(&c6, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c6, false);                        // no read incrementing
    channel_config_set_write_increment(&c6, false);                       // no write incrementing
    channel_config_set_chain_to(&c6, dma_chan_5);                         // chain to other channel

    dma_channel_configure(
        dma_chan_6,                         // Channel to be configured
        &c6,                                // The configuration we just created
        &dma_hw->ch[dma_chan_5].read_addr,  // Write address (channel 0 read address)
        &address_pointer_motor2,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // ------

    // Channel 2 (sends pulse length data to PIO pacer machine)
    dma_channel_config c7 = dma_channel_get_default_config(dma_chan_7);  // default configs
    channel_config_set_transfer_data_size(&c7, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c7, false);                        // no read incrementing
    channel_config_set_write_increment(&c7, false);                      // no write incrementing
    channel_config_set_dreq(&c7, DREQ_PIO1_TX1) ;                        // DREQ_PIO0_TX1 pacing (FIFO)
    channel_config_set_chain_to(&c7, dma_chan_8);                        // chain to other channel

    dma_channel_configure(
        dma_chan_7,                 // Channel to be configured
        &c7,                        // The configuration we just created
        &pio_1->txf[pacer_sm],        // write address (pacer PIO TX FIFO)
        pulse_length_motor2_address_pointer,
        1,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 3 (reconfigures the second channel)
    dma_channel_config c8 = dma_channel_get_default_config(dma_chan_8);   // default configs
    channel_config_set_transfer_data_size(&c8, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c8, false);                        // no read incrementing
    channel_config_set_write_increment(&c8, false);                       // no write incrementing
    channel_config_set_chain_to(&c8, dma_chan_7);                         // chain to other channel

    dma_channel_configure(
        dma_chan_8,                         // Channel to be configured
        &c8,                                // The configuration we just created
        &dma_hw->ch[dma_chan_7].read_addr,  // Write address (channel 2 read address)
        &pulse_length_motor2_address_pointer,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Channel 4 (sends pulse count to PIO counter machine)
    dma_channel_config c9 = dma_channel_get_default_config(dma_chan_9);  // default configs
    channel_config_set_transfer_data_size(&c9, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c9, false);                        // no read incrementing
    channel_config_set_write_increment(&c9, false);                      // no write incrementing

    dma_channel_configure(
        dma_chan_9,                 // Channel to be configured
        &c9,                        // The configuration we just created
        &pio_1->txf[count_sm],        // write address (pacer PIO TX FIFO)
        pulse_count_motor2_address_pointer,
        1,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_5) | (1u << dma_chan_7)) ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////
}

