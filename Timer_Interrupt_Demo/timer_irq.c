/**
 * V. Hunter Adams
 * Timer interrupt example
 */

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"

#define LED_PIN 25

// Timer ISR
bool repeating_timer_callback(struct repeating_timer *t) {
	gpio_put(LED_PIN, !gpio_get(LED_PIN)) ;
}

int main() {
    // Initialize stdio (required for printf)
    stdio_init_all();

    // Print a message
    printf("Timer IRQ!\n");

    // Initialize the LED pin
    gpio_init(LED_PIN);
    // Configure the LED pin as an output
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Create a repeating timer that calls repeating_timer_callback.
    // If the delay is > 0 then this is the delay between the previous callback ending and the next starting.
    // If the delay is negative (see below) then the next call to the callback will be exactly x us after the
    // start of the call to the last callback
    struct repeating_timer timer;

    // Negative delay so means we will call repeating_timer_callback, and call it again
    // 250,000us (4 Hz) later regardless of how long the callback took to execute
    add_repeating_timer_us(-250000, repeating_timer_callback, NULL, &timer);
    while(1){
    }
    return 0;
}