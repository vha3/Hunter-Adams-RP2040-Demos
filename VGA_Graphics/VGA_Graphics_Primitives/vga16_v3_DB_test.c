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
#include "VGA/vga16_graphics_v3.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
#include "pico/sync.h"

// protothreads header
#include "pt_cornell_rp2040_v1_4.h"
//
// restart graphics flag
bool restart_graphics = true ;
// shared variable for erase color
short bkgnd_color = 0 ;
// semaphore to sync drawing across cores
semaphore_t draw_now_s ;

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
    
    drawTextTiny8(65, 0, "Raspberry Pi Pico2", WHITE, BLUE) ;
    drawTextGLCD(65, 10, "Graphics primitives demo", WHITE, BLUE) ;
    drawTextGLCD(65, 20, "Hunter Adams vha3@cornell.edu", WHITE, BLUE) ;
    drawTextGLCD(65, 30, "Bruce Land brl4@cornell.edu", WHITE, BLUE) ;
    //
    drawTextVGA437(255, 10, " Test double buffer", BLACK, YELLOW);
    //
    drawTextGLCD(438, 10, "Protothreads rp2040/2350 1.4", BLACK, GREEN) ;
    drawTextGLCD(438, 20, "CPU clock 150 Mhz", BLACK, GREEN) ;
    drawTextGLCD(438, 30, "vga16_v3 driver", BLACK, GREEN) ;
    drawTextGLCD(438, 40, "(rp2350 ONLY with 2 buffers)", BLACK, GREEN) ;
    //
    if (get_buffer_type() == 1)  drawTextGLCD(270, 30, "double buffer 60 fps", BLACK, YELLOW) ;
    if (get_buffer_type() == 2)  drawTextGLCD(270, 30, "double buffer 30 fps", BLACK, YELLOW) ;
    if (get_buffer_type() == 3)  drawTextGLCD(270, 30, "no double buffer    ", BLACK, YELLOW) ;
    

     char video_buffer[64];
       
    //comment out if you turn off double buffer
    // 'copy to other' figures out which buffer you just drew in,
    // and copies it to the other buffer
    copy_buffer_to_other() ;

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
        
        // this syncs the thread to the buffer swap
        PT_YIELD_UNTIL(pt, draw_start_signal());
        
        // clear takes 200 uSec
        draw_time = PT_GET_TIME_usec();
        clearLowFrame(50, bkgnd_color);
        // signal the other core for tewsting re-entrant text
        PT_SEM_SDK_SIGNAL(pt, &draw_now_s) ;

        // A row of graphics primitives
        fillCircle(filled_circle_x, 150, 5+filled_circle_x/10, filled_circle_color);

        ////
        drawCircle(circle_x, 200, 45, circle_color);
        drawCircle(circle_x+1, 200, 45, circle_color);
        drawCircle(circle_x, 200, 44, circle_color);
        //
        fillRect(filled_rect_x, 250, 200-filled_rect_x/5, 150, filled_rect_color) ;
        //
        // build a quadralateral
        fillTri(filled_rect_x, 200, filled_rect_x+50, 150, filled_rect_x-30, 175, GREEN);
        fillTri(filled_rect_x, 120, filled_rect_x+50, 151, filled_rect_x-30, 176, GREEN);
        //
        //
        drawRect(rect_x, 52, 85, 150, rect_color) ;
        //
        fillRoundRect(filled_round_rect_x, 200, 250, 250-filled_round_rect_x/3, 
          15, filled_round_rect_color) ;
        //
        fillTri(filled_rect_x, 200, filled_rect_x+50, 251, filled_rect_x-50, 276, BLUE);
        
        //
        //drawRoundRect(round_rect_x+1, 53, 85, 83, 10, round_rect_color) ;
        drawRoundRect(round_rect_x, 300, 86, 60, 5, round_rect_color) ;
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
          filled_circle_color = (rand() & 0x7) + 8 ;
        }
        if(circle_x > right_side) {
          circle_x = 0;
          circle_color = (rand() & 0x7) + 1 ;
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
          filled_round_rect_color = (rand() & 0x7) + 8 ;
        }
        if(round_rect_x > right_side) {
          round_rect_x = 0;
          round_rect_color = rand() & 0xf ;
        }
        short point_list[8][2] = {{100, 300}, {120, 320}, {140, 280}, {160, 300},
                                {180, 300}, {200, 320}, {220, 280}, {240, 300}};
        for (int i=0; i<8; i++){
          point_list[i][1] = 240 + point_list[i][1] * (sin((float)time/100)/2);
        }
        drawMultiLine(8, point_list, GREEN) ;
        //
        sprintf(video_buffer, "Draw Time %4.1f mSec", (float)(PT_GET_TIME_usec()-draw_time)/1000);
        drawTextVGA437(260, 450, video_buffer, WHITE, BLACK) ;
        // test GLCDtext
        //drawTextGLCD(300, 300, "testing re-entrant text", WHITE, BLUE,  1) ;

        // Thread is paced by the Vsync edge
       // PT_YIELD_usec(20000) ;
       PT_YIELD(pt);
   }
   PT_END(pt);
} // graphics thread

// ==================================================
// === test re-entrant text thread on core 1
// ==================================================
// trst re-entrant text routines
static PT_THREAD (protothread_text(struct pt *pt))
{
    PT_BEGIN(pt);

      while(1) {
        // wait for draw thread to signal
        PT_SEM_SDK_WAIT(pt, &draw_now_s) ;
        //wait a random amount of time
        // to see if this messes up the draw thread
        PT_YIELD_usec(rand() & 0x3fff) ;

        drawTextVGA437(10, 450, "Testing re-entrant text", YELLOW, BLACK) ;
        drawTextGLCD(10, 466, "    async on other core", YELLOW, BLACK) ;
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
        short y1, y2, bkgnd_color;
        if(pt_serial_in_buffer[0]=='e'){
          sscanf(pt_serial_in_buffer+1,"%d %d %d ", &y1, &y2, &bkgnd_color) ;
          // wipe the whole frame
          e_time = PT_GET_TIME_usec();
          clearRegion(y1, y2, bkgnd_color) ;
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
  pt_add_thread(protothread_text) ;
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
  sem_init(&draw_now_s, 0, 1) ;


  // Initialize the VGA screen
  initVGA() ;
  //printf("video assumes cpu clock=%d MHz", rgb_vid_freq);

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  // === config threads ========================
  // for core 0
  pt_add_thread(protothread_graphics);
  //
  // === initalize the scheduler ===============
  pt_schedule_start ;
  // NEVER exits
  // ===========================================
} // end main