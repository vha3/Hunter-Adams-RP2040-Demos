/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * CAN Transciever test code
 * 
 */

// Standard C libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// High-level pico libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"
// Interface library to watchdog
#include "hardware/watchdog.h"
// CAN driver
#include "can_driver.h"
// Protothreads
#include "pt_cornell_rp2040_v1.h"


//                                   USER GLOBALS
//
// Let's count sent/received transmissions in the ISR's
volatile int number_sent = 0 ;
volatile int number_received = 0 ;
volatile int number_missed = 0 ;

// Flag for indicating that it is unsafe to transmit
volatile int unsafe_to_tx = 1 ;



//                        USER INTERRUPT SERVICE ROUTINES
//
// ISR entered at the end of packet transmit.
void tx_handler() {
    // Abort/reset DMA channel, clear FIFO, clear PIO irq
    resetTransmitter() ;
    // Toggle the LED
    gpio_put(LED_PIN, !gpio_get(LED_PIN)) ;
    // Increment number of messages sent
    number_sent += 1 ;
    // Signal to thread that it is safe to transmit
    unsafe_to_tx = 0 ;
}
// ISR entered when a packet is available for attempted receipt.
void rx_handler() {
    // Abort/reset DMA channel
    resetReceiver() ;
    // Attempt packet receipt
    if (attemptPacketReceive()) {
        // Increment number of messages received
        number_received += 1 ;
    }
    // Count number of rejected packets
    else {
        number_missed += 1 ;
    }
    // Clear the interrupt to receive the next message
    acceptNewPacket() ;
}



//                                 THREADS (USER CODE)
//
// Thread runs on core 1.
static PT_THREAD (protothread_send(struct pt *pt))
{
    PT_BEGIN(pt);

    // Brief delay before starting up
    sleep_ms(2000) ;

    // How many packets should we send?
    static int number_to_send = 1000000 ;
      while(1) {
        // If packets remain . . .
        if (number_to_send) {
            // Indicate that it is unsafe to transmit
            unsafe_to_tx = 1 ;
            // Send a packet
            sendPacket() ;
            // Decrement the remaining number of packets to send ;
            number_to_send -= 1 ;
            // Randomize the payload WHILE previous packet is being sent
            payload[0] = rand()&0b0111111111111111 ;
            payload[1] = rand()&0b0111111111111111 ;
            payload[2] = rand()&0b0111111111111111 ;
            payload[3] = rand()&0b0111111111111111 ;
            payload[4] = rand()&0b0111111111111111 ;
            // Print some data occasionally
            if (((number_to_send+1) % 1000)==0) {
                printf("Sent: %d\n", number_sent) ;
                printf("Received: %d\n", number_received) ;
                printf("Rejected: %d\n\n", number_missed) ;
            }
            // Wait until it's safe to send again
            while(unsafe_to_tx) {} ;
        }
        // If no packets remain, print some data
        else {
            sleep_ms(500) ;
            printf("Number sent: %d\n", number_sent) ;
            printf("Number received: %d\n", number_received) ;
            printf("Number rejected: %d\n\n", number_missed) ;
        }
      } 
  PT_END(pt);
}
// Thread runs on core 0
static PT_THREAD (protothread_watchdog(struct pt *pt))
{
    PT_BEGIN(pt);

    // Enable the watchdog, requiring the watchdog to be updated every 1sec or the chip will reboot
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    watchdog_enable(1000, 1);

      while(1) {
        sleep_ms(100) ;
        watchdog_update();
      } 
  PT_END(pt);
}



//                             MAIN FOR CORES 0 AND 1
//
// Main for core 1
void core1_main() {
    // CAN transmitter will run on core 1
    setupCANTX(tx_handler) ;
    // Add the send thread
    pt_add_thread(protothread_send) ;
    // Start the threader
    pt_schedule_start ;
}
// Main for core 0
int main() {
    // Overclock to 160MHz (divides evenly to 1 megabaud) (160/5/32=1)
    set_sys_clock_khz(OVERCLOCK_RATE, true) ;

    // Initialize stdio
    stdio_init_all();

    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
    } else {
        printf("Clean boot\n");
    }

    // Print greeting
    printf("System starting up\n\n") ;

    // Initialize LED
    gpio_init(LED_PIN) ;
    gpio_set_dir(LED_PIN, GPIO_OUT) ;
    gpio_put(LED_PIN, 0) ;

    // start core 1 threads
    multicore_reset_core1();
    multicore_launch_core1(&core1_main);

    // Setup the CAN receiver on core 0
    setupCANRX(rx_handler) ;

    // Add thread to scheduler, and start it
    pt_add_thread(protothread_watchdog) ;
    pt_schedule_start ;
}
