/**
 * V. Hunter Adams (vha3@cornell.edu)
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "hardware/pwm.h"

// The LED is connected to GPIO 25
#define LED_PIN 25
#define BOOTLOAD_PIN 14

// UART configurations
#define UART_ID     uart0
#define BAUD_RATE   4800
#define DATA_BITS   8
#define STOP_BITS   1
#define PARITY      UART_PARITY_NONE

// We are using pins 0 and 1
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// PWM to pin 15
#define PWM_PIN 15

// Length of our line buffer (bytes)
#define BUFFER_LEN 44

// Sector and page sizes for flash memory
#define SECTOR_SIZE 4096
#define PAGE_SIZE   256

// Application program offset in flash (32*1024)
// This should agree with the linker script for the
// application program.
#define PROGRAM_OFFSET 12288

// Delay between lines
#define UART_DELAY 81
#define UART_SMALL_DELAY 2

// For keeping track of where we are in programming/erasing flash
uint32_t programming_address ;
uint32_t erasure_address ;

// Address of binary information header
uint8_t *flash_target_contents = (uint8_t *) (XIP_BASE + PROGRAM_OFFSET + 0xD4);

// Line buffer
char line_buffer[BUFFER_LEN] = {0x00} ;
char dest_buffer[BUFFER_LEN] = {0x00} ;

// We'll claim a DMA channel
int dma_chan_1 ; // Performs checksum
int dma_chan_2 ; // Clears buffer
int dma_chan_3 ; // Clears flashbuffer
int dma_chan_4 ; // Copies flashbuffer to first_page_buffer

// For counting characters received
int char_count ;

// For zeroing-out receive buffer
char zeroval = 0x00 ;

// Recieved hexline buffers
unsigned char num_bytes ;
unsigned short address ;
unsigned char hextype ;
char data_buffer[16] = {0} ;
unsigned char checksum ;

// Flash memory buffer and index
unsigned char flashbuffer[PAGE_SIZE] = {0} ;
unsigned char flashdex = 0 ;

// We're going to program the first page LAST, so we don't
// accidentally vector into a partially-programmed application
unsigned char first_page = 0 ;
unsigned char first_page_buffer[PAGE_SIZE] = {0} ;


// Retrieve and buffer a line (ASCII)
// THIS IS THE ONLY PLACE WHERE UART STUFF HAPPENS.
// If you want to acquire a new program via some other input
// (light? audio? TCP/UDP? IR?), modify this function.
void getLine(char* buffer) {

    // Clear the buffer (DMA for speed)
    dma_channel_set_write_addr(dma_chan_2, buffer, true) ;
    dma_channel_wait_for_finish_blocking(dma_chan_2) ;

    // Count characters as we receive them
    char_count = 0 ;

    // Here's where we'll buffer a single character
    // (the first one is a colon, throw it away)
    char ch ;
    do {
        ch = uart_getc(UART_ID) ;
    } while (ch!=':') ;

    // For as long as we don't see a newline or carriage return . . .
    // NOTE TO FUTURE SELF: program could get stuck here.
    do {
        // Go get a character
        ch = uart_getc(UART_ID) ;
        // Buffer that character
        *buffer++ = ch ;
        // Count the character
        char_count += 1 ;
    } while ((ch!='\n') && (ch!='\r')) ;

    // Null-terminate the string
    *buffer++ = 0 ;
}

// Tranforms line from ASCII to binary reprentation
void transformLine(char* buffer) {

    // Iterator, top nibble, bottom nibble
    int i ;
    char top ;
    char bottom ;

    // Each character is a nibble, so loop to half the
    // number of characters that we counted
    for (i=0; i<(char_count>>1); i++) {

        // Get the top nibble (careful of ASCII offsets)
        if (buffer[i<<1] < 'A') top = buffer[i<<1] - '0' ;
        else top = buffer[i<<1] - 'A' + 10 ;

        // Get the bottom nibble (careful of ASCII offsets)
        if (buffer[(i<<1)+1] < 'A') bottom = buffer[(i<<1)+1] - '0' ;
        else bottom = buffer[(i<<1)+1] - 'A' + 10 ;

        // Mask the two nibbles into a byte
        buffer[i] = (top<<4) | bottom ;
    }

    // Zero-out the rest of the buffer
    while(i<BUFFER_LEN) {
        buffer[i] = 0x00 ;
        i += 1 ;
    }

}

// Performs a checksum on the received hex line
int checkLine(char* source_buffer, char* destination_buffer) {

    // Reset the sniff register to zero
    dma_sniffer_set_data_accumulator(0x00000000) ;

    // Reset DMA read and write addresses, trigger and wait for finish
    dma_channel_set_read_addr(dma_chan_1, source_buffer, false) ;
    dma_channel_set_write_addr(dma_chan_1, destination_buffer, true) ;
    dma_channel_wait_for_finish_blocking(dma_chan_1) ;

    // Return 2's complement of LSB of checksum. If zero, check passed
    return ((~(dma_hw->sniff_data) + 0x1) & 0x000000FF) ;
}

// Parse the verified line into length, address, type, data, and checksum
void parseLine(char* buffer) {

    // Iterator
    int i ;

    // Grab length, address, hextype
    num_bytes = buffer[0] ;
    address = (buffer[1]<<8) | buffer[2] ;
    hextype = buffer[3] ;

    // Grab the data
    for (i=0; i<num_bytes; i++) {
        data_buffer[i] = buffer[i+4] ;
    }

    // Grab the checksum
    checksum = buffer[num_bytes+4] ;
}

// Acquire a single hexline. Return 1 if valid, 0 if checksum failed
int acquireLine(char* source_buffer, char* destination_buffer) {

    // Did the checksum pass?
    int valid ;

    // "Please send me a line"
    uart_putc(UART_ID, 'A') ;

    // Get a raw line of ASCII characters
    getLine(source_buffer) ;

    // Transform those ASCII characters to binary
    transformLine(source_buffer) ;

    // Perform a checksum
    valid = checkLine(source_buffer, destination_buffer) ;

    // If passed, parse the line. Else return 0.
    if (!valid) {
        // "Advance your line counter"
        uart_putc(UART_ID, 'B') ;
        parseLine(destination_buffer) ;
        return 1 ;
    }
    else {
        return 0 ;
    }
}

// Initializes erasure address and programming address.
// Clears the flashbuffer, zeroes the flashdex.
// Erases the first sector of flash, and increments erasure
// address by the sector size (4096 bytes).
static inline void handleFileStart() {

    // Reset the erasure pointer
    erasure_address = PROGRAM_OFFSET ;

    // Reset the programming pointer
    programming_address = PROGRAM_OFFSET ;

    // Clear the flashbuffer
    dma_channel_set_write_addr(dma_chan_3, flashbuffer, true) ;
    dma_channel_wait_for_finish_blocking(dma_chan_3) ;

    // Reset the flashdex to 0
    flashdex = 0 ;

    // Erase the first sector (4096 bytes)
    flash_range_erase(erasure_address, SECTOR_SIZE) ;

    // Increment the erasure pointer by 4096 bytes
    erasure_address += SECTOR_SIZE ;

}

// After we've received a data hexline, buffer that data into the flashbuffer.
// If the flashbuffer is full, then write the buffer to flash memory at the
// programming address. If there's no more space in the erased sector of
// flash memory, then erase the next sector before programming.
static inline void handleData() {

    // Loop thru the data in the line we just received . . .
    for (int i = 0; i<num_bytes; i++) {
        // Store a byte from the data buffer into the flashbuffer.
        // Increment the flashdex. This will overflow from 255->0.
        flashbuffer[flashdex++] = data_buffer[i] ;

        // Did we just overflow to zero? If so, it's time to program
        // the buffer into flash memory.
        if (!flashdex) {
            // Is there space remaining in the erased sector? 
            // If so . . .
            if (programming_address < erasure_address) {

                // If this is the first page, copy it to the first_page_buffer
                // and reset first_page
                if (first_page) {
                    // Copy contents of flashbuffer to first_page_buffer
                    dma_channel_set_read_addr(dma_chan_4, flashbuffer, false) ;
                    dma_channel_set_write_addr(dma_chan_4, first_page_buffer, true) ;
                    dma_channel_wait_for_finish_blocking(dma_chan_4) ;
                    // Reset first_page
                    first_page = 0 ;
                }
                else {
                    // Program flash memory at the programming address
                    flash_range_program(programming_address, flashbuffer, PAGE_SIZE) ;
                }

                // Increment the programming address by page size (256 bytes)
                programming_address += PAGE_SIZE ;

                // Clear the flashbuffer. Flashdex autowraps to 0.
                dma_channel_set_write_addr(dma_chan_3, flashbuffer, true) ;
                dma_channel_wait_for_finish_blocking(dma_chan_3) ;
            }

            // If not . . .
            else {
                // Erase the next sector (4096 bytes)
                flash_range_erase(erasure_address, SECTOR_SIZE) ;

                // Increment the erasure pointer by 4096 bytes
                erasure_address += SECTOR_SIZE ;

                // Program flash memory at the programming address
                flash_range_program(programming_address, flashbuffer, PAGE_SIZE) ;

                // Increment the programming address by page size (256 bytes)
                programming_address += PAGE_SIZE ;

                // Clear the flashbuffer. Flashdex autowraps to 0.
                dma_channel_set_write_addr(dma_chan_3, flashbuffer, true) ;
                dma_channel_wait_for_finish_blocking(dma_chan_3) ;
            }
        }
    }
    // Toggle the LED
    gpio_put(LED_PIN, !gpio_get(LED_PIN)) ;
}

// We may have a partially-full buffer when we get the end of file hexline.
// If so, write that last buffer to flash memory.
static inline void writeLastBuffer() {

    // Program the final flashbuffer.
    // Is there space remaining in the erased sector?
    // If so . . .
    if (programming_address < erasure_address) {
        // Program flash memory at the programming address
        flash_range_program(programming_address, flashbuffer, PAGE_SIZE) ;
    }

    // If not . . .
    else {
        // Erase the next sector (4096 bytes)
        flash_range_erase(erasure_address, SECTOR_SIZE) ;
        // Program flash memory at the programming address
        flash_range_program(programming_address, flashbuffer, PAGE_SIZE) ;
    }

}

// We program the first page LAST, since we use it to figure out if there's
// a valid program to vector into.
static inline void writeFirstPage() {
    flash_range_program(PROGRAM_OFFSET, first_page_buffer, PAGE_SIZE) ;
}

// Before branching to our application, we want to disable interrupts
// and release any resources that we claimed for the bootloader.
static inline void cleanUp() {

    // Disable systick
    // hw_set_bits((io_rw_32 *)0xe000e010, 0xFFFFFFFF);

    // Turn off interrupts (NVIC ICER, NVIC ICPR)
    hw_set_bits((io_rw_32 *)0xe000e180, 0xFFFFFFFF);
    hw_set_bits((io_rw_32 *)0xe000e280, 0xFFFFFFFF);

    // SysTick->CTRL &= ~1;

    // Free-up DMA
    dma_channel_cleanup(dma_chan_1) ;
    dma_channel_unclaim(dma_chan_1) ;
    dma_channel_cleanup(dma_chan_2) ;
    dma_channel_unclaim(dma_chan_2) ;
    dma_channel_cleanup(dma_chan_3) ;
    dma_channel_unclaim(dma_chan_3) ;

    // Disable the sniffer
    dma_sniffer_disable() ;

    uart_puts(UART_ID, "Branching\n") ;

    // Release UART and GPIO
    uart_deinit(UART_ID) ;
    gpio_deinit(LED_PIN) ;
    gpio_deinit(BOOTLOAD_PIN) ;

}

// Set VTOR register, set stack pointer, and jump to the reset
// vector in our application. Basically copied from crt0.S.
static inline void handleBranch() {

    // In an assembly snippet . . .
    // Set VTOR register, set stack pointer, and jump to reset
    asm volatile (
    "mov r0, %[start]\n"
    "ldr r1, =%[vtable]\n"
    "str r0, [r1]\n"
    "ldmia r0, {r0, r1}\n"
    "msr msp, r0\n"
    "bx r1\n"
    :
    : [start] "r" (XIP_BASE + PROGRAM_OFFSET), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
    :
    );
}

// Convert a nibble to an ASCII value
unsigned char numToHex(unsigned char binary_value) {
    switch(binary_value)
    {
        case 0:
            return 48 ;
            break ;
        case 1:
            return 49 ;
            break ;
        case 2:
            return 50 ;
            break ;
        case 3:
            return 51 ;
            break ;
        case 4:
            return 52 ;
            break ;
        case 5:
            return 53 ;
            break ;
        case 6:
            return 54 ;
            break ;
        case 7:
            return 55 ;
            break ;
        case 8:
            return 56 ;
            break ;
        case 9:
            return 57 ;
            break ;
        case 10:
            return 65 ;
            break ;
        case 11:
            return 66 ;
            break ;
        case 12:
            return 67 ;
            break ;
        case 13:
            return 68 ;
            break ;
        case 14:
            return 69 ;
            break ;
        case 15:
            return 70 ;
            break ;
        default:
            return 0 ;
            break ;
    }
}


// Main (runs on core 0)
int main() {

    gpio_init(BOOTLOAD_PIN) ;
    gpio_pull_up(BOOTLOAD_PIN) ;

    // Did the application send us back to the bootloader?
    // If so, reset WD scratch and go to the bootloader
    if (watchdog_hw->scratch[0]) {
        watchdog_hw->scratch[0] = 0 ;
    }

    // If not, use the BINARY_INFO_MARKER_START to determine 
    // whether there's a valid application to which we can branch.
    else if (   (flash_target_contents[0]==0xF2) &&
                (flash_target_contents[1]==0xEB) &&
                (flash_target_contents[2]==0x88) &&
                (flash_target_contents[3]==0x71)) {

                // If not, are we supposed to enter the bootloader anyway?
                if (gpio_get(BOOTLOAD_PIN)) {
                    gpio_deinit(BOOTLOAD_PIN) ;
                    handleBranch() ;
                }

    }
    // There isn't a valid application, go to the bootloader!

    // Initialize PWM
    // Tell GPIO 0 and 1 they are allocated to the PWM
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    // Set period of 4 cycles (0 to 3 inclusive)
    pwm_set_wrap(slice_num, 2232);
    // Set initial B output high for three cycles before dropping
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1116);
    // Set the PWM running
    pwm_set_enabled(slice_num, true);

    // Initialize the LED pin
    gpio_init(LED_PIN);

    // Configure the LED pin as an output
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize the UART channel
    uart_init(UART_ID, BAUD_RATE) ;

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_pull_up(UART_RX_PIN) ;

    // Claim a few DMA channels
    dma_chan_1 = dma_claim_unused_channel(true) ;
    dma_chan_2 = dma_claim_unused_channel(true) ;
    dma_chan_3 = dma_claim_unused_channel(true) ;
    dma_chan_4 = dma_claim_unused_channel(true) ;

    // Configure the first channel (performs checksum)
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_1);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_8);
    channel_config_set_read_increment(&c1, true); 
    channel_config_set_write_increment(&c1, true); 

    dma_channel_configure(
        dma_chan_1,         // Channel to be configured
        &c1,                // The configuration we just created
        dest_buffer,        // write address
        line_buffer,        // The initial read address
        BUFFER_LEN,         // Number of transfers; in this case each is 1 byte.
        false               // Don't start immediately.
    );

    // Configure the second channel (clears ascii buffer)
    dma_channel_config c2 = dma_channel_get_default_config(dma_chan_2); 
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_8);
    channel_config_set_read_increment(&c2, false); 
    channel_config_set_write_increment(&c2, true); 

    dma_channel_configure(
        dma_chan_2,         // Channel to be configured
        &c2,                // The configuration we just created
        line_buffer,        // write address
        &zeroval,           // read address
        BUFFER_LEN,         // Number of transfers; in this case each is 1 byte.
        false               // Don't start immediately.
    );

    // Configure the sniffer!
    dma_sniffer_enable(dma_chan_1, 0x0F, true);
    hw_set_bits(&dma_hw->sniff_data, 0x0);

    // Configure the third channel (clears flashbuffer)
    dma_channel_config c3 = dma_channel_get_default_config(dma_chan_3); 
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_8);
    channel_config_set_read_increment(&c3, false); 
    channel_config_set_write_increment(&c3, true); 

    dma_channel_configure(
        dma_chan_3,         // Channel to be configured
        &c3,                // The configuration we just created
        flashbuffer,        // write address
        &zeroval,           // read address
        PAGE_SIZE,          // Number of transfers; in this case each is 1 byte.
        false               // Don't start immediately.
    );

    // Configure the fourth channel (copies flashbuffer to first_page_buffer)
    dma_channel_config c4 = dma_channel_get_default_config(dma_chan_4);
    channel_config_set_transfer_data_size(&c4, DMA_SIZE_8); 
    channel_config_set_read_increment(&c4, true); 
    channel_config_set_write_increment(&c4, true); 

    dma_channel_configure(
        dma_chan_4,         // Channel to be configured
        &c4,                // The configuration we just created
        first_page_buffer,  // write address
        flashbuffer,        // read address
        PAGE_SIZE,          // Number of transfers; in this case each is 1 byte.
        false               // Don't start immediately.
    );

    // Waiting for start of program
    gpio_put(LED_PIN, 1) ;

    // Loop
    while (true) {


        // Go acquire and parse a new hexline, making sure it passes checksum
        if (acquireLine(line_buffer, dest_buffer)) {

            // Which type of hexline did we receive?
            switch(hextype)
            {
                // DATA
                case 0x00:
                    handleData() ;
                    break ;

                // END OF FILE
                case 0x01:

                    // Write last buffer
                    writeLastBuffer() ;
                    writeFirstPage() ;
                    cleanUp() ;

                    // Stop the PWM running
                    pwm_set_enabled(slice_num, false);
                    gpio_deinit(PWM_PIN) ;

                    // Branch to program
                    handleBranch() ;
                    break ;

                // EXTENDED LINEAR ADDRESS (START OF FILE)
                case 0x04:
                    first_page = 1 ;
                    handleFileStart() ;
                    break ;

                // ANYTHING ELSE
                default:
                    break ;
            }
        }
        else {
            // Our checksum has failed! 
            gpio_put(LED_PIN, 1) ;
        }
    }
}
