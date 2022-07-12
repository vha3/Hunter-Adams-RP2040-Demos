
/**
 * V. Hunter Adams
 * vha3@cornell.edu
 * September, 2021
 * 
 * Uses DDS to set the velocity of 4 ULN2003 stepper motors.
 * 
 * https://vanhunteradams.com/Pico/Steppers/Lorenz.html
 * 
*/

#include "motor_library.h"
#include <math.h>

// Motor base pins
#define MOTOR1_IN1 2
#define MOTOR2_IN1 6
#define MOTOR3_IN1 10
#define MOTOR4_IN1 14

// DS parameters
#define two32 4294967296.0 // 2^32 
#define Fs 50.0

// the DDS units:
volatile unsigned int phase_accum_main_0;
volatile unsigned int phase_accum_main_1;
volatile unsigned int phase_accum_main_2;
volatile unsigned int phase_accum_main_3;
volatile unsigned int phase_incr_main_0 = (unsigned int)((.125*two32)/Fs) ;
volatile unsigned int phase_incr_main_1 = (unsigned int)((.0625*two32)/Fs) ;
volatile unsigned int phase_incr_main_2 = (unsigned int)((.03125*two32)/Fs) ;
volatile unsigned int phase_incr_main_3 = (unsigned int)((.015625*two32)/Fs) ;

// DDS sine table
#define sine_table_size 256
volatile float sin_table[sine_table_size] ;

// Variables to hold motor speed
volatile unsigned int motorspeed_0 = 125000;
volatile unsigned int motorspeed_1 = 125000;
volatile unsigned int motorspeed_2 = 125000;
volatile unsigned int motorspeed_3 = 125000 ;

// Sine table values
volatile float sineval_0 ;
volatile float sineval_1 ;
volatile float sineval_2 ;
volatile float sineval_3 ;

// Speed values
volatile float speed_0 ;
volatile float speed_1 ;
volatile float speed_2 ;
volatile float speed_3 ;

// Motor directions
unsigned int motor0_direction ;
unsigned int motor1_direction ;
unsigned int motor2_direction ;
unsigned int motor3_direction ;

// Converts from desired motor velocity (steps/interrupt) to wait time
#define convert(a) (unsigned int)(125000000./(Fs*(float)a))


bool repeating_timer_callback(struct repeating_timer *t) {

    // DDS phase and sine table lookup
    phase_accum_main_0 += phase_incr_main_0  ;
    phase_accum_main_1 += phase_incr_main_1  ;
    phase_accum_main_2 += phase_incr_main_2  ;
    phase_accum_main_3 += phase_incr_main_3  ;

    sineval_0 = sin_table[phase_accum_main_0 >> 24] ;
    sineval_1 = sin_table[phase_accum_main_1 >> 24] ;
    sineval_2 = sin_table[phase_accum_main_2 >> 24] ;
    sineval_3 = sin_table[phase_accum_main_3 >> 24] ;

    motor0_direction = (sineval_0>0) ? CLOCKWISE : COUNTERCLOCKWISE  ;
    motor1_direction = (sineval_1>0) ? CLOCKWISE : COUNTERCLOCKWISE  ;
    motor2_direction = (sineval_2>0) ? CLOCKWISE : COUNTERCLOCKWISE ;
    motor3_direction = (sineval_3>0) ? CLOCKWISE : COUNTERCLOCKWISE ;

    SET_DIRECTION_MOTOR_1(motor0_direction) ;
    SET_DIRECTION_MOTOR_2(motor1_direction) ;
    SET_DIRECTION_MOTOR_3(motor2_direction) ;
    SET_DIRECTION_MOTOR_4(motor3_direction) ;

    speed_0 = (sineval_0>0) ? sineval_0:-sineval_0 ;
    speed_1 = (sineval_1>0) ? sineval_1:-sineval_1 ;
    speed_2 = (sineval_2>0) ? sineval_2:-sineval_2 ;
    speed_3 = (sineval_3>0) ? sineval_3:-sineval_3 ;

    motorspeed_0 = convert(speed_0) ;
    motorspeed_1 = convert(speed_1) ;
    motorspeed_2 = convert(speed_2) ;
    motorspeed_3 = convert(speed_3) ;

    SET_SPEED_MOTOR_1(motorspeed_0) ;
    SET_SPEED_MOTOR_2(motorspeed_1) ;
    SET_SPEED_MOTOR_3(motorspeed_2) ;
    SET_SPEED_MOTOR_4(motorspeed_3) ;

    return true;
}


int main() {
    stdio_init_all();

    // Setup each motor
    setupMotor1(MOTOR1_IN1);//, pio0_interrupt_handler) ;
    setupMotor2(MOTOR2_IN1);//, pio1_interrupt_handler) ;
    setupMotor3(MOTOR3_IN1);//, pio1_interrupt_handler) ;
    setupMotor4(MOTOR4_IN1);

    // === build the sine lookup table =======
    // scaled to produce values between 0 and 20, max number of steps per interrupt
    // at 50Hz
    int ii;
    for (ii = 0; ii < sine_table_size; ii++){
         sin_table[ii] = (int)((20.*sin((float)ii*6.283/(float)sine_table_size)));
    }

    // Create a repeating timer that calls repeating_timer_callback.
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 50ms (20 Hz) later regardless of how long the callback took to execute
    add_repeating_timer_ms(-20, repeating_timer_callback, NULL, &timer);

    while (true) {
    }
}
