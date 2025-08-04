/**
 * Hunter Adams (vha3@cornell.edu)
 * modifed for 16 colors by BRL4
 * 
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
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * NOTE
 *  - This is a translation of the display primitives
 *    for the PIC32 written by Bruce Land and students
 *
 */


// Give the I/O pins that we're using some names that make sense - usable in main()
 enum vga_pins {HSYNC=16, VSYNC, LO_GRN, HI_GRN, BLUE_PIN, RED_PIN} ;

// We can only produce 16 (4-bit) colors, so let's give them readable names - usable in main()
enum colors {BLACK, DARK_GREEN, MED_GREEN, GREEN,
            DARK_BLUE, BLUE, LIGHT_BLUE, CYAN,
            RED, DARK_ORANGE, ORANGE, YELLOW, 
            MAGENTA, PINK, LIGHT_PINK, WHITE} ;

// VGA primitives - usable in main
void initVGA(void) ;
void drawPixel(short x, short y, char color) ;
void drawVLine(short x, short y, short h, char color) ;
void drawHLine(int x, int y, int w, char color) ; // faster mod 5/11/2025
void drawLine(short x0, short y0, short x1, short y1, char color) ;
void drawRect(short x, short y, short w, short h, char color);
void drawCircle(short x0, short y0, short r, char color) ;
void drawCircleHelper( short x0, short y0, short r, unsigned char cornername, char color) ;
void fillCircle(short x0, short y0, short r, char color) ;
void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color) ;
void drawRoundRect(short x, short y, short w, short h, short r, char color) ;
void fillRoundRect(short x, short y, short w, short h, short r, char color) ;
void fillRect(short x, short y, short w, short h, char color) ;
void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size) ;
void setCursor(short x, short y);
void setTextColor(char c);
void setTextColor2(char c, char bg);
void setTextSize(unsigned char s);
void setTextWrap(char w);
void tft_write(unsigned char c) ;
void writeString(char* str) ;
// ===  strings added 10/11/2023 brl4
void drawCharBig(short x, short y, unsigned char c, char color, char bg) ;
void writeStringBig(char* str) ;
void setTextColorBig(char, char); //works, but can use usual setTextColor2
// 5x7 font
void writeStringBold(char* str);
// === added by brl4 5/11/2025
// fast clear -- x1 and x2 must be EVEN numbered pixels
void clearRect(short x1, short y1, short x2, short y2, short c) ;
// clears the whole frame below top value
void clearLowFrame(short, short) ;
// === get the color of apixel from the frame buffer
short readPixel(short, short) ;
// draw a little cross
void crosshair(short x, short y, short c) ;