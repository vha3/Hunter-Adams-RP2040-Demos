
/**
 * V. Hunter Adams
 * vha3@cornell.edu
 * September, 2021
 * 
 * This is a simple application for the stepper motor
 * driver, which moves a 3D-printed delta robot thru
 * a sample maneuver.
 * 
 * See link below.
 * 
 * https://vanhunteradams.com/Pico/Steppers/Lorenz.html#Delta-Robot
 * 
*/

#include "motor_library.h"
#include <math.h>

#define MOTOR1_IN1 2
#define MOTOR2_IN1 6
#define MOTOR3_IN1 10
// #define MOTOR4_IN1 14

volatile int state1 = 0 ;
volatile int state2 = 0 ;
volatile int state3 = 0 ;
// volatile int state4 = 0 ;


void pio0_interrupt_handler() {
    pio_interrupt_clear(pio_0, 1) ;
    if (state1 == 0) {
        SET_DIRECTION_MOTOR_1(COUNTERCLOCKWISE) ;
        state1 = 1 ;
        MOVE_STEPS_MOTOR_1(600) ;
    }
    else if (state1 == 1) {
        SET_DIRECTION_MOTOR_1(CLOCKWISE) ;
        state1 = 2 ;
        MOVE_STEPS_MOTOR_1(200) ;
    }
    else if (state1 == 2) {
        SET_DIRECTION_MOTOR_1(COUNTERCLOCKWISE) ;
        state1 = 3 ;
        MOVE_STEPS_MOTOR_1(200) ;
    }
    else if (state1 == 3) {
        SET_DIRECTION_MOTOR_1(STOPPED) ;
        state1 = 4 ;
        MOVE_STEPS_MOTOR_1(800) ;
    }
    else if (state1 == 4) {
        SET_DIRECTION_MOTOR_1(CLOCKWISE) ;
        state1 = 5 ;
        MOVE_STEPS_MOTOR_1(800) ;
    }
    else if (state1 == 5) {
        SET_DIRECTION_MOTOR_1(COUNTERCLOCKWISE) ;
        state1 = 6 ;
        MOVE_STEPS_MOTOR_1(200) ;
    }
    else {
        SET_DIRECTION_MOTOR_1(STOPPED) ;
        state1 = 0 ;
        MOVE_STEPS_MOTOR_1(1000) ;
    }
    // MOVE_STEPS_MOTOR_1(600) ;
}

void pio0_interrupt_handler_2() {
    pio_interrupt_clear(pio_0, 0) ;
    if (state3 == 0) {
        SET_DIRECTION_MOTOR_3(COUNTERCLOCKWISE) ;
        state3 = 1 ;
        MOVE_STEPS_MOTOR_3(600) ;
    }
    else if (state3 == 1) {
        SET_DIRECTION_MOTOR_3(STOPPED) ;
        state3 = 2 ;
        MOVE_STEPS_MOTOR_3(400) ;
    }
    else if (state3 == 2) {
        SET_DIRECTION_MOTOR_3(CLOCKWISE) ;
        state3 = 3 ;
        MOVE_STEPS_MOTOR_3(200) ;
    }
    else if (state3 == 3) {
        SET_DIRECTION_MOTOR_3(COUNTERCLOCKWISE) ;
        state3 = 4 ;
        MOVE_STEPS_MOTOR_3(200) ;
    }
    else if (state3 == 4) {
        SET_DIRECTION_MOTOR_3(STOPPED) ;
        state3 = 5 ;
        MOVE_STEPS_MOTOR_3(400) ;
    }
    else if (state3 == 5) {
        SET_DIRECTION_MOTOR_3(CLOCKWISE) ;
        state3 = 6 ;
        MOVE_STEPS_MOTOR_3(800) ;
    }
    else if (state3 == 6) {
        SET_DIRECTION_MOTOR_3(COUNTERCLOCKWISE) ;
        state3 = 7 ;
        MOVE_STEPS_MOTOR_3(200) ;
    }
    else {
        SET_DIRECTION_MOTOR_3(STOPPED) ;
        state3 = 0 ;
        MOVE_STEPS_MOTOR_3(1000) ;
    }
    // MOVE_STEPS_MOTOR_3(600) ;
}

void pio1_interrupt_handler() {
    pio_interrupt_clear(pio_1, 1) ;
    if (state2 == 0) {
        SET_DIRECTION_MOTOR_2(COUNTERCLOCKWISE) ;
        state2 = 1 ;
        MOVE_STEPS_MOTOR_2(600) ;
    }
    else if (state2 == 1) {
        SET_DIRECTION_MOTOR_2(STOPPED) ;
        state2 = 2 ;
        MOVE_STEPS_MOTOR_2(800) ;
    }
    else if (state2 == 2) {
        SET_DIRECTION_MOTOR_2(CLOCKWISE) ;
        state2 = 3 ;
        MOVE_STEPS_MOTOR_2(200) ;
    }
    else if (state2 == 3) {
        SET_DIRECTION_MOTOR_2(COUNTERCLOCKWISE) ;
        state2 = 4 ;
        MOVE_STEPS_MOTOR_2(200) ;
    }
    else if (state2 == 4) {
        SET_DIRECTION_MOTOR_2(CLOCKWISE) ;
        state2 = 5 ;
        MOVE_STEPS_MOTOR_2(800) ;
    }
    else if (state2 == 5) {
        SET_DIRECTION_MOTOR_2(COUNTERCLOCKWISE) ;
        state2 = 6 ;
        MOVE_STEPS_MOTOR_2(200) ;
    }
    else {
        SET_DIRECTION_MOTOR_2(STOPPED) ;
        state2 = 0 ;
        MOVE_STEPS_MOTOR_2(1000) ;
    }
    // MOVE_STEPS_MOTOR_2(600) ;
}


int main() {
    stdio_init_all();

    // Setup each motor
    setupMotor1(MOTOR1_IN1, pio0_interrupt_handler) ;
    setupMotor2(MOTOR2_IN1, pio1_interrupt_handler) ;
    setupMotor3(MOTOR3_IN1, pio0_interrupt_handler_2) ;
    
    // Call the interrupt handlers
    pio1_interrupt_handler() ;
    pio0_interrupt_handler() ;
    pio0_interrupt_handler_2() ;

    while (true) {
    }
}
