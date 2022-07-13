/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * Conway's Game of Life
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
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"



int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize VGA
    initVGA() ;

    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////// Game of Life ////////////////
    /////////////////////////////////////////////////////////////////////

    // Initialize the screen (specific eternal growth initial conditions)
    drawCell(160, -50+70, WHITE) ;
    drawCell(160, -50+71, WHITE) ;
    drawCell(160, -50+72, WHITE) ;
    drawCell(160, -50+73, WHITE) ;
    drawCell(160, -50+74, WHITE) ;
    drawCell(160, -50+75, WHITE) ;
    drawCell(160, -50+76, WHITE) ;
    drawCell(160, -50+77, WHITE) ;

    drawCell(160, -50+79, WHITE) ;
    drawCell(160, -50+80, WHITE) ;
    drawCell(160, -50+81, WHITE) ;
    drawCell(160, -50+82, WHITE) ;
    drawCell(160, -50+83, WHITE) ;

    drawCell(160, -50+87, WHITE) ;
    drawCell(160, -50+88, WHITE) ;
    drawCell(160, -50+89, WHITE) ;

    drawCell(160, -50+96, WHITE) ;
    drawCell(160, -50+97, WHITE) ;
    drawCell(160, -50+98, WHITE) ;
    drawCell(160, -50+99, WHITE) ;
    drawCell(160, -50+100, WHITE) ;
    drawCell(160, -50+101, WHITE) ;
    drawCell(160, -50+102, WHITE) ;

    drawCell(160, -50+104, WHITE) ;
    drawCell(160, -50+105, WHITE) ;
    drawCell(160, -50+106, WHITE) ;
    drawCell(160, -50+107, WHITE) ;
    drawCell(160, -50+108, WHITE) ;
    
    int i = 0 ;
    int j = 0 ;
    char saved_row[320] = {BLACK} ;
    char updated_row[320] = {BLACK} ;
    int saved_row_num = 238 ;

    int living ;
    int neighbors ;

    uint32_t start_time ;
    uint32_t end_time ;

    while(1) {

        start_time = time_us_32() ;

        for (j=1; j<239; j++) {
            for (i=1; i<319; i++) {
                // Check if cell is alive, and get number of neighbors
                living = isAlive(i, j) ;
                neighbors = checkNeighbors(i, j) ;

                // Apply rules, save living/dead status in saved_row array
                if (living && ((neighbors==2) || (neighbors==3))) {
                    updated_row[i] = WHITE ;
                }

                else if (!living && (neighbors==3)) {
                    updated_row[i] = WHITE ;
                }

                else {
                    updated_row[i] = BLACK ;
                }
            }
            // Draw the saved row
            for (i=0; i<319; i++) {
                drawCell(i, saved_row_num, saved_row[i]) ;
            }
            // Move the updated row to the saved row
            memcpy(saved_row, updated_row, 320) ;
            // Increment the saved row number, wrapping at 318
            saved_row_num += 1 ;
            if (saved_row_num >= 239) {
                saved_row_num = 1 ;
            }
        }

        end_time = time_us_32() ;
        printf("Time to animate: %f\n", (float)(end_time-start_time)*(1./1000000.)) ;
    }
}
