/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * Non-blocking UDP transmitter for RP2040
 * 
 * Hardware connections (unless you specify otherwise
 * in the first argument to initUDP())
 * - GPIO 3 -----> TX+
 * - GPIO 2 -----> TX-
 * 
 * Resources utilized
 * - All DMA channels
 * - PIO state machines 0, 1, and 2 on PIO block 0
 * - PWM slice 7
 * - UART 0 (GPIO's 0 and 1)
 * 
 * To use:
 * - Modify parameters in udp_tx_parameters.h
 *   for your particular network
 * - When you start the program, it will print the contents of the UDP
 *   packet and then prompt for any user input
 * 
 * To test:
 *  - Here is some Python code that you can use to test.
 *    Just modify to your specific IP address and port number
 *    (these should match those you specify in the
 *    udp_tx_parameters.h file)
 * 
 *       import socket
 *       sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
 *       sock.bind(('169.254.177.93', 1024))  # (host, port) because AF_INET
 *       print("Listening...")
 *       while True:
 *           print(sock.recv(180)) # buffer size
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "udp_tx.h"


// Flag to signal transaction complete from ISR to main
volatile int done = 0 ;

// An interrupt handler, runs after packet is sent
void pio0_interrupt_handler() {
    // Clear the interrupt
    pio_interrupt_clear(pio0, 1) ;
    // Toggle the LED
    gpio_put(25, !gpio_get(25)) ;
    // Signal completion
    done = 1 ;
}


int main() {
    // Overclock to 240MHz (makes clock dividers for PIO nice round numbers)
    // You MUST do this in order for the UDP transmitter to work!
    set_sys_clock_khz(240000, true) ;

    // Initialize stdio
    stdio_init_all();

    // If you wanted to prompt for network data, you could do so with this function
    // GetNetworkData()

    // Initialize the UDP transmit machine
    // First argument is GPIO number for TX- (TX+ is this number plus 1).
    // Second argument is the name of the ISR which will be called on transmit completion.
    initUDP(2, pio0_interrupt_handler) ;

    // Print out the contents of the packet
    printf("Packet contents: \n");
    for (int i=0; i<60; i++) {
        printf("%02x\n", assembled_packet[i]) ;
    }

    // Takes a moment for the link to form, prompt for user to start the program
    printf("\n\nEnter anything to begin\n\n") ;
    char str1[50] ;
    scanf("%s", str1) ;

    // We'll blink the LED in the ISR
    gpio_init(25) ;
    gpio_set_dir(25, GPIO_OUT) ;
    gpio_put(25, 0) ;


    while(1) {

        // Send a packet (non-blocking!)
        SEND_PACKET ;

        // Modify the data for the next packet, while the previous
        // one is still being sent!
        udp_payload[17] = udp_payload[17] + 1 ;

        // Wait for packet transaction to finish, if it hasn't already
        // (done is set in the ISR triggered by end of transmission)
        while(done!=1) {
        }

        // Reset the done flag
        done = 0 ;
    }
}
