/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * Mandelbrot set calculation and visualization
 * Uses PIO-assembly VGA driver.
 * 
 * Core 1 draws the bottom half of the set using floating point.
 * Core 0 draws the top half of the set using fixed point.
 * This illustrates the speed improvement of fixed point over floating point.
 * 
 * https://vanhunteradams.com/FixedPoint/FixedPoint.html
 * https://vanhunteradams.com/Pico/VGA/VGA.html
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
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// Stuff for Mandelbrot ///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Fixed point data type
typedef signed int fix28 ;
#define multfix28(a,b) ((fix28)(((( signed long long)(a))*(( signed long long)(b)))>>28)) 
#define float2fix28(a) ((fix28)((a)*268435456.0f)) // 2^28
#define fix2float28(a) ((float)(a)/268435456.0f) 
#define int2fix28(a) ((a)<<28)
// the fixed point value 4
#define FOURfix28 0x40000000 
#define SIXTEENTHfix28 0x01000000
#define ONEfix28 0x10000000

// Maximum number of iterations
#define max_count 1000

////////////////////////////////////////////////////////////////////////////////////////////////////
// Core 1 entry point
void core1_entry() {
    // Mandelbrot values
    float Zre, Zim, Cre, Cim ;
    float Zre_sq, Zim_sq ;

    int i, j, count, total_count ;

    float x[640] ;
    float y[240] ;
    uint32_t begin_time ;
    uint32_t end_time ;
    uint32_t total_time ;
    while (true) {

        // x values
        for (i=0; i<640; i++) {
            x[i] = (-2.0f + 3.0f * (float)i/640.0f) ;
        }
        
        // y values
        for (j=0; j<240; j++) {
            y[j] = ( 1.0f - 2.0f * (float)(j+240)/480.0f) ;
        }

        total_count = 0 ;

        begin_time = time_us_32() ;

        for (i=0; i<640; i++) {
            
            for (j=239; j>-1; j--) {

                Zre = Zre_sq = Zim = Zim_sq = 0 ;

                Cre = x[i] ;
                Cim = y[j] ;

                count = 0 ;

                // Mandelbrot iteration
                while (count++ < max_count) {
                    Zim = (2*Zre*Zim) + Cim ;
                    Zre = Zre_sq - Zim_sq + Cre ;
                    Zre_sq = (Zre * Zre) ;
                    Zim_sq = (Zim * Zim) ;

                    if ((Zre_sq + Zim_sq) >= 4.0) break ;
                }
                // Increment total count
                total_count += count ;

                // Draw the pixel
                if (count >= max_count) drawPixel(i, j+240, BLACK) ;
                else if (count >= (max_count>>1)) drawPixel(i, j+240, WHITE) ;
                else if (count >= (max_count>>2)) drawPixel(i, j+240, CYAN) ;
                else if (count >= (max_count>>3)) drawPixel(i, j+240, BLUE) ;
                else if (count >= (max_count>>4)) drawPixel(i, j+240, RED) ;
                else if (count >= (max_count>>5)) drawPixel(i, j+240, YELLOW) ;
                else if (count >= (max_count>>6)) drawPixel(i, j+240, MAGENTA) ;
                else drawPixel(i, j+240, RED) ;

            }
        }

        end_time = time_us_32() ;
        total_time = end_time - begin_time ;
        multicore_fifo_push_blocking(total_time) ;
    }
}

int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize VGA
    initVGA() ;

    // Launch core 1
    multicore_launch_core1(core1_entry) ;


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ===================================== Mandelbrot =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // Mandelbrot values
    fix28 Zre, Zim, Cre, Cim ;
    fix28 Zre_sq, Zim_sq ;

    int i, j, count, total_count ;

    fix28 x[640] ;
    fix28 y[240] ;
    uint32_t begin_time ;
    uint32_t end_time ;
    uint32_t core1_time ;
    float total_time_core_0 ;
    float total_time_core_1 ;
    while (true) {

        // x values
        for (i=0; i<640; i++) {
            x[i] = float2fix28(-2.0f + 3.0f * (float)i/640.0f) ;
        }
        
        // y values
        for (j=0; j<240; j++) {
            y[j] = float2fix28( 1.0f - 2.0f * (float)j/480.0f) ;
        }

        total_count = 0 ;


        begin_time = time_us_32() ;

        for (i=0; i<640; i++) {
            
            for (j=0; j<240; j++) {

                Zre = Zre_sq = Zim = Zim_sq = 0 ;

                Cre = x[i] ;
                Cim = y[j] ;

                count = 0 ;

                // Mandelbrot iteration
                while (count++ < max_count) {
                    Zim = (multfix28(Zre, Zim)<<1) + Cim ;
                    Zre = Zre_sq - Zim_sq + Cre ;
                    Zre_sq = multfix28(Zre, Zre) ;
                    Zim_sq = multfix28(Zim, Zim) ;

                    if ((Zre_sq + Zim_sq) >= FOURfix28) break ;
                }
                // Increment total count
                total_count += count ;

                // Draw the pixel
                if (count >= max_count) drawPixel(i, j, BLACK) ;
                else if (count >= (max_count>>1)) drawPixel(i, j, WHITE) ;
                else if (count >= (max_count>>2)) drawPixel(i, j, CYAN) ;
                else if (count >= (max_count>>3)) drawPixel(i, j, BLUE) ;
                else if (count >= (max_count>>4)) drawPixel(i, j, RED) ;
                else if (count >= (max_count>>5)) drawPixel(i, j, YELLOW) ;
                else if (count >= (max_count>>6)) drawPixel(i, j, MAGENTA) ;
                else drawPixel(i, j, RED) ;

            }
        }

        end_time = time_us_32() ;
        total_time_core_0 = (float)(end_time - begin_time)*(1./1000000.) ;

        core1_time = multicore_fifo_pop_blocking() ;
        total_time_core_1 = (float)(core1_time)*(1./1000000.) ;

        printf("\nTotal time core 0: %3.6f seconds \n", total_time_core_0) ;
        printf("\nTotal time core 1: %3.6f seconds \n", total_time_core_1) ;
        // printf("Total iterations: %d", total_count) ;
    }
}
