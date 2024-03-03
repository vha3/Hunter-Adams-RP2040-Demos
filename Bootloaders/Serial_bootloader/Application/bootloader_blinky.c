/**
 * V. Hunter Adams (vha3@cornell.edu)
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

// The LED is connected to GPIO 25
#define LED_PIN 25
#define BOOTLOAD_PIN 14


// GPIO ISR. Sends system back to bootloader
void gpio_callback() {
    watchdog_hw->scratch[0]=1 ;
    watchdog_reboot(0, 0, 1) ;
}

// Main (runs on core 0)
int main() {

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize the return to loader pin
    gpio_init(BOOTLOAD_PIN) ;
    gpio_pull_up(BOOTLOAD_PIN) ;

    // Configure GPIO interrupt
    gpio_set_irq_enabled_with_callback(BOOTLOAD_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Loop
    while (true) {

        gpio_put(LED_PIN, 0) ;
        sleep_ms(250) ;
        gpio_put(LED_PIN, 1) ;
        sleep_ms(250) ;

    }

}
