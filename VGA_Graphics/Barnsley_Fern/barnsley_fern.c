/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * Barnsley Fern calculation and visualization
 * Uses PIO-assembly VGA driver
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 330 ohm resistor ---> VGA Red
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0 and 1
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 */
#include "vga_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// Stuff for Barnsley fern ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Fixed point data type
typedef signed int fix15 ;
#define multfix15(a,b) ((fix15)(((( signed long long)(a))*(( signed long long)(b)))>>15)) 
#define float2fix15(a) ((fix15)((a)*32768.0f)) // 2^15
#define fix2float15(a) ((float)(a)/32768.0f) 
#define int2fix15(a) ((a)<<15)

// Maximum number of iterations
#define max_count 50000


// State transition equations
fix15 f1y_coeff_1 = float2fix15(0.16) ; ;
#define F1x(a,b) 0 ;
#define F1y(a,b) ((fix15)(multfix15(a,f1y_coeff_1)))

fix15 f2x_coeff_1 = float2fix15(0.85) ;
fix15 f2x_coeff_2 = float2fix15(0.04) ;
#define F2x(a,b) ((fix15)(multfix15(f2x_coeff_1,a) + multfix15(f2x_coeff_2,b)))
fix15 f2y_coeff_1 = float2fix15(-0.04) ;
fix15 f2y_coeff_2 = float2fix15(0.85) ;
fix15 f2y_coeff_3 = float2fix15(1.6) ;
#define F2y(a,b) ((fix15)(multfix15(f2y_coeff_1,a) + multfix15(f2y_coeff_2,b) + f2y_coeff_3))

fix15 f3x_coeff_1 = float2fix15(0.2) ;
fix15 f3x_coeff_2 = float2fix15(0.26) ;
#define F3x(a,b) ((fix15)(multfix15(f3x_coeff_1,a) - multfix15(f3x_coeff_2,b)))
fix15 f3y_coeff_1 = float2fix15(0.23) ;
fix15 f3y_coeff_2 = float2fix15(0.22) ;
fix15 f3y_coeff_3 = float2fix15(1.6) ;
#define F3y(a,b) ((fix15)(multfix15(f3y_coeff_1,a) + multfix15(f3y_coeff_2,b) + f3y_coeff_3))

fix15 f4x_coeff_1 = float2fix15(-0.15) ;
fix15 f4x_coeff_2 = float2fix15(0.28) ;
#define F4x(a,b) ((fix15)(multfix15(f4x_coeff_1,a) + multfix15(f4x_coeff_2,b)))
fix15 f4y_coeff_1 = float2fix15(0.26) ;
fix15 f4y_coeff_2 = float2fix15(0.24) ;
fix15 f4y_coeff_3 = float2fix15(0.44) ;
#define F4y(a,b) ((fix15)(multfix15(f4y_coeff_1,a) + multfix15(f4y_coeff_2,b) + f4y_coeff_3))

// Probability thresholds (rearrange for faster execution, check lowest range last)
#define F1_THRESH 21474835
#define F2_THRESH 1846835936
#define F3_THRESH 1997159792

fix15 vga_scale = float2fix15(45.0) ;


////////////////////////////////////////////////////////////////////////////////////////////////////


int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize VGA
    initVGA() ;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===================================== Fern =======================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    fix15 x_old ;
    fix15 y_old ;
    fix15 x_new ;
    fix15 y_new ;

    fix15 scaled_x ;
    fix15 scaled_y ;
    
    x_old = 0 ;
    y_old = 0 ;

    uint32_t start_time ;
    uint32_t end_time ;

    start_time = time_us_32() ;
    
    for (int i=0; i<max_count; i++) {

        int test = rand() ;

        if (test<F1_THRESH) {
            x_new = F1x(x_old, y_old) ;
            y_new = F1y(x_old, y_old) ;
        }
        else if (test<F2_THRESH) {
            x_new = F2x(x_old, y_old) ;
            y_new = F2y(x_old, y_old) ;
        }
        else if (test<F3_THRESH) {
            x_new = F3x(x_old, y_old) ;
            y_new = F3y(x_old, y_old) ;
        }
        else {
            x_new = F4x(x_old, y_old) ;
            y_new = F4y(x_old, y_old) ;
        }

        scaled_x = multfix15(vga_scale, x_new) ;
        scaled_y = multfix15(vga_scale, y_new) ;

        drawPixel((scaled_x>>15) + 320, 460-(scaled_y>>15), GREEN) ;
        
        x_old = x_new ;
        y_old = y_new ;
    }

    end_time = time_us_32() ;
    printf("\n\n Time to compute %d points: %3.6f sec\n\n", max_count, (float)((end_time-start_time)/1000000.)) ;

}
