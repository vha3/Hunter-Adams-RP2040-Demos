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
#include "stepper1.pio.h"
#include "pacer1.pio.h"

// Some macros for motor direction
#define COUNTERCLOCKWISE 2
#define CLOCKWISE 1
#define STOPPED 0

// Forward, reverse, and stopped pulse sequences
unsigned char pulse_sequence_forward [8]  = {0x9, 0x8, 0xc, 0x4, 0x6, 0x2, 0x3, 0x1} ;
unsigned char pulse_sequence_backward [8] = {0x1, 0x3, 0x2, 0x6, 0x4, 0xc, 0x8, 0x9} ;
unsigned char pulse_sequence_stationary [8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} ;

// Unsigned ints to hold pulse length and no. of pulses
unsigned int pulse_length_motor1 = 125000 ;
unsigned int pulse_length_motor2 = 125000 ;
unsigned int pulse_length_motor3 = 125000 ;
unsigned int pulse_length_motor4 = 125000 ;

// Pointers to the addresses of the above objects (motor 1)
unsigned char * address_pointer_motor1 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor1_address_pointer = &pulse_length_motor1 ;

// Pointers to the addresses of the above objects (motor 2)
unsigned char * address_pointer_motor2 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor2_address_pointer = &pulse_length_motor2 ;

// Pointers to the addresses of the above objects (motor 3)
unsigned char * address_pointer_motor3 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor3_address_pointer = &pulse_length_motor3 ;

// Pointers to the addresses of the above objects (motor 4)
unsigned char * address_pointer_motor4 = &pulse_sequence_forward[0] ;
unsigned int * pulse_length_motor4_address_pointer = &pulse_length_motor4 ;

// Macros for setting motor steps, speed, and direction (motor1)
#define SET_SPEED_MOTOR_1(a) pulse_length_motor1=a
#define SET_DIRECTION_MOTOR_1(a) address_pointer_motor1 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]

// Macros for setting motor steps, speed, and direction (motor2)
#define SET_SPEED_MOTOR_2(a) pulse_length_motor2=a
#define SET_DIRECTION_MOTOR_2(a) address_pointer_motor2 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]

// Macros for setting motor steps, speed, and direction (motor3)
#define SET_SPEED_MOTOR_3(a) pulse_length_motor3=a
#define SET_DIRECTION_MOTOR_3(a) address_pointer_motor3 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]

// Macros for setting motor steps, speed, and direction (motor4)
#define SET_SPEED_MOTOR_4(a) pulse_length_motor4=a
#define SET_DIRECTION_MOTOR_4(a) address_pointer_motor4 = (a==2) ? &pulse_sequence_forward[0] : (a==1) ? &pulse_sequence_backward[0] : &pulse_sequence_stationary[0]


// Choose pio0 or pio1 (we'll have a motor on each)
PIO pio_0 = pio0;
PIO pio_1 = pio1;

// Select state machines
int pulse_sm = 0;
int pacer_sm = 1;
int pulse1_sm = 2;
int pacer1_sm = 3;

// DMA channels
// 0 sends pulse train data to motor 1, 1 sends pulse length data to motor 1
// 2 restarts 1 (chained) and 0 (writes to read address trig register)
//
// 5 sends pulse train data to motor 2, 6 reconfigures and restarts 0
// 7 sends pulse length data to motor 2, 8 reconfigures and restarts 2
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
int dma_chan_10 = 10;
int dma_chan_11 = 11;



void setupMotor1(unsigned int in1) {//, irq_handler_t handler) {
    // Load PIO programs onto PIO0
    uint pio0_offset_0 = pio_add_program(pio_0, &stepper_program);
    uint pio0_offset_1 = pio_add_program(pio_0, &pacer_program);

    // Initialize PIO programs
    stepper_program_init(pio_0, pulse_sm, pio0_offset_0, in1);
    pacer_program_init(pio_0, pacer_sm, pio0_offset_1);

    // Start the PIO programs
    pio_sm_set_enabled(pio_0, pulse_sm, true);
    pio_sm_set_enabled(pio_0, pacer_sm, true);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel Zero (sends pulse train data to PIO stepper machine)
    dma_channel_config c0 = dma_channel_get_default_config(dma_chan_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c0, true);                        // no read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX0) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    // channel_config_set_chain_to(&c0, dma_chan_1);                        // chain to other channel

    dma_channel_configure(
        dma_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio_0->txf[pulse_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor1,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // ------

    // Channel 1 (sends pulse length data to PIO pacer machine)
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_1);  // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                      // no write incrementing
    channel_config_set_dreq(&c1, DREQ_PIO0_TX1) ;                        // DREQ_PIO0_TX1 pacing (FIFO)
    channel_config_set_chain_to(&c1, dma_chan_2);                        // chain to other channel

    dma_channel_configure(
        dma_chan_1,                 // Channel to be configured
        &c1,                        // The configuration we just created
        &pio_0->txf[pacer_sm],        // write address (pacer PIO TX FIFO)
        pulse_length_motor1_address_pointer,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 2 (reconfigures both channels)
    dma_channel_config c2 = dma_channel_get_default_config(dma_chan_2);   // default configs
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c2, false);                        // no read incrementing
    channel_config_set_write_increment(&c2, false);                       // no write incrementing
    channel_config_set_chain_to(&c2, dma_chan_1);                         // chain to other channel

    dma_channel_configure(
        dma_chan_2,                         // Channel to be configured
        &c2,                                // The configuration we just created
        &dma_hw->ch[dma_chan_0].al3_read_addr_trig,  // Write address (channel 2 read address)
        &address_pointer_motor1,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_0) | (1u << dma_chan_1)) ;
}

void setupMotor3(unsigned int in1) {//, irq_handler_t handler) {
    // Load PIO programs onto PIO0
    uint pio0_offset_2 = pio_add_program(pio_0, &stepper1_program);
    uint pio0_offset_3 = pio_add_program(pio_0, &pacer1_program);

    // Initialize PIO programs
    stepper1_program_init(pio_0, pulse1_sm, pio0_offset_2, in1);
    pacer1_program_init(pio_0, pacer1_sm, pio0_offset_3);

    // Start the PIO programs
    pio_sm_set_enabled(pio_0, pulse1_sm, true);
    pio_sm_set_enabled(pio_0, pacer1_sm, true);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel Zero (sends pulse train data to PIO stepper machine)
    dma_channel_config c3 = dma_channel_get_default_config(dma_chan_3);  // default configs
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c3, true);                        // no read incrementing
    channel_config_set_write_increment(&c3, false);                      // no write incrementing
    channel_config_set_dreq(&c3, DREQ_PIO0_TX2) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    // channel_config_set_chain_to(&c3, dma_chan_1);                        // chain to other channel

    dma_channel_configure(
        dma_chan_3,                 // Channel to be configured
        &c3,                        // The configuration we just created
        &pio_0->txf[pulse1_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor3,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // ------

    // Channel 1 (sends pulse length data to PIO pacer machine)
    dma_channel_config c4 = dma_channel_get_default_config(dma_chan_4);  // default configs
    channel_config_set_transfer_data_size(&c4, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c4, false);                        // no read incrementing
    channel_config_set_write_increment(&c4, false);                      // no write incrementing
    channel_config_set_dreq(&c4, DREQ_PIO0_TX3) ;                        // DREQ_PIO0_TX1 pacing (FIFO)
    channel_config_set_chain_to(&c4, dma_chan_5);                        // chain to other channel

    dma_channel_configure(
        dma_chan_4,                 // Channel to be configured
        &c4,                        // The configuration we just created
        &pio_0->txf[pacer1_sm],        // write address (pacer PIO TX FIFO)
        pulse_length_motor3_address_pointer,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 2 (reconfigures both channels)
    dma_channel_config c5 = dma_channel_get_default_config(dma_chan_5);   // default configs
    channel_config_set_transfer_data_size(&c5, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c5, false);                        // no read incrementing
    channel_config_set_write_increment(&c5, false);                       // no write incrementing
    channel_config_set_chain_to(&c5, dma_chan_4);                         // chain to other channel

    dma_channel_configure(
        dma_chan_5,                         // Channel to be configured
        &c5,                                // The configuration we just created
        &dma_hw->ch[dma_chan_3].al3_read_addr_trig,  // Write address (channel 2 read address)
        &address_pointer_motor3,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_3) | (1u << dma_chan_4)) ;
}

void setupMotor2(unsigned int in1){//, irq_handler_t handler) {

    // Load PIO programs onto PIO1
    uint pio1_offset_0 = pio_add_program(pio_1, &stepper_program);
    uint pio1_offset_1 = pio_add_program(pio_1, &pacer_program);

    stepper_program_init(pio_1, pulse_sm, pio1_offset_0, in1);
    pacer_program_init(pio_1, pacer_sm, pio1_offset_1);

    pio_sm_set_enabled(pio_1, pulse_sm, true);
    pio_sm_set_enabled(pio_1, pacer_sm, true);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel 5 (sends pulse train data to PIO stepper machine)
    dma_channel_config c6 = dma_channel_get_default_config(dma_chan_6);  // default configs
    channel_config_set_transfer_data_size(&c6, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c6, true);                        // no read incrementing
    channel_config_set_write_increment(&c6, false);                      // no write incrementing
    channel_config_set_dreq(&c6, DREQ_PIO1_TX0) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    // channel_config_set_chain_to(&c6, dma_chan_7);                        // chain to other channel

    dma_channel_configure(
        dma_chan_6,                 // Channel to be configured
        &c6,                        // The configuration we just created
        &pio_1->txf[pulse_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor2,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // ------

    // Channel 7 (sends pulse length data to PIO pacer machine)
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
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 8 (reconfigures each other channel)
    dma_channel_config c8 = dma_channel_get_default_config(dma_chan_8);   // default configs
    channel_config_set_transfer_data_size(&c8, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c8, false);                        // no read incrementing
    channel_config_set_write_increment(&c8, false);                       // no write incrementing
    channel_config_set_chain_to(&c8, dma_chan_7);                         // chain to other channel

    dma_channel_configure(
        dma_chan_8,                         // Channel to be configured
        &c8,                                // The configuration we just created
        &dma_hw->ch[dma_chan_6].al3_read_addr_trig,  // Write address (channel 2 read address)
        &address_pointer_motor2,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_6) | (1u << dma_chan_7)) ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////
}

void setupMotor4(unsigned int in1){//, irq_handler_t handler) {

    // Load PIO programs onto PIO1
    uint pio1_offset_2 = pio_add_program(pio_1, &stepper1_program);
    uint pio1_offset_3 = pio_add_program(pio_1, &pacer1_program);

    stepper1_program_init(pio_1, pulse1_sm, pio1_offset_2, in1);
    pacer1_program_init(pio_1, pacer1_sm, pio1_offset_3);

    pio_sm_set_enabled(pio_1, pulse1_sm, true);
    pio_sm_set_enabled(pio_1, pacer1_sm, true);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===========================-== DMA Data Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Channel 9 (sends pulse train data to PIO stepper machine)
    dma_channel_config c9 = dma_channel_get_default_config(dma_chan_9);  // default configs
    channel_config_set_transfer_data_size(&c9, DMA_SIZE_8);              // 32-bit txfers
    channel_config_set_read_increment(&c9, true);                        // no read incrementing
    channel_config_set_write_increment(&c9, false);                      // no write incrementing
    channel_config_set_dreq(&c9, DREQ_PIO1_TX2) ;                        // DREQ_PIO0_TX0 pacing (FIFO)
    // channel_config_set_chain_to(&c9, dma_chan_7);                        // chain to other channel

    dma_channel_configure(
        dma_chan_9,                 // Channel to be configured
        &c9,                        // The configuration we just created
        &pio_1->txf[pulse1_sm],        // write address (stepper PIO TX FIFO)
        address_pointer_motor4,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // ------

    // Channel 10 (sends pulse length data to PIO pacer machine)
    dma_channel_config c10 = dma_channel_get_default_config(dma_chan_10);  // default configs
    channel_config_set_transfer_data_size(&c10, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c10, false);                        // no read incrementing
    channel_config_set_write_increment(&c10, false);                      // no write incrementing
    channel_config_set_dreq(&c10, DREQ_PIO1_TX3) ;                        // DREQ_PIO0_TX1 pacing (FIFO)
    channel_config_set_chain_to(&c10, dma_chan_11);                        // chain to other channel

    dma_channel_configure(
        dma_chan_10,                 // Channel to be configured
        &c10,                        // The configuration we just created
        &pio_1->txf[pacer1_sm],        // write address (pacer PIO TX FIFO)
        pulse_length_motor4_address_pointer,
        8,                          // Number of transfers; in this case each is 4 byte.
        false                       // Don't start immediately.
    );

    // Channel 8 (reconfigures each other channel)
    dma_channel_config c11 = dma_channel_get_default_config(dma_chan_11);   // default configs
    channel_config_set_transfer_data_size(&c11, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c11, false);                        // no read incrementing
    channel_config_set_write_increment(&c11, false);                       // no write incrementing
    channel_config_set_chain_to(&c11, dma_chan_10);                         // chain to other channel

    dma_channel_configure(
        dma_chan_11,                         // Channel to be configured
        &c11,                                // The configuration we just created
        &dma_hw->ch[dma_chan_9].al3_read_addr_trig,  // Write address (channel 2 read address)
        &address_pointer_motor4,      // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // -------

    // Start the two data channels
    dma_start_channel_mask((1u << dma_chan_9) | (1u << dma_chan_10)) ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////
}

