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
#include "vga16_graphics_v3.h"
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
// === protothreads globals ===
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
short bkgnd_color = 0 ;

// ==================================================
// === graphics demo -- RUNNING on core 0
// ==================================================
static PT_THREAD (protothread_graphics(struct pt *pt)) {
    PT_BEGIN(pt);
    // the protothreads interval timer
    PT_INTERVAL_INIT( ) ;

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
    drawTextTiny8(65, 0, "Raspberry Pi Pico", WHITE, BLUE) ;
    drawTextGLCD(65, 10, "Graphics primitives", WHITE, BLUE) ;
    drawTextGLCD(65, 20, "Hunter Adams vha3@cornell.edu", WHITE, BLUE) ;
    drawTextGLCD(65, 30, "Bruce Land brl4@cornell.edu", WHITE, BLUE) ;
    //
    drawTextVGA437(255, 10, "Test vga_graphics_v3", BLACK, YELLOW);
    //

    drawTextGLCD(438, 10, "Protothreads rp2040/2350 1.4", BLACK, GREEN) ;
    drawTextGLCD(438, 20, "CPU clock 150 Mhz", BLACK, GREEN) ;
    drawTextGLCD(438, 30, "vga16_v3 driver", BLACK, GREEN) ;
    drawTextGLCD(438, 40, "Double Buffer on rp2350 ONLY", BLACK, GREEN) ;

     char video_buffer[64];
     // The color pallette
    //setTextColor2(WHITE, BLACK) ;
    //setTextSize(1) ;
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++){
        fillRect(i*70+20, 150+j*70, 60, 49, i+4*j);  
        //setCursor(i*70+20, 150+j*70) ; 
        sprintf(video_buffer, "%2d", i+4*j);
        drawTextGLCD(i*70+20, 150+j*70, video_buffer, WHITE, BLACK);
      }
    }
    setTextColor2(WHITE, BLACK) ;
    setTextSize(1) ;
    // first row of colors
    //setCursor(0*70+20, 200+0*70) ; 
    //writeString("BLACK") ;
    drawTextGLCD(0*70+20, 200+0*70, "BLACK", WHITE, BLACK);
    drawTextGLCD(1*70+20, 200+0*70, "DARK_GREEN", WHITE, BLACK);
    drawTextGLCD(2*70+20, 200+0*70, "MED_GREEN", WHITE, BLACK);
    drawTextGLCD(3*70+20, 200+0*70, "GREEN", WHITE, BLACK);
    drawTextGLCD(0*70+20, 200+1*70, "DARK_BLUE", WHITE, BLACK);
    drawTextGLCD(1*70+20, 200+1*70, "BLUE", WHITE, BLACK);
    drawTextGLCD(2*70+20, 200+1*70, "LIGHT_BLUE", WHITE, BLACK);
    drawTextGLCD(3*70+20, 200+1*70, "CYAN", WHITE, BLACK);

    drawTextGLCD(0*70+20, 200+2*70, "RED", WHITE, BLACK);
    drawTextGLCD(1*70+20, 200+2*70, "DARK_ORANGE", WHITE, BLACK);
    drawTextGLCD(2*70+20, 200+2*70, " ORANGE", WHITE, BLACK);
    drawTextGLCD(3*70+20, 200+2*70, "YELLOW", WHITE, BLACK);
    drawTextGLCD(0*70+20, 200+3*70, "MAGENTA", WHITE, BLACK);
    drawTextGLCD(1*70+20, 200+3*70, "PINK", WHITE, BLACK);
    drawTextGLCD(2*70+20, 200+3*70, "LIGHT_PINK", WHITE, BLACK);
    drawTextGLCD(3*70+20, 200+3*70, "WHITE", WHITE, BLACK);
    
    // GLCD font
    int line_spacing = 8;
    int line_base = 150 ;
    int line_start = 330 ;
    drawTextGLCD(line_start, line_base,"--GLCD 5x7 font-- test truncation at the edge of screen", WHITE,  BLACK );
    drawTextGLCD(line_start, line_base+line_spacing, "1234567890~!@#$%^&*()", WHITE, BLACK);
    drawTextGLCD(line_start, line_base+2*line_spacing, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", WHITE, BLACK);
    drawTextGLCD(line_start, line_base+3*line_spacing, "abcdefghijklmnopqrstuvwxyz", WHITE, BLACK);
    drawTextGLCD(line_start, line_base+4*line_spacing, "`-+=;:,./?|", WHITE, BLACK);
    // ASCII font
    line_spacing = 8;
    line_base = 430 ;
    line_start = 30 ;
    drawTextAscii(line_start, line_base,"--ASCII 5x7 font--", WHITE,  BLACK );
    drawTextAscii(line_start, line_base+line_spacing, "1234567890~!@#$%^&*()", WHITE, BLACK);
    drawTextAscii(line_start, line_base+2*line_spacing, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", WHITE, BLACK);
    drawTextAscii(line_start, line_base+3*line_spacing, "abcdefghijklmnopqrstuvwxyz", WHITE, BLACK);
    drawTextAscii(line_start, line_base+4*line_spacing, "`-+=;:,./?|", WHITE, BLACK);

    // Tiny8
    line_spacing = 9;
    line_base = 200 ;
    drawTextTiny8(330, line_base,"--Tiny8 8x8 font-- test truncation at edge of screen", GREEN,  BLACK );
    drawTextTiny8(330, line_base+line_spacing, "1234567890~!@#$%^&*()", GREEN, BLACK);
    drawTextTiny8(330, line_base+2*line_spacing, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", GREEN, BLACK);
    drawTextTiny8(330, line_base+3*line_spacing, "abcdefghijklmnopqrstuvwxyz", GREEN, BLACK);
    drawTextTiny8(330, line_base+4*line_spacing, "`-+=;:,./?|", GREEN, BLACK);

    // VGA font
    line_spacing = 15;
    line_base = 250 ;
    drawTextVGA437(330, line_base, "--VGA437 8x16 font-- test truncation at edge of screen", CYAN, BLACK);
    drawTextVGA437(330, line_base+line_spacing, "1234567890~!@#$%^&*()", CYAN, BLACK);
    drawTextVGA437(330, line_base+2*line_spacing, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", CYAN, BLACK);
    drawTextVGA437(330, line_base+3*line_spacing, "abcdefghijklmnopqrstuvwxyz", CYAN, BLACK);
    drawTextVGA437(330, line_base+4*line_spacing, "`-+=;:,./?|", CYAN, BLACK);
    
    // Arial 24
    line_spacing = 26;
    line_base = 330 ;
    drawTextArial24(320, line_base, "-Arial24 16x24-test truncation", YELLOW, BLACK);
    drawTextArial24(320, line_base+24, "AaBbGg 123 !@#", YELLOW, BLACK);
    //
    drawTextGrotesk32(320, 385, "-Grotesk32 16x32-test trunc", WHITE, BLACK);
    drawTextGrotesk32(320, 417, "AaBbYy 123 !@#{}", WHITE, BLACK);
    //
    //
    if (get_buffer_type() == 1)  drawTextGLCD(270, 30, "double buffer 60 fps", BLACK, YELLOW) ;
    if (get_buffer_type() == 2)  drawTextGLCD(270, 30, "double buffer 30 fps", BLACK, YELLOW) ;
    if (get_buffer_type() == 3)  drawTextGLCD(270, 30, "no double buffer    ", BLACK, YELLOW) ;
        
    copy_buffer0to1();

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
        // this syncs the thread to the buffer swap
        PT_YIELD_UNTIL(pt, draw_start_signal());
        
        draw_time = PT_GET_TIME_usec();
        // clear is fast
        clearRect(0, 50, 640, 150, bkgnd_color);

        // A row of graphics primitives
        fillCircle(filled_circle_x, 100, 45, filled_circle_color);
        
        //
        drawCircle(circle_x, 100, 45, circle_color);
        drawCircle(circle_x+1, 100, 45, circle_color);
        drawCircle(circle_x, 100, 44, circle_color);  
        //
        fillRect(filled_rect_x, 52, 85, 85, filled_rect_color) ;
        //
        drawRect(rect_x, 52, 85, 85, rect_color) ;
        
        //
        fillRoundRect(filled_round_rect_x, 60, 86, 70, 5, filled_round_rect_color) ;
        
        //
        //drawRoundRect(round_rect_x+1, 53, 85, 83, 10, round_rect_color) ;
        drawRoundRect(round_rect_x, 70, 86, 60, 5, round_rect_color) ;
        
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
          filled_circle_color = (rand() & 0x7) + 8 ;
        }
        if(circle_x > right_side) {
          circle_x = 0;
          circle_color = rand() & 0xf ;
        }
        if(filled_rect_x > right_side) {
          filled_rect_x = 0;
          filled_rect_color = (rand() & 0x7) + 8 ;
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
        uint8_t pix_color = readPixel(320,120) ;
        // now mark the pixel
        crosshair(320, 120, WHITE);
        // and draw a string
        if(pix_color>5)  
          drawTextVGA437(320, 121, " Pixel (320,120) color", BLACK, pix_color);
        else  
          drawTextVGA437(320, 121, " Pixel (320,120) color", WHITE, pix_color);
      
        
        //
        drawVLine(320,121,10, WHITE) ;
        drawVLine(321,121,10, BLACK) ;
        drawVLine(319,121,10, BLACK) ;

        sprintf(video_buffer, "Draw Time %4.1f mSec", (float)(PT_GET_TIME_usec()-draw_time)/1000);
        drawTextVGA437(260, 450, video_buffer, WHITE, BLACK) ;

        // Thread is paced by the Vsync edge
       // PT_YIELD_usec(20000) ;
       // PT_YIELD(pt);
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
          sscanf(pt_serial_in_buffer+1,"%d ", &bkgnd_color) ;
          // wipe the whole frame
          e_time = PT_GET_TIME_usec();
          clearLowFrame(0, bkgnd_color) ;
          copy_buffer_to_other() ;
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
  // bump up voltage for overclocking
  // rp2350 will run at 300 Mhz at 1.3 volt
  // vreg_set_voltage (VREG_VOLTAGE_1_30);
  //
  // set the clock
  set_sys_clock_khz(150000, true); // 

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