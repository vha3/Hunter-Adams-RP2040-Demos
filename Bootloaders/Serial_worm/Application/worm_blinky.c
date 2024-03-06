/**
 * V. Hunter Adams (vha3@cornell.edu)
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

// The LED is connected to GPIO 25
#define LED_PIN 25
#define BOOTLOAD_PIN 14
#define SHARE_APPLICATION_PIN 13


// GPIO ISR. Back to bootloader
void gpio_callback() {
    watchdog_hw->scratch[0]=1 ;
    watchdog_reboot(0, 0, 1) ;
}

// Main (runs on core 0)
int main() {

    // Initialize the LED pin
    gpio_init(LED_PIN);
    // Configure the LED pin as an output
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BOOTLOAD_PIN) ;
    gpio_pull_up(BOOTLOAD_PIN) ;
    gpio_init(SHARE_APPLICATION_PIN) ;
    gpio_pull_up(SHARE_APPLICATION_PIN) ;

    // Configure GPIO interrupt
    gpio_set_irq_enabled_with_callback(BOOTLOAD_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Loop
    while (true) {
        while(gpio_get(SHARE_APPLICATION_PIN)) {
            gpio_put(LED_PIN, 0) ;
            sleep_ms(250) ;
            gpio_put(LED_PIN, 1) ;
            sleep_ms(250) ;
        }
        // Share application via worm
        watchdog_hw->scratch[1] = 1 ;
        watchdog_reboot(0, 0, 1) ;
        sleep_ms(1000) ;

    }
}
