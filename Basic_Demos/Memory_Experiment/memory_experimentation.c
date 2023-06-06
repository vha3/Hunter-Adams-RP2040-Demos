/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * Experimenting with executing code from RAM
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

int variable ;

// GPIO ISR. Toggles LED
void function_one() {
    gpio_put(25,0) ;
}

void __not_in_flash_func(function_two)() {
    gpio_put(25,1) ;
}

int main() {
    // Initialize stdio
    stdio_init_all();
    printf("Memory experiment\n");

    // Set GPIO's 3 to output
    gpio_init(25) ;
    gpio_set_dir(25, GPIO_OUT);

    // Set GPIO 3 to zero
    gpio_put(25, 0) ;

    while (1) {
        
        printf("Function 1: %x\n", (unsigned long int)function_one) ;
        printf("Function 2: %x\n", (unsigned long int)function_two) ;
        printf("Variable location: %x\n\n", (unsigned long int)&variable) ;
        sleep_ms(5000) ;
    }

}
