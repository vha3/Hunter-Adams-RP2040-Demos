
/**
 * V. Hunter Adams
 * vha3@cornell.edu
 * September, 2021
 * 
 * This demonstrates position control of four
 * ULN2003 stepper motors. The motors are controlled
 * from a collection of PIO state machines, each of
 * which interrupts the C code. Each motor has an
 * associated ISR.
 * 
 * This is for position control only. Speed is fixed.
 * 
 * Each ISR implements a very simple state machine that
 * moves each motor back and forth.
 * 
 * Note that this uses all 12 DMA channels, and all 4
 * state machines on each of the two PIO coprocessors.
 * 
 * https://vanhunteradams.com/Pico/Steppers/Lorenz.html
 * 
*/

#include "motor_library.h"
#include <math.h>

// Base pins for each motor
#define MOTOR1_IN1 2
#define MOTOR2_IN1 6
#define MOTOR3_IN1 10
#define MOTOR4_IN1 14

// State machine variables
volatile int state1 = 0 ;
volatile int state2 = 0 ;
volatile int state3 = 0 ;
volatile int state4 = 0 ;

// A collection of ISR's
void pio0_interrupt_handler() {
    pio_interrupt_clear(pio_0, 1) ;
    if (state1 == 0) {
        SET_DIRECTION_MOTOR_1(COUNTERCLOCKWISE) ;
        state1 = 1 ;
    }
    else if (state1 == 1) {
        SET_DIRECTION_MOTOR_1(CLOCKWISE) ;
        state1 = 2 ;
    }
    else {
        SET_DIRECTION_MOTOR_1(STOPPED) ;
        state1 = 0 ;
    }
    MOVE_STEPS_MOTOR_1(1024) ;
}

void pio0_interrupt_handler_2() {
    pio_interrupt_clear(pio_0, 0) ;
    if (state3 == 0) {
        SET_DIRECTION_MOTOR_3(COUNTERCLOCKWISE) ;
        state3 = 1 ;
    }
    else if (state3 == 1) {
        SET_DIRECTION_MOTOR_3(CLOCKWISE) ;
        state3 = 2 ;
    }
    else {
        SET_DIRECTION_MOTOR_3(STOPPED) ;
        state3 = 0 ;
    }
    MOVE_STEPS_MOTOR_3(4096) ;
}

void pio1_interrupt_handler() {
    pio_interrupt_clear(pio_1, 1) ;
    if (state2 == 0) {
        SET_DIRECTION_MOTOR_2(COUNTERCLOCKWISE) ;
        state2 = 1 ;
    }
    else if (state2 == 1) {
        SET_DIRECTION_MOTOR_2(CLOCKWISE) ;
        state2 = 2 ;
    }
    else {
        SET_DIRECTION_MOTOR_2(STOPPED) ;
        state2 = 0 ;
    }
    MOVE_STEPS_MOTOR_2(2048) ;
}

void pio1_interrupt_handler_2() {
    pio_interrupt_clear(pio_1, 0) ;
    if (state4 == 0) {
        SET_DIRECTION_MOTOR_4(COUNTERCLOCKWISE) ;
        state4 = 1 ;
    }
    else if (state4 == 1) {
        SET_DIRECTION_MOTOR_4(CLOCKWISE) ;
        state4 = 2 ;
    }
    else {
        SET_DIRECTION_MOTOR_4(STOPPED) ;
        state4 = 0 ;
    }
    MOVE_STEPS_MOTOR_4(8192) ;
}

int main() {
    stdio_init_all();

    // Setup each motor
    setupMotor1(MOTOR1_IN1, pio0_interrupt_handler) ;
    setupMotor2(MOTOR2_IN1, pio1_interrupt_handler) ;
    setupMotor3(MOTOR3_IN1, pio0_interrupt_handler_2) ;
    setupMotor4(MOTOR4_IN1, pio1_interrupt_handler_2) ;
    
    // Call the interrupt handlers
    pio1_interrupt_handler() ;
    pio0_interrupt_handler() ;
    pio0_interrupt_handler_2() ;
    pio1_interrupt_handler_2() ;

    while (true) {
    }
}
