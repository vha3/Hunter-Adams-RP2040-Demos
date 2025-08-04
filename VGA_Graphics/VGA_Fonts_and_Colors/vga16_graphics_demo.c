/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * HARDWARE CONNECTIONS
 *  
- GPIO 16 ---> VGA Hsync
- GPIO 17 ---> VGA Vsync
- GPIO 18 ---> VGA Green lo-bit --> 470 ohm resistor --> VGA_Green
- GPIO 19 ---> VGA Green hi_bit --> 330 ohm resistor --> VGA_Green
- GPIO 20 ---> 330 ohm resistor ---> VGA-Blue
- GPIO 21 ---> 330 ohm resistor ---> VGA-Red
- RP2040 GND ---> VGA-GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * Protothreads v1.4
 * Threads:
 * core 0:
 * Graphics demo
 * blink LED25 
 * core 1:
 * Serial i/o 
 */
// ==========================================
// === VGA graphics library
// ==========================================
#include "vga16_graphics_v2.h"
#include <stdio.h>
#include <stdlib.h>
// #include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
//
#include "hardware/vreg.h"
#include "hardware/clocks.h"

// ==========================================
// === protothreads globals
// ==========================================
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "string.h"
// protothreads header
#include "pt_cornell_rp2040_v1_4.h"
//
// restart graphics flag
bool restart_graphics = true ;
// shared variable for erase color
int bkgnd_color = 0 ;

// ==================================================
// === graphics demo -- RUNNING on core 0
// ==================================================
static PT_THREAD (protothread_graphics(struct pt *pt)) {
    PT_BEGIN(pt);
    // the protothreads interval timer
    PT_INTERVAL_INIT() ;

    static uint64_t draw_time ;
    static int time ;

    // position of the display primitivea
    static short filled_circle_x = 0, filled_circle_color = 3;
    static short circle_x = 100, circle_color = 11 ; 
    static short rect_x = 300 , rect_color = 7;
    static short filled_rect_x = 200,  filled_rect_color = 9;
    static short round_rect_x = 400 , round_rect_color = 7;
    static short filled_round_rect_x = 500,  filled_round_rect_color = 12;
    static short right_side = 638 ;
    //
    // Draw some filled rectangles
    fillRect(64, 0, 176, 50, BLUE); // blue box
    fillRect(250, 0, 176, 50, YELLOW); // red box
    fillRect(435, 0, 176, 50, GREEN); // green box

    // Write some text
    setTextColor(WHITE) ;
    setCursor(65, 0) ;
    setTextSize(1) ;
    writeString("Raspberry Pi Pico") ;
    setCursor(65, 10) ;
    writeString("Graphics primitives demo") ;
    setCursor(65, 20) ;
    writeString("Hunter Adams vha3@cornell.edu") ;
    setCursor(65, 30) ;
    writeString("Bruce Land brl4@cornell.edu") ;
    //
    setCursor(255, 10) ;
    setTextColor(BLACK) ;
    writeStringBig("Test vga_graphics_v2") ;
    //
    setCursor(438, 10) ;
    setTextColor(BLACK) ;
    setTextSize(1) ;
    writeString("Protothreads rp2040/2350 1.4") ;
    setCursor(438, 20) ;
    writeString("CPU clock 150 Mhz") ;
    setCursor(438, 30) ;
    writeString("vga16_v2 driver") ;
    setTextColor(WHITE) ;

    // Setup a 1Hz timer
    //static struct repeating_timer timer;
    ////add_repeating_timer_ms(-1000, repeating_timer_callback, NULL, &timer);

     char video_buffer[64];
    setTextColor2(WHITE, BLACK) ;
    setTextSize(1) ;
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++){
        fillRect(i*70+20, 150+j*70, 60, 60, i+4*j);  
        setCursor(i*70+20, 150+j*70) ; 
        sprintf(video_buffer, "%2d", i+4*j);
        writeString(video_buffer) ;
      }
    }
    // first row of colors
    setCursor(0*70+20, 200+0*70) ; 
    writeString("BLACK") ;
    setCursor(1*70+20, 200+0*70) ; 
    writeString("DARK_GREEN") ;
    setCursor(2*70+20, 200+0*70) ; 
    writeString("MED_GREEN") ;
    setCursor(3*70+20, 200+0*70) ; 
    writeString("GREEN") ;
    // second row of colors
    setCursor(0*70+20, 200+1*70) ; 
    writeString("DARK_BLUE") ;
    setCursor(1*70+20, 200+1*70) ; 
    writeString("BLUE") ;
    setCursor(2*70+20, 200+1*70) ; 
    writeString("LIGHT_BLUE") ;
    setCursor(3*70+20, 200+1*70) ; 
    writeString("CYAN") ;
    // thrid row of colors
    setCursor(0*70+20, 200+2*70) ; 
    writeString("RED") ;
    setCursor(1*70+20, 200+2*70) ; 
    writeString("DARK_ORANGE") ;
    setCursor(2*70+20, 200+2*70) ; 
    writeString("ORANGE") ;
    setCursor(3*70+20, 200+2*70) ; 
    writeString("YELLOW") ;
    // fourth row of colors
    setCursor(0*70+20, 200+3*70) ; 
    writeString("MAGENTA") ;
    setCursor(1*70+20, 200+3*70) ; 
    writeString("PINK") ;
    setCursor(2*70+20, 200+3*70) ; 
    writeString("LIGHT_PINK") ;
    setCursor(3*70+20, 200+3*70) ; 
    writeString("WHITE") ;

    // small font
    int line_spacing = 8;
    int line_base = 150 ;
    setCursor(330, line_base) ; 
    setTextColor2(GREEN, BLACK);
    writeString("GLCD 5x7 font: size=1");

    setCursor(330, line_base+line_spacing) ; 
    setTextColor2(RED, BLACK);
    writeString("1234567890~!@#$%^&*()");

    setCursor(330, line_base+line_spacing*2) ; 
    setTextColor2(ORANGE, BLACK);
    writeString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    setTextColor2(LIGHT_BLUE, BLACK);
    setCursor(330, line_base+line_spacing*3) ; 
    writeString("abcdefghijklmnopqrstuvwxyz");

    setCursor(330, line_base+line_spacing*4) ; 
    setTextColor2(PINK, BLACK);
    writeString("`-+=;:,./?|");

    // small bold font
    line_spacing = 8;
    line_base = 200 ;
    setCursor(330, line_base) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBold("GLCD 5x7 BOLD font: size=1");

    setCursor(330, line_base+line_spacing) ; 
    setTextColor2(RED, BLACK);
    writeStringBold("1234567890~!@#$%^&*()");

    setCursor(330, line_base+line_spacing*2) ; 
    setTextColor2(ORANGE, BLACK);
    writeStringBold("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    setTextColor2(LIGHT_BLUE, BLACK);
    setCursor(330, line_base+line_spacing*3) ; 
    writeStringBold("abcdefghijklmnopqrstuvwxyz");

    setCursor(330, line_base+line_spacing*4) ; 
    setTextColor2(PINK, BLACK);
    writeStringBold("`-+=;:,./?|");

    // small bold font size=2
    line_spacing = 16;
    line_base = 250 ;
    setTextSize(2);
    setCursor(300, line_base) ; 
    setTextColor2(GREEN, BLACK);
    writeString("GLCD 5x7 font: size=2");

    setCursor(300, line_base+line_spacing) ; 
    setTextColor2(RED, BLACK);
    writeString("1234567890~!@#$%^&*()");

    setCursor(300, line_base+line_spacing*2) ; 
    setTextColor2(ORANGE, BLACK);
    writeString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    setTextColor2(LIGHT_BLUE, BLACK);
    setCursor(300, line_base+line_spacing*3) ; 
    writeString("abcdefghijklmnopqrstuvwxyz");

    setCursor(300, line_base+line_spacing*4) ; 
    setTextColor2(PINK, BLACK);
    writeString("`-+=;:,./?|");

    // big font
    line_spacing = 15;
    line_base = 350 ;
    setTextSize(1);
    setCursor(330, line_base) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBig("VGA437 8x16 font: \x13 only one size \x13");

    setCursor(330, line_base+line_spacing) ; 
    setTextColor2(RED, BLACK);
    writeStringBig("1234567890~!@#$%^&*()");

    setCursor(330, line_base+line_spacing*2) ; 
    setTextColor2(ORANGE, BLACK);
    writeStringBig("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    setTextColor2(LIGHT_BLUE, BLACK);
    setCursor(330, line_base+line_spacing*3) ; 
    writeStringBig("abcdefghijklmnopqrstuvwxyz");

    setCursor(330, line_base+line_spacing*4) ; 
    setTextColor2(PINK, BLACK);
    writeStringBig("`-+=;:,./?|");
    
    drawHLine(75, 430, 50, WHITE );
    setCursor(5, 430) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("drawHLine");
    //
    drawLine(75, 445, 120, 452, WHITE);
    setCursor(5, 440) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("drawLine");
    //
    drawVLine(75, 450, 25, WHITE );
    setCursor(5, 450) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("drawVLine");
    //
    crosshair(220, 454, GREEN);
    drawPixel(222, 458, GREEN) ;
    setCursor(150, 450) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("drawPixel");
    //
    setCursor(340, 430) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("Serial cmd: (e)rase to color (0-15)");
    setCursor(340, 440) ; 
    setTextColor2(WHITE, BLACK);
    writeStringBold("Serial cmd: (r)estart graphics");
 

    while(true) {

        // restart logic
        if (restart_graphics){
            // reset the flag
            restart_graphics = false ;       
            // restarts the current thread at PT_BEGIN(pt);
            // as if it has not executed before
            PT_RESTART(pt) ;
        }
        // frame number
        time++;
        // clear the row of shapes
        // during the vertical interval
        // this syncs the thread to the frame rate
        while(gpio_get(VSYNC)){};
        // DOES NOT CHECK for an edge, so very fast drawing
        // MAY need a 1 mSec yield at the end
        //PT_YIELD_UNTIL(pt, !gpio_get(VSYNC));
       // draw_time = PT_GET_TIME_usec();
        // clear takes 200 uSec
        clearRect(0, 50, 640, 150, (short)bkgnd_color);

        draw_time = PT_GET_TIME_usec();

        // A row of graphics primitives
        fillCircle(filled_circle_x, 100, 45, filled_circle_color);
        setCursor(filled_circle_x-35, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("fillCircle");

        //
        drawCircle(circle_x, 100, 45, circle_color);
        drawCircle(circle_x+1, 100, 45, circle_color);
        drawCircle(circle_x, 100, 44, circle_color);
        setCursor(circle_x-35, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("drawCircle");
        //
        fillRect(filled_rect_x, 52, 85, 85, filled_rect_color) ;
        setCursor(filled_rect_x+5, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("fillRect");
        //
        drawRect(rect_x, 52, 85, 85, rect_color) ;
        setCursor(rect_x+5, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("drawRect");
        //
        //
        fillRoundRect(filled_round_rect_x, 60, 86, 60, 5, filled_round_rect_color) ;
        setCursor(filled_round_rect_x+1, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("fillRoundRect");
        //
        //drawRoundRect(round_rect_x+1, 53, 85, 83, 10, round_rect_color) ;
        drawRoundRect(round_rect_x, 70, 86, 60, 5, round_rect_color) ;
        setCursor(round_rect_x+1, 100) ; 
        setTextColor2(WHITE, BLACK);
        writeStringBold("drawRoundRect");
       // drawRoundRect(round_rect_x+2, 54, 81, 83, 10, round_rect_color) ;
        // upate position
        filled_circle_x += 2 ;
        circle_x += 3 ;
        filled_rect_x += 1 ;
        rect_x += (time & 0x1)==0 ;
        filled_round_rect_x += (time & 0x1)==0 ;
        round_rect_x += 1 ;

        // range check on position
        if(filled_circle_x > right_side) {
          filled_circle_x = 0;
          filled_circle_color = rand() & 0xf ;
        }
        if(circle_x > right_side) {
          circle_x = 0;
          circle_color = rand() & 0xf ;
        }
        if(filled_rect_x > right_side) {
          filled_rect_x = 0;
          filled_rect_color = rand() & 0xf ;
        }
        if(rect_x > right_side) {
          rect_x = 0;
          rect_color = rand() & 0xf ;
        }
        if(filled_round_rect_x > right_side) {
          filled_round_rect_x = 0;
          filled_round_rect_color = rand() & 0xf ;
        }
        if(round_rect_x > right_side) {
          round_rect_x = 0;
          round_rect_color = rand() & 0xf ;
        }
        
        // read back a pixel and show it
        uint8_t pix_color = readPixel(320,100) ;
        // now mark the pixel
        crosshair(320, 100, WHITE);
        // and draw a string
        setCursor(320, 130) ;
        if(pix_color>5)  setTextColor2(BLACK, pix_color) ;
        else  setTextColor2(WHITE, pix_color) ;
        writeStringBig("Pixel (320,100) color") ;
        //
        drawVLine(320,100,30, WHITE) ;
        drawVLine(321,100,30, BLACK) ;
        drawVLine(319,100,30, BLACK) ;

        sprintf(video_buffer, "Draw Time %4.1f mSec", (float)(PT_GET_TIME_usec()-draw_time)/1000);
        setTextColor2(WHITE, BLACK) ;
        setCursor(260, 450);
        writeStringBig(video_buffer) ;

        // Thread is paced by the Vsync edge
        PT_YIELD_usec(20000) ;
        //PT_YIELD(pt);
   }
   PT_END(pt);
} // graphics thread

// ==================================================
// === toggle25 thread on core 0
// ==================================================
// the on-board LED blinks
static PT_THREAD (protothread_toggle25(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false ;
    
     // set up LED p25 to blink
     gpio_init(25) ;	
     gpio_set_dir(25, GPIO_OUT) ;
     gpio_put(25, true);
     // data structure for interval timer
     PT_INTERVAL_INIT() ;

      while(1) {
        // yield time 0.1 second
        //PT_YIELD_usec(100000) ;
        PT_YIELD_INTERVAL(100000) ;

        // toggle the LED on PICO
        LED_state = LED_state? false : true ;
        gpio_put(25, LED_state);
        //
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // blink thread


// ==================================================
// === user's serial input thread on core 1
// ==================================================
// serial_read an serial_write do not block any thread
// except this one
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
      static uint64_t e_time ;

      while(1) {
        // print prompt
        sprintf(pt_serial_out_buffer, "erase/restart: ");
        // spawn a thread to do the non-blocking write
        serial_write ;

        // spawn a thread to do the non-blocking serial read
        serial_read ;
        // convert input string to number
        //sscanf(pt_serial_in_buffer,"%d %d", &test_in1, &test_in2) ;
        short color;
        if(pt_serial_in_buffer[0]=='e'){
          sscanf(pt_serial_in_buffer+1,"%d\r", &bkgnd_color) ;
          // wipe the whole frame
          e_time = PT_GET_TIME_usec();
          clearLowFrame(0, (short)bkgnd_color) ;
          printf( "Erase Time %lld uSec\n\r", (PT_GET_TIME_usec() - e_time));
        }

        if(pt_serial_in_buffer[0]=='r'){
          restart_graphics = true;
        }

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // serial thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main(){ 
  //
  //  === add threads  ====================
  // for core 1
  pt_add_thread(protothread_serial) ;
  //
  // === initalize the scheduler ==========
  pt_schedule_start ;
  // NEVER exits
  // ======================================
}

// ========================================
// === core 0 main
// ========================================
int main(){
  // set the clock
  set_sys_clock_khz(150000, true);

  // start the serial i/o
  stdio_init_all() ;
  // announce the threader version on system reset
  printf("\n\rProtothreads RP2040/2350 v1.4 \n\r");

  // Initialize the VGA screen
  initVGA() ;
  //printf("video assumes cpu clock=%d MHz", rgb_vid_freq);

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0
  pt_add_thread(protothread_graphics);
  pt_add_thread(protothread_toggle25);
  //
  // === initalize the scheduler ===============
  pt_schedule_start ;
  // NEVER exits
  // ===========================================
} // end main