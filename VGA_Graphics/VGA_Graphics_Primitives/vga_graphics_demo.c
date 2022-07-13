/**
 * Hunter Adams (vha3@cornell.edu)
 * 
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
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 */

// VGA graphics library
#include "vga_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

// Some globals for storing timer information
volatile unsigned int time_accum = 0;
unsigned int time_accum_old = 0 ;
char timetext[40];

// Timer interrupt
bool repeating_timer_callback(struct repeating_timer *t) {

    time_accum += 1 ;
    return true;
}


int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize the VGA screen
    initVGA() ;

    // circle radii
    short circle_x = 0 ;

    // color chooser
    char color_index = 0 ;

    // position of the disc primitive
    short disc_x = 0 ;
    // position of the box primitive
    short box_x = 0 ;
    // position of vertical line primitive
    short Vline_x = 350;
    // position of horizontal line primitive
    short Hline_y = 250;

    // Draw some filled rectangles
    fillRect(64, 0, 176, 50, BLUE); // blue box
    fillRect(250, 0, 176, 50, RED); // red box
    fillRect(435, 0, 176, 50, GREEN); // green box

    // Write some text
    setTextColor(WHITE) ;
    setCursor(65, 0) ;
    setTextSize(1) ;
    writeString("Raspberry Pi Pico") ;
    setCursor(65, 10) ;
    writeString("Graphics primitives demo") ;
    setCursor(65, 20) ;
    writeString("Hunter Adams") ;
    setCursor(65, 30) ;
    writeString("vha3@cornell.edu") ;
    setCursor(250, 0) ;
    setTextSize(2) ;
    writeString("Time Elapsed:") ;

    // Setup a 1Hz timer
    struct repeating_timer timer;
    add_repeating_timer_ms(-1000, repeating_timer_callback, NULL, &timer);

    while(true) {

        // Modify the color chooser
        if (color_index ++ == 7) color_index = 0 ;

        // A row of filled circles
        fillCircle(disc_x, 100, 20, color_index);
        disc_x += 35 ;
        if (disc_x > 640) disc_x = 0;
        
        // Concentric empty circles
        drawCircle(320, 200, circle_x, color_index);
        circle_x += 1 ;        
        if (circle_x > 130) circle_x = 0;

        // A series of rectangles
        drawRect(10, 300, box_x, box_x, color_index);
        box_x += 5 ;
        if (box_x > 195) box_x = 10;

        // Random lines
        drawLine(210+(rand()&0x7f), 350+(rand()&0x7f), 210+(rand()&0x7f), 
                 350+(rand()&0x7f), color_index);

        // Vertical lines
        drawVLine(Vline_x, 300, (Vline_x>>2), color_index);
        Vline_x += 2 ;
        if (Vline_x > 620) Vline_x = 350;
        
        // Horizontal lines
        drawHLine(400, Hline_y, 150, color_index);
        Hline_y += 2 ;
        if (Hline_y > 400) Hline_y = 240;

        // Timing text
        if (time_accum != time_accum_old) {
            time_accum_old = time_accum ;
            fillRect(250, 20, 176, 30, RED); // red box
            sprintf(timetext, "%d", time_accum) ;
            setCursor(250, 20) ;
            setTextSize(2) ;
            writeString(timetext) ;
        }

        // A brief nap
        sleep_ms(10) ;

   }

}
