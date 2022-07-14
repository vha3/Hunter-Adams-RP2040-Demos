/**
 * V. Hunter Adams (vha3@cornell.edu)
 */

#include "pico/stdlib.h"

// The LED is connected to GPIO 25
#define LED_PIN 25

// Main (runs on core 0)
int main() {
    // Initialize the LED pin
    gpio_init(LED_PIN);
    // Configure the LED pin as an output
    gpio_set_dir(LED_PIN, GPIO_OUT);
    // Loop
    while (true) {
        // Set high
        gpio_put(LED_PIN, 1);
        // Sleep
        sleep_ms(250);
        // Set low
        gpio_put(LED_PIN, 0);
        // Sleep
        sleep_ms(250);
    }
}
