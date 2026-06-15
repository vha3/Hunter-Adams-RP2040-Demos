#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
// Our assembled programs:
// Each gets the name <pio_filename.pio.h>
#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"
// Header file
#include "vga16_graphics_v3.h"
// Font files
#include "font_glcd.c"
#include "font_ascii_characters.h"
#include "font_rom_237_brl4.h"
#include "font_Arial_round_16x24.h"
#include "font_Grotesk16x32.h"
#include "font_Tiny8.h"
#include <string.h>

/*
===================================================================================
Some of the source code is derived from Adafruit GFX library:
Software License Agreement (BSD License)

Copyright (c) 2012 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
===================================================================================
Many of these fonts were taken from
http://www.rinkydinkelectronics.com/r_fonts.php
The page Font license:
All fonts on this page are considered Public Domain. This means that 
you are free to use them as you see fit in any project, commercial or not.
Commercial projects will still need a commercial license for any libraries used. 
====================================================================================
- GPIO 16 ---> VGA Hsync
- GPIO 17 ---> VGA Vsync
- GPIO 18 ---> VGA Green lo-bit --> 470 ohm resistor -->  VGA_Green
- GPIO 19 ---> VGA Green hi_bit --> 330 ohm resistor -->  VGA_Green
- GPIO 20 ---> 330 ohm resistor ---> VGA-Blue
- GPIO 21 ---> 330 ohm resistor ---> VGA-Red
- RP2040 GND ---> VGA-GND

New text commands are re-entrant
DrawPixel is faster
*/

// VGA timing constants
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE 319    // (horizontal active)/2 - 1
// #define RGB_ACTIVE 639 // change to this if 1 pixel/byte

// Length of the pixel array, and number of DMA transfers
#define VGA_BUFFER_COUNT 153600 // Total pixels/2 (since we have 2 pixels per byte)

// ===============================
// !!!=========================!!!
// Define exactly ONE of
// DOUBLE_BUFFER_60, DOUBLE_BUFFER_30, DOUBLE_BUFFER_NONE
//Double-buffered!
//Turn off second buffer to save memory on RP2040.
// =============================
//#define DOUBLE_BUFFER_60
//#define DOUBLE_BUFFER_30
#define DOUBLE_BUFFER_NONE
// !!!~=========================!!!
// ===============================

// Pixel color array that is DMAed to the PIO machines and
// a pointer to the ADDRESS of this color array.
// Note that this array is automatically initialized to all 0's (black)
unsigned char vga_buffer_0[VGA_BUFFER_COUNT];
char * pointer_vga_buffer_0 = &vga_buffer_0[0] ;
//
// only define second buffer if necessary
#ifndef DOUBLE_BUFFER_NONE
  unsigned char vga_buffer_1[VGA_BUFFER_COUNT];
  char * pointer_vga_buffer_1 = &vga_buffer_1[0] ;
#endif
//
// the actual pointer taken from a table defined below
// and loaded by a DMA channel
char * current_draw_buffer ;
char * pointer_current_display_buffer ;

// define the buffer order arrays to be cycled thru by DMA
// must be aligned to use DMA hardware looping
char * draw_buffer[4]  __attribute__ ((aligned (16))) ;
char * pointer_display_buffer[4] __attribute__ ((aligned (16)));
int    start_flag_array[4] __attribute__ ((aligned (16)));

// DMA sets this when it is time to draw
int start_flag = 0 ;
// used to signal buffer type to thread
int buffer_type ;

// Bit masks for drawPixel routine
#define TOPMASK 0b00001111
#define BOTTOMMASK 0b11110000

// For drawLine
#define swap(a, b) { short t = a; a = b; b = t; }

// For writing text -- obsolete!
#define tabspace 4 // number of spaces for a tab

// For accessing the font librarys in flash memory
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_short(addr) (*(const unsigned short *)(addr))

// For drawing characters -- obsolete!
unsigned short cursor_y, cursor_x, textsize ;
char textcolor, textbgcolor, wrap;

// Screen width/height
#define _width 640
#define _height 480

void initVGA() {
    // Choose which PIO instance to use (there are two instances, each with 4 state machines)
    PIO pio = pio0;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember these locations!
    //
    // We only have 32 instructions to spend! If the PIO programs contain more than
    // 32 instructions, then an error message will get thrown at these lines of code.
    //
    // The program name comes from the .program part of the pio file
    // and is of the form <program name_program>
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program);

    // Manually select a few state machines from pio instance pio0.
    // void pio_sm_claim (PIO pio, uint sm)
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;
    pio_sm_claim (pio, hsync_sm);
    pio_sm_claim (pio, vsync_sm);
    pio_sm_claim (pio, rgb_sm);

    // Call the initialization functions that are defined within each PIO file.
    // Why not create these programs here? By putting the initialization function in
    // the pio file, then all information about how to use/setup that state machine
    // is consolidated in one place. Here in the C, we then just import and use it.
    hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC);
    vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC);
    rgb_program_init(pio, rgb_sm, rgb_offset, LO_GRN);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // ============================== PIO DMA Channels =================================================
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // initialize
    current_draw_buffer = vga_buffer_0 ;    
    
    // define arrays used by DMA channels to automatically swap buffers
    // after the last pixel is drawn and at the beginning of the verrtical sync
    #ifdef DOUBLE_BUFFER_60
      buffer_type = 1;
      // alternate draw/display
      draw_buffer[0] = vga_buffer_1 ;
      draw_buffer[1] = vga_buffer_0 ; 
      draw_buffer[2] = vga_buffer_1 ;
      draw_buffer[3] = vga_buffer_0 ;
      //
      pointer_display_buffer[0] = pointer_vga_buffer_0 ;
      pointer_display_buffer[1] = pointer_vga_buffer_1 ;  
      pointer_display_buffer[2] = pointer_vga_buffer_0 ; 
      pointer_display_buffer[3] = pointer_vga_buffer_1 ;
      // start draw is valid on every video frame
      start_flag_array[0] = 1 ;
      start_flag_array[1] = 1 ;
      start_flag_array[2] = 1 ;
      start_flag_array[3] = 1 ;
    #endif

    #ifdef DOUBLE_BUFFER_30
      buffer_type = 2;
      draw_buffer[0] = vga_buffer_0 ;
      draw_buffer[1] = vga_buffer_0 ; 
      draw_buffer[2] = vga_buffer_1 ;
      draw_buffer[3] = vga_buffer_1 ;
      //
      pointer_display_buffer[0] = pointer_vga_buffer_1 ;
      pointer_display_buffer[1]= pointer_vga_buffer_1 ;  
      pointer_display_buffer[2] = pointer_vga_buffer_0  ; 
      pointer_display_buffer[3] = pointer_vga_buffer_0 ;
      // start draw is valid on every other video frame 
      start_flag_array[0] = 2 ;
      start_flag_array[1] = 0 ;
      start_flag_array[2] = 2 ;
      start_flag_array[3] = 0 ;
    #endif

    #ifdef DOUBLE_BUFFER_NONE
      buffer_type = 3;
      draw_buffer[0] = vga_buffer_0 ;
      draw_buffer[1] = vga_buffer_0 ; 
      draw_buffer[2] = vga_buffer_0 ;
      draw_buffer[3] = vga_buffer_0 ;
      //
      pointer_display_buffer[0] = pointer_vga_buffer_0;
      pointer_display_buffer[1]= pointer_vga_buffer_0 ;  
      pointer_display_buffer[2] = pointer_vga_buffer_0  ; 
      pointer_display_buffer[3] = pointer_vga_buffer_0 ;
      // No double buffer staert anytime
      start_flag_array[0] = 3 ;
      start_flag_array[1] = 3 ;
      start_flag_array[2] = 3 ;
      start_flag_array[3] = 3 ;
    #endif

    // DMA channels - 
    // data_chan sends color data to output PIO
    // disp_chan sets the current display buffer
    // draw_chan sets the current drawing buffer
    // start_chan signals the draw thread to start drawing
    int rgb_data_chan = dma_claim_unused_channel(true); //data
    int set_disp_chan = dma_claim_unused_channel(true); // reset pointer to current displqy
    int set_draw_chan = dma_claim_unused_channel(true); // 
    int set_start_chan = dma_claim_unused_channel(true); // 

    // change this to true of other DMA channels interfer with video
    #define rgb_high_priority false

    // data_chan (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(rgb_data_chan);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 8-bit txfers
    channel_config_set_read_increment(&c0, true);                        // yes read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2) ;                        // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, set_disp_chan);                        // chain to other channel
    channel_config_set_high_priority (&c0, rgb_high_priority) ;

    dma_channel_configure(
        rgb_data_chan,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
        &vga_buffer_0,            // The initial read address (pixel color array)
        VGA_BUFFER_COUNT,           // Number of transfers; in this case each is 1 byte.
        false                       // Don't start immediately.
    );

    // Channel set_disp_chan (reconfigures the first channel)
    // sets pointer_display_buffer from a table
    dma_channel_config c1 = dma_channel_get_default_config(set_disp_chan);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, true);                        // read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, set_draw_chan);                       // chain to other channel
    channel_config_set_ring (&c1, false, 4); // 16 byte table

    dma_channel_configure(
        set_disp_chan,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[rgb_data_chan].read_addr,  // Write address (channel 0 read address)
        pointer_display_buffer,            // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // send the draw pointer to the current draw pointer
    // sets current_draw_buffer from a table
    c1 = dma_channel_get_default_config(set_draw_chan);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, true);                        // read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, set_start_chan);                       // chain to other channel
    channel_config_set_ring (&c1, false, 4); // 16 byte table

    dma_channel_configure(
        set_draw_chan,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &current_draw_buffer,               // Write address (variabler)
        draw_buffer,                       // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // send the start signal to a thread
    // sets a variable from a table
     c1 = dma_channel_get_default_config(set_start_chan);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, true);                        // read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, rgb_data_chan);                       // chain to other channel
    channel_config_set_ring (&c1, false, 4); // 16 byte table

    dma_channel_configure(
        set_start_chan,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &start_flag,                       // Write address (variabler)
        start_flag_array,                       // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Initialize PIO state machine counters. This passes the information to the state machines
    // that they retrieve in the first 'pull' instructions, before the .wrap_target directive
    // in the assembly. Each uses these values to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);


    // Start the two pio machine IN SYNC
    // Note that the RGB state machine is running at full speed,
    // so synchronization doesn't matter for that one. But, we'll
    // start them all simultaneously anyway.
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continously DMA's to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << rgb_data_chan)) ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ============================== Drawing routines  =================================================
/////////////////////////////////////////////////////////////////////////////////////////////////////

// A function for drawing a pixel with a specified color.
// Note that because information is passed to the PIO state machines through
// a DMA channel, we only need to modify the contents of the array and the
// pixels will be automatically updated on the screen.
void drawPixel(short x, short y, char color) {
    // Range checks (640x480 display)
    if((x > 639) | (x < 0) | (y > 479) | (y < 0) ) return;

    // Which pixel is it?
    // shift by one to get the byte (two pixels/byte)
    //int pixel = (640 * y + x) >> 1;
    char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
    // Is this pixel stored in the first 4 bits
    // of the vga data array index, or the second
    // 4 bits? Check, then mask.
    // draws to the current_draw_buffer
    if (x & 1) {
        *(draw_loc) = (*(draw_loc) & TOPMASK) | (color << 4) ;
    }
    else {
        *(draw_loc) = (*(draw_loc) & BOTTOMMASK) | (color) ;
    }
}

// vertical line
void drawVLine(short x, short y, short h, char color) {
    for (short i=y; i<(y+h); i++) {
        drawPixel(x, i, color) ;
    }
}

// horizontal line
// note that this function draws using drawPiexl AND
// directly hitting the buffer memory for speed
void drawHLine(int x, int y, int w, char color) {
  // range checks
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x - 1;
  if(w<1) return ;
  //
  if(w == 1){
    drawPixel(x,y,color);
    return ;
  }
  //
  short both_color = color | (color<<4) ;
  // loner pixel at x -- align left with next byte boundary
  if((x & 1)) {
    drawPixel(x,y,color);
    x++ ;
    w-- ;
  }
  // draw loner pixel at end and adjust width
  if((w & 1)){
    drawPixel(x+w-1, y, color);
    w-- ;
  }
  // draw rest of line
  int len = (w>>1)  ;
  if (len>0  )  //&& len+x < 640 && y<480
    memset(current_draw_buffer+(320*y+(x>>1)), both_color, len) ;
}

// general line drawing
// Bresenham's algorithm - thx wikipedia and thx Bruce!
void drawLine(short x0, short y0, short x1, short y1, char color) {
/* Draw a straight line from (x0,y0) to (x1,y1) with given color
 * Parameters:
 *      x0: x-coordinate of starting point of line. The x-coordinate of
 *          the top-left of the screen is 0. It increases to the right.
 *      y0: y-coordinate of starting point of line. The y-coordinate of
 *          the top-left of the screen is 0. It increases to the bottom.
 *      x1: x-coordinate of ending point of line. The x-coordinate of
 *          the top-left of the screen is 0. It increases to the right.
 *      y1: y-coordinate of ending point of line. The y-coordinate of
 *          the top-left of the screen is 0. It increases to the bottom.
 *      color: 3-bit color value for line
 */
      short steep = abs(y1 - y0) > abs(x1 - x0);
      if (steep) {
        swap(x0, y0);
        swap(x1, y1);
      }

      if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
      }

      short dx, dy;
      dx = x1 - x0;
      dy = abs(y1 - y0);

      short err = dx / 2;
      short ystep;

      if (y0 < y1) {
        ystep = 1;
      } else {
        ystep = -1;
      }

      for (; x0<=x1; x0++) {
        if (steep) {
          drawPixel(y0, x0, color);
        } else {
          drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
          y0 += ystep;
          err += dx;
        }
      }
}

// Draw a rectangle
void drawRect(short x, short y, short w, short h, char color) {
/* Draw a rectangle outline with top left vertex (x,y), width w
 * and height h at given color
 * Parameters:
 *      x:  x-coordinate of top-left vertex. The x-coordinate of
 *          the top-left of the screen is 0. It increases to the right.
 *      y:  y-coordinate of top-left vertex. The y-coordinate of
 *          the top-left of the screen is 0. It increases to the bottom.
 *      w:  width of the rectangle
 *      h:  height of the rectangle
 *      color:  16-bit color of the rectangle outline
 * Returns: Nothing
 */
  drawHLine(x, y, w, color);
  drawHLine(x, y+h-1, w, color);
  drawVLine(x, y, h, color);
  drawVLine(x+w-1, y, h, color);
}

void drawCircle(short x0, short y0, short r, char color) {
/* Draw a circle outline with center (x0,y0) and radius r, with given color
 * Parameters:
 *      x0: x-coordinate of center of circle. The top-left of the screen
 *          has x-coordinate 0 and increases to the right
 *      y0: y-coordinate of center of circle. The top-left of the screen
 *          has y-coordinate 0 and increases to the bottom
 *      r:  radius of circle
 *      color: 16-bit color value for the circle. Note that the circle
 *          isn't filled. So, this is the color of the outline of the circle
 * Returns: Nothing
 */
  short f = 1 - r;
  short ddF_x = 1;
  short ddF_y = -2 * r;
  short x = 0;
  short y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void drawCircleHelper( short x0, short y0, short r, unsigned char cornername, char color) {
// Helper function for drawing circles and circular objects
  short f     = 1 - r;
  short ddF_x = 1;
  short ddF_y = -2 * r;
  short x     = 0;
  short y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

// ==================================================
// int sqrt from https://github.com/chmike/fpsqrt/blob/master/fpsqrt.c
int32_t sqrt_i32(int32_t v) {
    uint32_t b = 1<<30, q = 0, r = v;
    while (b > r)
        b >>= 2;
    while( b > 0 ) {
        uint32_t t = q + b;
        q >>= 1;           
        if( r >= t ) {     
            r -= t;        
            q += b;        
        }
        b >>= 2;
    }
    return q;
}
// =============================
// fill a circle
// uses simple (but fast) sqrt algorithm
void fillCircle(short x0, short y0, short r, char color) {
  // adding r here just makes a better fit
  int r2 = r * r + r;
  if((y0-r < 0) || (y0+r > 479)) return ;
  for(int i=0; i<=r; i++){
    // adding 2 seems to make the circle more symmetric
    int dx = sqrt_i32(r2 - i*i) ;
    // drawHLine(int x, int y, int w, char color) 
    drawHLine(x0-dx, y0+(i), 2*dx, color) ;
    drawHLine(x0-dx, y0-(i), 2*dx, color) ;
  }  
}
// ==================================================
// depricated
// void fillCircle(short x0, short y0, short r, char color) {
// /* Draw a filled circle with center (x0,y0) and radius r, with given color
//  * Parameters:
//  *      x0: x-coordinate of center of circle. The top-left of the screen
//  *          has x-coordinate 0 and increases to the right
//  *      y0: y-coordinate of center of circle. The top-left of the screen
//  *          has y-coordinate 0 and increases to the bottom
//  *      r:  radius of circle
//  *      color: 16-bit color value for the circle
//  * Returns: Nothing
//  */

//   drawVLine(x0, y0-r, 2*r+1, color);
//   fillCircleHelper(x0, y0, r, 3, 0, color);
// }

void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color) {
// Helper function for drawing filled circles
  short f     = 1 - r;
  short ddF_x = 1;
  short ddF_y = -2 * r;
  short x     = 0;
  short y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      drawVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}
  

// Draw a rounded rectangle
void drawRoundRect(short x, short y, short w, short h, short r, char color) {
/* Draw a rounded rectangle outline with top left vertex (x,y), width w,
 * height h and radius of curvature r at given color
 * Parameters:
 *      x:  x-coordinate of top-left vertex. The x-coordinate of
 *          the top-left of the screen is 0. It increases to the right.
 *      y:  y-coordinate of top-left vertex. The y-coordinate of
 *          the top-left of the screen is 0. It increases to the bottom.
 *      w:  width of the rectangle
 *      h:  height of the rectangle
 *      color:  16-bit color of the rectangle outline
 * Returns: Nothing
 */
  // smarter version
  drawHLine(x+r  , y    , w-2*r, color); // Top
  drawHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawVLine(x    , y+r  , h-2*r, color); // Left
  drawVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// =================================================
void fillRoundRect(short x, short y, short w, short h, short r, char color) {
  // smarter version
  fillRect(x, y+r, w, h-2*r, color);
  fillRect(x+r, y, w-2*r, r, color);
  fillRect(x+r, y+h-r, w-2*r, r, color);

  // draw four corners
  fillCircle(x+w-r, y+r, r-1, color);
  fillCircle(x+r  , y+r, r-1, color);
  fillCircle(x+w-r, y+h-r, r-1, color);
  fillCircle(x+r,   y+h-r, r-1, color);
}

// // =================================================
// // Fill a rounded rectangle
// void fillRoundRect(short x, short y, short w, short h, short r, char color) {
//   // smarter version
//   fillRect(x+r, y, w-2*r, h, color);

//   // draw four corners
//   fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
//   fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
// }


// fill a rectangle
void fillRect(short x, short y, short w, short h, char color) {
/* Draw a filled rectangle with starting top-left vertex (x,y),
 *  width w and height h with given color
 * Parameters:
 *      x:  x-coordinate of top-left vertex; top left of screen is x=0
 *              and x increases to the right
 *      y:  y-coordinate of top-left vertex; top left of screen is y=0
 *              and y increases to the bottom
 *      w:  width of rectangle
 *      h:  height of rectangle
 *      color:  3-bit color value
 * Returns:     Nothing
 */
   if((y + h - 1) >= _height) h = _height - y - 1;

  for(int j=y; j<(y+h); j++) {
    drawHLine(x, j, w, color) ;
  }
}

/////////////////////////////////////////////////////////////////////
// copied with minor mods from
// https://ece4760.github.io/Projects/Fall2023/av522_dy245/code.html
/////////////////////////////////////////////////////////////////////
// Draw a filled triangle
// uses top-right rasterization rule to leave no holes between triangles
// (see https://en.wikipedia.org/wiki/Rasterisation)
void fillTri(float x0, float y0, float x1, float y1, float x2, float y2, char color) {
  //
  // sort verts so y0 <= y1 <= y2 (p0 = top, p1 = middle, p2 = bottom)
  if (y1 < y0) {
    swap(x0, x1);
    swap(y0, y1);
  }
  if (y2 < y0) {
    swap(x0, x2);
    swap(y0, y2);
  }
  if (y2 < y1) {
    swap(x1, x2);
    swap(y1, y2);
  }

  // calculate slopes of each edge, in fix15 (don't divide by 0)
  float dxdy_01 = y1 == y0 ? 0 :  (x1 - x0) / (y1 - y0);
  float dxdy_02 = y2 == y0 ? 0 :  (x2 - x0) / (y2 - y0);
  float dxdy_12 = y2 == y1 ? 0 :  (x2 - x1) / (y2 - y1);
  // same for z
  //s15x16 dzdy_01 = y1 == y0 ? 0 : divs15x16(int_to_s15x16(z1 - z0), y1 - y0);
 // s15x16 dzdy_02 = y2 == y0 ? 0 : divs15x16(int_to_s15x16(z2 - z0), y2 - y0);
 // s15x16 dzdy_12 = y2 == y1 ? 0 : divs15x16(int_to_s15x16(z2 - z1), y2 - y1);

  // figure out whether p1 is on the left or right side of the triangle
  bool flat_top = (y0 == y1);
  bool flat_bottom = (y1 == y2);
  bool p1_is_left = flat_top ? (x0 > x1) : (dxdy_02 > dxdy_01);

  // starting at p0, we draw horizontal scanlines (from x_left to x_right, at height y)
  float x_left = x0;
  float x_right = x0;
  float y = y0;

  // similarly, we have interpolators for z coordinates
  //s15x16 z_left = int_to_s15x16(z0) + zeropt5;
  //s15x16 z_right = int_to_s15x16(z0) + zeropt5;

  // x_left and x_right are moved based on slopes of left/right edges
  float dx_left, dx_right;
  float dz_left, dz_right;
  if (p1_is_left) {
    dx_left = dxdy_01;
    dx_right = dxdy_02;
   // dz_left = dzdy_01;
   // dz_right = dzdy_02;
  } else {
    dx_left = dxdy_02;
    dx_right = dxdy_01;
    //dz_left = dzdy_02;
    //dz_right = dzdy_01;
  }

  // macro function to move the scanline down, and update its endpoints
  #define moveScanline() {\
    y += 1 ;\
    x_left += dx_left;\
    x_right += dx_right;\
  }
    //z_left += dz_left;\
   // z_right += dz_right;\
  }
  
  // draw top half of triangle; skipped for flat top case
  while (y < y1) {
    //drawScanline((short)fix2int15(y), (short)fix2int15(x_left), (short)fix2int15(x_right), z_left, z_right, color);
    drawHLine((int) (x_left), (int) (y), abs((int) (x_right-x_left)), color);

    moveScanline();
  }

  // flat bottom triangles skip the rest
  if (flat_bottom)
    return;

  // reconfigure one end of the scanline so it goes from p1 to p2
  if (p1_is_left) {
    x_left = x1;
    dx_left = dxdy_12;
    //z_left = int2fix15(z1) + zeropt5;
   // dz_left = dzdy_12;
  } else {
    x_right = x1;
    dx_right = dxdy_12;
   // z_right = int2fix15(z1) + zeropt5;
   // dz_right = dzdy_12;
  }

  // draw horizontal line through p1 (middle)
  //drawScanline((short)fix2int15(y), (short)fix2int15(x_left), (short)fix2int15(x_right), z_left, z_right, color);
  drawHLine((int) (x_left), (int) (y), (int) (x_right-x_left), color);
  // draw bottom half of triangle; skipped for flat bottom case
  while (y < y2) {
    moveScanline();
    //drawScanline((short)fix2int15(y), (short)fix2int15(x_left), (short)fix2int15(x_right), z_left, z_right, color);
    drawHLine((int) (x_left), (int) (y), (int) (x_right-x_left), color);
  }
}
  
// =============================================
// end copied code
// =============================================
// application builds an array of
// short point_list[numlines][2]

// multiline draw
void drawMultiLine(int num_lines,  short point_list[][2], char color){
  for(int i=1; i<num_lines; i++){
    drawLine(point_list[i-1][0], point_list[i-1][1], point_list[i][0], point_list[i][1], color);
  }
}

// === text stuff ===========================
// NOTE!!! 
// depricated
// do not USE these, they are NOT re-entrant .
// see below for the new text routines
// the OLDER implementation is here for back compatabioity
// Draw a character
void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size) {
    char i, j;
  if((x >= _width)            || // Clip right
     (y >= _height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for (i=0; i<6; i++ ) {
    unsigned char line;
    if (i == 5)
      line = 0x0;
    else
      line = pgm_read_byte(font+(c*5)+i);
    for ( j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        }
      } else if (bg != color) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, bg);
        else {  // big size
          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}
// depricated
inline void setCursor(short x, short y) {
/* Set cursor for text to be printed
 * Parameters:
 *      x = x-coordinate of top-left of text starting
 *      y = y-coordinate of top-left of text starting
 * Returns: Nothing
 */
  cursor_x = x;
  cursor_y = y;
}
// depricated
inline void setTextSize(unsigned char s) {
/*Set size of text to be displayed
 * Parameters:
 *      s = text size (1 being smallest)
 * Returns: nothing
 */
  textsize = (s > 0) ? s : 1;
}
// depricated
inline void setTextColor(char c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}
// depricated
inline void setTextColor2(char c, char b) {
/* Set color of text to be displayed
 * Parameters:
 *      c = 16-bit color of text
 *      b = 16-bit color of text background
 */
  textcolor   = c;
  textbgcolor = b;
}
// depricated
inline void setTextWrap(char w) {
  wrap = w;
}

// depricated
void tft_write(unsigned char c){
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else if (c == '\t'){
      int new_x = cursor_x + tabspace;
      if (new_x < _width){
          cursor_x = new_x;
      }
  } else {
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
  }
}
// depricated
inline void writeString(char* str){
/* Print text onto screen
 * Call tft_setCursor(), tft_setTextColor(), tft_setTextSize()
 *  as necessary before printing
 */
    while (*str){
        tft_write(*str++);
    }
}

//=================================================
// added 10/16/2023 brl4
// depricated
inline void setTextColorBig(char color, char background) {
/* Set color of text to be displayed
 * Parameters:
 *      color = 16-bit color of text
 *      b = 16-bit color of text background
 *      background ==-1 means trasnparten background
 */
  textcolor   = color;
  textbgcolor = background;
}
//=================================================
// added 10/11/2023 brl4
// Draw a character
// depricated
void drawCharBig(short x, short y, unsigned char c, char color, char bg) {
  char i, j ;
  unsigned char line; 
  for (i=0; i<15; i++ ) {   
    line = pgm_read_byte(bigFont+((int)c*16)+i);
    for ( j = 0; j<8; j++) {
      if (line & 0x80) {
        drawPixel(x+j, y+i, color);
      } else if (bg!=color){
        drawPixel(x+j, y+i, bg);
      }
      line <<= 1;
    }
  }
}
// depricated
inline void writeStringBig(char* str){
/* Print text onto screen
 * Call tft_setCursor(), tft_setTextColorBig()
 *  as necessary before printing
 */
    while (*str){
      char c = *str++;
        drawCharBig(cursor_x, cursor_y, c, textcolor, textbgcolor);
        cursor_x += 8 ;
    }
}
// depricated
inline void writeStringBold(char* str){
/* Print text onto screen
 * Call tft_setCursor(), tft_setTextColorBig()
 *  as necessary before printing
 */
   /* Print text onto screen
 * Call tft_setCursor(), tft_setTextColor(), tft_setTextSize()
 *  as necessary before printing
 */
    char temp_bg ;
    temp_bg = textbgcolor;
    while (*str){
        char c = *str++;
        drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
        drawChar(cursor_x+1, cursor_y, c, textcolor, textcolor, textsize);
        cursor_x += 7 * textsize ;
    }
    textbgcolor = temp_bg ;
}

// ===============================================
//Re-entrant text -- >>USE THESE!<<
//
// //GLCD font Adafruit and Hunter
// returns num chars drawn
int drawTextGLCD(short x, short y, char * str, char color, char bgcolor){
  char col[5];
  int char_count = 0 ;
  // get string start
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | y>470 ) return 0;
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // 
  while (*str){
    if((x+6 > 639)) return char_count ;
    char c = *str++ ;   
    char_count++ ; 
    for (int i=0; i<5; i++ ) { 
      col[i] = pgm_read_byte(font+(c*5)+i) ;
    }
    for (int i=0; i<8; i++ ) {   
      // each two pixels is one byte, so write 3 bytes
      // using the value of 'col' to index into the pixel table
      // then do a lot of bit shffling to transpose the character
        *(draw_loc+i*320) =   pix_value[(((col[0]>>i)&0x01)<<1) | (((col[1]>>i)&0x01))] ;
        *(draw_loc+i*320+1) = pix_value[(((col[2]>>i)&0x01)<<1) | (((col[3]>>i)&0x01))] ;
        *(draw_loc+i*320+2) = pix_value[(((col[4]>>i)&0x01)<<1) ] ;   
    }
    draw_loc += 3 ;
    x += 6 ;
  }     
  return char_count ;  
}

// ASCII from Designed by: David Perez de la Cruz,and Ed Lau
// see: https://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/s2005/dp93/index.html
//
int drawTextAscii(short x, short y, char * str, char color, char bgcolor){
  // get string start
  int char_count = 0 ;
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | y>470) return 0;
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // holds a line of the char bit map
  unsigned char line; 
  while (*str){
    if((x+6 > 639)) return char_count ;
    // s
    char c = (*str++) ;    
    char_count++ ;
    for (int i=0; i<7; i++ ) {   
      line = pgm_read_byte(asciifont+((int)c*7)+i);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320) =   pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+1) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+2) = pix_value[(line>>2) & 0x03] ;
        //*(draw_loc+i*320+3) = pix_value[(line) & 0x03] ;     
    }
    draw_loc += 3 ;
    x += 6 ;
  }
  return char_count ;
}

//
// TinyFont from http://www.rinkydinkelectronics.com/r_fonts.php
//
int drawTextTiny8(short x, short y, char * str, char color, char bgcolor){
  // get string start
  int char_count = 0 ;
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | y>470) return 0;
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // holds a line of the char bit map
  unsigned char line; 
  while (*str){
    if((x+8 > 639)) return char_count ;
    // subtract 32 because first file wentry is <space>
    char c = (*str++) - 32 ;    
    char_count++ ;
    for (int i=0; i<8; i++ ) {   
      line = pgm_read_byte(TinyFont+((int)c*8)+i);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320) =   pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+1) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+2) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+3) = pix_value[(line) & 0x03] ;     
    }
    draw_loc += 4 ;
    x += 8 ;
  }
  return char_count ;
}

//  VGA437 from Code Block 437 IBM font 1982
int drawTextVGA437(short x, short y, char * str, char color, char bgcolor){
  int char_count = 0 ;
  // get string start
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | x>630 | y>463) return 0;
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // holds a line of the char bit map
  unsigned char line; 
  while (*str){
    if((x+8 > 639)) return char_count ;
    char c = *str++ ;    
    char_count++ ;
    for (int i=0; i<16; i++ ) {   
      line = pgm_read_byte(bigFont+((int)c*16)+i);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320) =   pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+1) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+2) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+3) = pix_value[(line) & 0x03] ;     
    }
    draw_loc += 4 ;
    x += 8 ;
  }
  return char_count ;
}
//
// Arial_round_16x24  bypasses the general drawPixel becuase of the
// packed nature of the draw buffer access
// http://www.rinkydinkelectronics.com/r_fonts.php
int drawTextArial24(short x, short y, char * str, char color, char bgcolor){
  int char_count = 0 ;
  // get string start
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | y>455 ) return 0;
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // holds a line of the char bit map
  unsigned short line; 
  while (*str){
    if((x+16 > 639)) return char_count ;
    // font filer startsa at character <space>
    char c = (*str++) - 32 ;   
    char_count++ ; 
    for (int i=0; i<24; i++ ) {   
      line = pgm_read_byte(Arial_round_16x24+((int)c*48)+2*i);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320) =   pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+1) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+2) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+3) = pix_value[(line) & 0x03] ;  
      line = pgm_read_byte(Arial_round_16x24+((int)c*48)+2*i+1);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320+4) = pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+5) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+6) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+7) = pix_value[(line) & 0x03] ;     
    }
    draw_loc += 8 ;
    x += 16 ;
  }
  return char_count ;
}
// Grotesk16x32
// http://www.rinkydinkelectronics.com/r_fonts.php
int drawTextGrotesk32(short x, short y, char * str, char color, char bgcolor){
  int char_count = 0 ;
  // get string start
  char * draw_loc = (current_draw_buffer + ((640 * y + x) >> 1)) ;
  // error check
  if(x<0 | y<0 | y>479-32 ) return 0 ; //(x+16*strlen(str)>639)
  // set up the possible values for any byte
  char pix_value[4] = 
    {(bgcolor<<4 | bgcolor), (color<<4 | bgcolor), (bgcolor<<4 | color), (color<<4 | color)};
  // holds a line of the char bit map
  unsigned short line; 
  while (*str){
    // font filer startsa at character <space>
    if((x+16 > 639)) return char_count ;
    char c = (*str++) - 32 ;   
    char_count++ ; 
    for (int i=0; i<31; i++ ) {   
      line = pgm_read_byte(Grotesk16x32+((int)c*64)+2*i);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320) =   pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+1) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+2) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+3) = pix_value[(line) & 0x03] ;  
      line = pgm_read_byte(Grotesk16x32+((int)c*64)+2*i+1);
      // each two pixels is one byte, so write 4 bytes
      // using the value of 'line' to index into the pixel table//
        *(draw_loc+i*320+4) = pix_value[(line>>6) & 0x03] ;
        *(draw_loc+i*320+5) = pix_value[(line>>4) & 0x03] ; //draw_loc+i*320
        *(draw_loc+i*320+6) = pix_value[(line>>2) & 0x03] ;
        *(draw_loc+i*320+7) = pix_value[(line) & 0x03] ;     
    }
    draw_loc += 8 ;
    x +=16 ;
  }
  return char_count ;
}
// ======================================================
// depricated
// !!!Dont use!!!!!! slow and is superceeded by Tiny8
void drawBoldTextGLCD(short x, short y, char * str, char textcolor, char textbgcolor, char size){
   char temp_bg ;
    temp_bg = textbgcolor;
    while (*str){
        char c = *str++;
        drawChar(x, y, c, textcolor, textbgcolor, size);
        drawChar(x+1, y, c, textcolor, textcolor, size);
        x += 7 * size ;
    }
    textbgcolor = temp_bg ;
}
    
// /////////////////////////////////////////////
// Fast erase functions
// NOTE that there is NO RANGE check on these funcitons
// They will clobber memory if x,y falls outside
// the vga display boundaries (0,0) to (640,480)
void clearRect(short x1, short y1, short x2, short y2, short c) {
  for(int i=y1; i<y2; i++){
    memset(current_draw_buffer+320*i+(x1>>1), c | (c<<4), (x2-x1)>>1) ;
  };
}
//
void clearLowFrame(short top, short c) {
    memset((current_draw_buffer+320*top), c | (c<<4), (VGA_BUFFER_COUNT-320*top) );
}
// region from y1 to y2 with y1 < y2
void clearRegion(short y1, short y2, short c) {
  memset((current_draw_buffer+320*y1), c | (c<<4), (320*(y2-y1)) );
}

// ======================================
// buffer copy utilities
#ifndef DOUBLE_BUFFER_NONE
  void copy_buffer0to1(void){
    memcpy(vga_buffer_1, vga_buffer_0, VGA_BUFFER_COUNT) ;
  }

  void copy_buffer1to0(void){
    memcpy(vga_buffer_0, vga_buffer_1, VGA_BUFFER_COUNT) ;
  }

  void copy_buffer_to_other(void) {
      if((int)current_draw_buffer == (int)vga_buffer_1)
        memcpy(vga_buffer_0, vga_buffer_1, VGA_BUFFER_COUNT) ;
      else
        memcpy(vga_buffer_1, vga_buffer_0, VGA_BUFFER_COUNT) ;
  }
#endif

// ====================================
// driver communication with thread
// draw-sync signal to thread -- clears the flag!
int draw_start_signal(void){
  if(start_flag==1) {
    start_flag = 0 ;
    return 1 ;
  }
  else {
    return 0 ;
  }
}

// returns 1 for 60fps, 2 for 30fps, 3 for no double buffer
int get_buffer_type(void){
  return buffer_type ;
}

//////////////////////////////////////////////////
// read back from VGA
// get the color of a pixel
// but remember there are two buffers!
short readPixel(short x, short y) {
  // Which pixel is it?
  int pixel = ((640 * y) + x)>>1 ;
  short color ;
  // Is this pixel stored in the first 4 bits
  // of the vga data array index, or the second
  // 4 bits? Check, then mask.
  if (x & 1) {
      color = *(current_draw_buffer+pixel) >> 4 ;
  }
  else {
      color = *(current_draw_buffer+pixel)& 0xf  ;
  }
  return color ;
}
  
///////////////////////////////////////////////
void crosshair(short x, short y, short c){
  drawPixel(x,y,c);
  drawPixel(x-1,y,c);
  drawPixel(x+1,y,c);
  drawPixel(x,y-1,c);
  drawPixel(x,y+1,c);
  drawPixel(x-2,y,c);
  drawPixel(x+2,y,c);
  drawPixel(x,y-2,c);
  drawPixel(x,y+2,c);
}

///////////////////////////////////////////////