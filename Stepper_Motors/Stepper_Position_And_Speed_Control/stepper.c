
/**
 * V. Hunter Adams
 * vha3@cornell.edu
 * September, 2021
 * 
 * This demonstrates position and velocity control
 * of two ULN2003 steppers. If each of those steppers
 * is attached to the knob of an etch-a-sketch with a timing
 * belt, this demo draws the Lorenz curve on the etch-a-sketch.
 * 
 * https://vanhunteradams.com/Pico/Steppers/Lorenz.html
 * 
 * 
*/

#include "motor_library.h"
#include <math.h>

#define MOTOR1_IN1 2
#define MOTOR2_IN1 6

#define convert(a) (unsigned int)(125000000./(Fs*(float)a))

#define Fs 50.0

// Variables to hold motor speed
volatile unsigned int motorspeed = 125000;
volatile unsigned int motorspeed2 = 125000 ;

// Variables to track motor direction
volatile int direction = 1 ;
volatile int old_direction = 1 ;
volatile int direction2 = 1 ;
volatile int old_direction2 = 1 ;

// Delay variables
volatile int delay1 = 0 ;
volatile int delay2 = 0 ;

// Sine table values
volatile float sineval ;
volatile float sineval2 ;

// Speed values
volatile float speed ;
volatile float speed2 ;

// Motor directions
unsigned int motor1_direction ;
unsigned int motor2_direction ;

// Lorenz parameters
float sigma = 10.0 ;
float beta = 2.6667 ;
float rho = 28.0 ;
volatile float x1 = -1.0 ;
volatile float x2 = 0.1 ;
volatile float x3 = 25.0 ;
volatile float dx1 ;
volatile float dx2 ;
volatile float dx3 ;
float dt = 0.0002 ;

bool repeating_timer_callback(struct repeating_timer *t) {

    // Compute derivatives
    dx1 = sigma * (x2-x1) ;
    dx2 = x1*(rho-x3) - x2 ;
    dx3 = x1*x2 - beta*x3 ;

    // Integrate equations
    x1 = x1 + dt*dx1 ;
    x2 = x2 + dt*dx2 ;
    x3 = x3 + dt*dx3 ;

    // Determine motor direction
    motor2_direction = (dx1>0) ? CLOCKWISE : COUNTERCLOCKWISE  ;
    motor1_direction = (dx3>0) ? CLOCKWISE : COUNTERCLOCKWISE ;

    // Store motor direction
    direction2 = (dx1>0) ? 1 : 0 ;
    direction = (dx3>0) ? 1 : 0 ;

    // Set motor direction
    SET_DIRECTION_MOTOR_1(motor1_direction) ;
    SET_DIRECTION_MOTOR_2(motor2_direction) ;

    // Scale motor speeds
    speed2 = (dx1>0) ? .09*dx1:-.09*dx1 ;
    speed = (dx3>0) ? .09*dx3:-.09*dx3 ;

    // Delay if direction changed, to pop through hysterisis
    if (direction != old_direction) {
        old_direction = direction ;
        delay1 = 15 ;
    }
    if (direction2 != old_direction2) {
        old_direction2 = direction2 ;
        delay2 = 15 ;
    }

    // Convert speed to wait time
    motorspeed = convert(speed) ;
    motorspeed2 = convert(speed2) ;

    // Clamp at minimum speed
    if (motorspeed > 6250000) {
        SET_DIRECTION_MOTOR_1(STOPPED) ;
        motorspeed = 6250000 ;
    }
    if (motorspeed2 > 6250000) {
        SET_DIRECTION_MOTOR_2(STOPPED) ;
        motorspeed2 = 6250000 ;
    }

    // If necessary, speed through hysterisis
    if (!delay1) {
        SET_SPEED_MOTOR_1(motorspeed) ;
    }
    else {
        SET_SPEED_MOTOR_1(125000) ;
        delay1 -= 1 ;
    }
    if (!delay2) {
        SET_SPEED_MOTOR_2(motorspeed2) ;
    }
    else {
        delay2 -= 1 ;
        SET_SPEED_MOTOR_2(125000) ;
    }

    return true;
}

// Position control interrupts
void pio0_interrupt_handler() {
    pio_interrupt_clear(pio_0, 0) ;
    MOVE_STEPS_MOTOR_1(0xFFFFFFFF) ;
}
void pio1_interrupt_handler() {
    pio_interrupt_clear(pio_1, 0) ;
    MOVE_STEPS_MOTOR_2(0xFFFFFFFF) ;
}

int main() {
    stdio_init_all();

    // Setup each motor
    setupMotor1(MOTOR1_IN1, pio0_interrupt_handler) ;
    setupMotor2(MOTOR2_IN1, pio1_interrupt_handler) ;

    // Create a repeating timer that calls repeating_timer_callback.
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 50ms (20 Hz) later regardless of how long the callback took to execute
    add_repeating_timer_ms(-20, repeating_timer_callback, NULL, &timer);
    
    // Call the interrupt handlers
    pio1_interrupt_handler() ;
    pio0_interrupt_handler() ;

    while (true) {
    }
}
