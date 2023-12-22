/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 470 ohm resistor ---> VGA Green 
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - GPIO 21 ---> 330 ohm resistor ---> VGA Red
 *  - RP2040 GND ---> VGA GND
 * 
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels obtained by claim mechanism
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * Protothreads v1.1.3
 * Threads:
 * core 0:
 * Graphics demo
 * blink LED25 
 * core 1:
 * Toggle gpio 4 
 * Serial i/o 
 */
// ==========================================
// === VGA graphics library
// ==========================================
#include "vga16_graphics.h"
#include <stdio.h>
#include <stdlib.h>
// #include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
// // Our assembled programs:
// // Each gets the name <pio_filename.pio.h>
// #include "hsync.pio.h"
// #include "vsync.pio.h"
// #include "rgb.pio.h"

// ==========================================
// === protothreads globals
// ==========================================
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "string.h"
// protothreads header
#include "pt_cornell_rp2040_v1_3.h"

char user_string[40] = "Type up to 40 characters" ;
int new_str = 1;

// ==================================================
// === graphics demo -- RUNNING on core 0
// ==================================================
static PT_THREAD (protothread_graphics(struct pt *pt)) {
    PT_BEGIN(pt);
    // the protothreads interval timer
    PT_INTERVAL_INIT() ;

    // circle radii
    static short circle_x = 0 ;

    // color chooser
    static char color_index = 0 ;

    // position of the disc primitive
    static short disc_x = 0 ;
    // position of the box primitive
    static short box_x = 0 ;
    // position of vertical line primitive
    static short Vline_x = 350;
    // position of horizontal line primitive
    static short Hline_y = 250;

    // Draw some filled rectangles
    fillRect(64, 0, 176, 50, BLUE); // blue box
    //fillRect(250, 0, 176, 50, DARK_ORANGE); // red box
    fillRect(435, 0, 176, 50, LIGHT_BLUE); // green box

    // Write some text
    setTextColor(WHITE) ;
    setCursor(65, 0) ;
    setTextSize(1) ;
    writeString("Raspberry Pi Pico") ;
    setCursor(65, 10) ;
    writeString("Font demo") ;
    setCursor(65, 20) ;
    writeString("Hunter Adams") ;
    setCursor(65, 30) ;
    writeString("Bruce Land") ;
    setCursor(65, 40) ;
    writeString("ece4760 Cornell") ;
    //
    setCursor(445, 10) ;
    setTextColor(BLACK) ;
    setTextSize(1) ;
    writeString("Protothreads rp2040 v1.3") ;
    setCursor(445, 20) ;
    writeString("VGA 4-bit color") ;
    setCursor(445, 30) ;
    writeString("Tested on pico and picoW") ;
    //
    setCursor(270, 20) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("VGA437 Font Test") ;
    setCursor(270, 37) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBold("GLCD BOLD Font Test") ;
    

    // Setup a 1Hz timer
    //static struct repeating_timer timer;
    //add_repeating_timer_ms(-1000, repeating_timer_callback, NULL, &timer);

     static char video_buffer[32];
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
    writeString("Testing 5x7 font: size=1");

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
    writeStringBold("Testing 5x7 BOLD font: size=1");

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
    writeStringBold("5x7 BOLD font: size=2");

    setCursor(300, line_base+line_spacing) ; 
    setTextColor2(RED, BLACK);
    writeStringBold("1234567890~!@#$%^&*()");

    setCursor(300, line_base+line_spacing*2) ; 
    setTextColor2(ORANGE, BLACK);
    writeStringBold("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    setTextColor2(LIGHT_BLUE, BLACK);
    setCursor(300, line_base+line_spacing*3) ; 
    writeStringBold("abcdefghijklmnopqrstuvwxyz");

    setCursor(300, line_base+line_spacing*4) ; 
    setTextColor2(PINK, BLACK);
    writeStringBold("`-+=;:,./?|");

    // big font
    line_spacing = 15;
    line_base = 350 ;
    setTextSize(1);
    setCursor(330, line_base) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBig("Testing 8x16 font: \x13 only one size \x13");

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

    char test_str[32] ={1,32,2,32,3,32,4,32,5,32,6,32,7,32,8,32, 
                        9,32,10,32,11,32,12,32,13,32,14,32,15, 0} ;
    setCursor(330, line_base+line_spacing*5) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBig(test_str);

    char test_str_2[32] ={16,32,17,32,18,32,19,32,20,32,21,32,22,32,23,
                      32,24,32,25,32,26,27,32,28,32,29,32,30,32,31,0} ;
    setCursor(330, line_base+line_spacing*6) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBig(test_str_2);

    char test_str_3[32] ={8,7,8,7,8,7,8,7,8,7,8,7,8,7,8,7, 0} ;
    char test_str_4[32] ={32,9,10,9,10,9,10,9,10,9,10,9,10,9,10,9,10, 0} ;
    setCursor(330, line_base+line_spacing*7) ; 
    setTextColor2(GREEN, BLACK);
    writeStringBig(test_str_3);
    setCursor(330, line_base+line_spacing*8) ;
    writeStringBig(test_str_4);


    ///setCursor(100, 320) ; 
    //setTextColor(GREEN);
    //writeStringBig("testing transparent");

    static char white_text[40] = "White text test ABC abc 123";
    static char black_text[40] = "Black text test ABC abc 123";
    static char vary_text[40] = "Random color text test ABC abc 123";

    while(true) {

        // Modify the color 
        if (color_index ++ == 15) color_index = 0 ;

        fillRect(50, 60, 550, 60, color_index);

        setTextColor2(BLACK, color_index );
        setCursor(55, 65) ; 
        writeStringBig(black_text);
        setCursor(55, 85) ; 
        writeString(black_text);

        setTextColor2(WHITE, color_index );
        setCursor(320, 65) ; 
        writeStringBig(white_text);
        setCursor(320, 85) ; 
        writeString(white_text);
        //setTextColor(WHITE);
        //setCursor(321, 85) ; 
        //writeString(white_text);

        static  char text_color ;
        text_color = rand()&0x0f;
        setTextColor2(text_color, color_index );
        setCursor(75, 100) ; 
        writeStringBig(vary_text);

        sprintf(video_buffer, "rect:%02d  text:%02d", color_index, text_color) ;
        setCursor(400, 100) ; 
        writeStringBig (video_buffer);

        // user string
        if (new_str){
          new_str = false ;
          // erase
          setCursor(40, 450) ;
          setTextColor2(WHITE, BLACK ); 
          writeStringBig ("                                       ");
          //
          setCursor(40, 450) ;
          setTextColor2(text_color, BLACK ); 
          writeStringBig (user_string);
        }
  
        // A brief nap
        PT_YIELD_usec(3000000) ;
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
      static int test_in1, test_in2, sum ;
      //
      while(1) {
        // print prompt
        sprintf(pt_serial_out_buffer, "input string <40 char: ");
        // spawn a thread to do the non-blocking write
        serial_write ;

        // spawn a thread to do the non-blocking serial read
         serial_read ;
        // convert input string to number
        strcpy(user_string, pt_serial_in_buffer) ;
        new_str = true ;

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
  //pt_add_thread(protothread_toggle_gpio4) ;
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
  //set_sys_clock_khz(250000, true); // 171us
  // start the serial i/o
  stdio_init_all() ;
  // announce the threader version on system reset
  printf("\n\rProtothreads RP2040 v1.3 two-core, priority\n\r");

  // Initialize the VGA screen
  initVGA() ;
     
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