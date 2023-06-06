/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * Simple GPIO interrupt demo.
 * 
 * Wire GPIO 2 to GPIO 3 (thru a resistor).
 * The code toggles GPIO 3, triggering an ISR
 * at every rising edge. The ISR blinks the LED.
 * 
 * Note that the onboard LED is on GPIO 25.
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// GPIO ISR. Toggles LED
void gpio_callback() {
    gpio_put(25, !gpio_get(25)) ;
}

int main() {
    // Initialize stdio
    stdio_init_all();
    printf("GPIO interrupt\n");

    // Configure GPIO interrupt
    gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    // Set GPIO's 3 to output
    gpio_init(3) ;
    gpio_init(25) ;
    gpio_set_dir(3, GPIO_OUT);
    gpio_set_dir(25, GPIO_OUT);

    // Set GPIO 3 to zero
    gpio_put(3, 0) ;

    while (1) {
        // Raise GPIO 3. This triggers an ISR
        gpio_put(3, 1) ;
        sleep_ms(250) ;
        gpio_put(3, 0) ;
        sleep_ms(250) ;
    }

}
