/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * Uses resistive touchscreen to draw to VGA.
 * 
 * https://vanhunteradams.com/Pico/VGA/Trackpad.html
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

#include "vga_graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"

// Attached to resistive touchscreen
#define XMINUS 6
#define YPLUS 7
#define XPLUS 8
#define YMINUS 9

// Setup for reading the y coordinate
// Y+ and Y- set to input (high impedance)
// X+ and X- set to output
void setupY(void) {

    // direction
    gpio_set_dir(XMINUS, GPIO_OUT) ;
    gpio_set_dir(XPLUS, GPIO_OUT) ;
    gpio_set_dir(YPLUS, GPIO_IN) ;
    gpio_set_dir(YMINUS, GPIO_IN) ;

    // values
    gpio_put(XMINUS, 0);
    gpio_put(XPLUS, 1);

}

// Setup for reading the x coordinate
// X+ and X- set to input (high impedance)
// Y+ and Y- set to output
void setupX(void) {

    // direction
    gpio_set_dir(XMINUS, GPIO_IN) ;
    gpio_set_dir(XPLUS, GPIO_IN) ;
    gpio_set_dir(YPLUS, GPIO_OUT) ;
    gpio_set_dir(YMINUS, GPIO_OUT) ;

    // values
    gpio_put(YMINUS, 0);
    gpio_put(YPLUS, 1);

}

// Selects between measuring x/y coord
volatile int chooser;

// Raw ADC measurements
volatile uint adc_x_raw ;
volatile uint adc_y_raw ;

// X/Y buffers for filtering
volatile uint xarray[16] ;
volatile uint yarray[16] ;

// Accumulation and output of filter
volatile float xout ;
volatile float yout ;
volatile int xret ;
volatile int yret ;

// Pointer to index in sample buffer
unsigned char xpointer = 0 ;
unsigned char ypointer = 0 ;

// Timer ISR
bool repeating_timer_callback(struct repeating_timer *t) {
    // Read X coord
    if (chooser == 1) {
        setupX() ;
        adc_select_input(0);
        adc_x_raw = adc_read();

        xarray[xpointer & 0x0F] = adc_x_raw ;

        xout = 0 ;
        xret = 0 ;
        for (int i=0; i<16; i++) {
            xout += xarray[i] ;
        }
        xret = ((uint)(xout))>>4 ;

        xpointer += 1 ;

        chooser = 0 ;
    }
    // Read Y coord
    else {
        setupY() ;
        adc_select_input(1);
        adc_y_raw = adc_read();

        yarray[ypointer & 0x0F] = adc_y_raw ;

        yout = 0 ;
        yret = 0 ;
        for (int i=0; i<16; i++) {
            yout += yarray[i] ;
        }
        yret = ((uint)(yout))>>4 ;

        ypointer += 1 ;

        chooser = 1 ;
    }

    // Scale and draw to VGA
    if ((xret<3500) && (xret>1500) && (yret<2600) && (yret>1700)) {
        drawPixel(640-(((xret - 1500) * (640)) / (3500 - 1500)),
            (((yret - 1700) * (480)) / (2600 - 1700)), WHITE) ;
    }
}


int main() {

    // Initialize stdio
    stdio_init_all();

    // Initialize ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Initialize the VGA screen
    initVGA() ;

    // Initialize GPIO
    gpio_init(XMINUS) ;
    gpio_init(YPLUS) ;
    gpio_init(XPLUS) ;
    gpio_init(YMINUS) ;

    // Repeating timer struct
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 25us (40kHz) later regardless of how long the callback took to execute
    add_repeating_timer_us(-500, repeating_timer_callback, NULL, &timer);

    while(1) {
    }

}
