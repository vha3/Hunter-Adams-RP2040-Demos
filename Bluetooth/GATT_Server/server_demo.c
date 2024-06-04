/**
 * V. Hunter Adams (vha3@cornell.edu)
 * vanhunteradams.com
 * June, 2024
 * 
 * 
 * Demonstration of a custom GATT service. Connect to Pico server
 * using LightBlue or a similar app.
 * 
 *  HARDWARE CONNECTIONS
 *  - GPIO 0  ---> UART RX (white)
 *  - GPIO 1  ---> UART TX (green)
 * 
 *  - GPIO 2  ---> Audio ISR timing GPIO
 * 
 *  - GPIO 5  ---> DAC CS
 *  - GPIO 6  ---> DAC SCK
 *  - GPIO 7  ---> DAC SDI
 *  - GND     ---> DAC LDAC
 * 
 *  - 3.3V    ---> DAC Vcc
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 470 ohm resistor ---> VGA Green 
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - GPIO 21 ---> 330 ohm resistor ---> VGA Red
 * 
 *  - RP2040 GND ---> VGA GND & DAC GND
 */


// Standard libraries
#include <stdio.h>
#include <math.h>

// BTstack
#include "btstack.h"

// High-level libraries
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"

// Hardware API's
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/sync.h"

// VGA driver
#include "VGA/vga16_graphics.h"

// GAP and GATT
#include "GAP_Advertisement/gap_config.h"
#include "GATT_Service/service_implementation.h"


// Low-level alarm infrastructure we'll be using for DDS
#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

// DDS parameters
#define two32 4294967296.0 // 2^32 
#define Fs 50000
#define DELAY 20 // 1/Fs (in microseconds)
#define two32overFs 85899
// the DDS units:
volatile unsigned int phase_accum_main;
volatile float freq = 400.0 ;
volatile unsigned int phase_incr_main = (400*two32)/Fs ; ;//

// SPI data
uint16_t DAC_data ; // output value

// DAC parameters
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000

// SPI configurations
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  6
#define PIN_MOSI 7
#define SPI_PORT spi0

// GPIO for timing the ISR
#define ISR_GPIO 2

// DDS sine table
#define sine_table_size 256
volatile int sin_table[sine_table_size] ;

// Period with which we'll enter the BTstack timer callback
#define HEARTBEAT_PERIOD_MS 250

// BTstack objects
static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;

// Some data that we will communicate over Bluetooth
static int characteristic_a_val = 0 ;

// We send data as formatted strings (just like a serial console)
static char characteristic_a_tx[100] ;
static char characteristic_b_tx[100] ;
static char characteristic_c_tx[100] ;
static char characteristic_d_rx[100] ;
static char characteristic_e_tx[5] ;
static char characteristic_f_tx[2] ;


// Alarm ISR for Direct Digital Synth
static void alarm_irq(void) {

    // Assert a GPIO when we enter the interrupt
    gpio_put(ISR_GPIO, 1) ;

    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    // Reset the alarm register
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY ;

    // DDS phase and sine table lookup
    phase_accum_main += phase_incr_main  ;
    DAC_data = (DAC_config_chan_A | ((sin_table[phase_accum_main>>24] + 2048) & 0xffff))  ;

    // Perform an SPI transaction
    spi_write16_blocking(SPI_PORT, &DAC_data, 1) ;

    // De-assert the GPIO when we leave the interrupt
    gpio_put(ISR_GPIO, 0) ;

}

// Heartbeat protothread
static PT_THREAD (protothread_heartbeat(struct pt *pt))
{
    PT_BEGIN(pt) ;

    // Buffer for writing counter to VGA
    static char countval[20] ;

    while(1) {

        // Increment the counter
        characteristic_a_val += 1 ;

        // Write the counter value to the VGA
        setCursor(65, 60) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("Counter value: ") ;
        sprintf(countval, "%d", characteristic_a_val) ;
        writeStringBig(countval) ;

        // Update characteristic (sends to client if notifications enabled)
        set_characteristic_a_value(characteristic_a_val) ;

        // Yield
        PT_YIELD_usec(500000) ;
    }

    PT_END(pt) ;
}

// Protothread that handles received Bluetooth data
static PT_THREAD (protothread_ble(struct pt *pt))
{
    PT_BEGIN(pt);

    // Character buffer for writing to VGA
    static char video_buffer[32];

    // Initialize the characteristic B value
    sprintf(characteristic_b_tx, "%f", freq) ;
    sprintf(characteristic_e_tx, "OFF") ;
    sprintf(characteristic_f_tx, "%d", 15) ;

    // Draw some filled rectangles
    fillRect(64, 0, 176, 50, BLUE); // blue box
    //fillRect(250, 0, 176, 50, DARK_ORANGE); // red box
    fillRect(435, 0, 176, 50, LIGHT_BLUE); // green box

    // Write some text
    setTextColor(WHITE) ;
    setCursor(65, 0) ;
    setTextSize(1) ;
    writeString("Raspberry Pi Pico W") ;
    setCursor(65, 10) ;
    writeString("GATT Server Demo") ;
    setCursor(65, 20) ;
    writeString("Hunter Adams") ;
    setCursor(65, 30) ;
    writeString("Bruce Land") ;
    setCursor(65, 40) ;
    writeString("ece4760 Cornell") ;
    setCursor(445, 10) ;
    setTextColor(BLACK) ;
    setTextSize(1) ;
    writeString("Protothreads rp2040 v1.3") ;
    setCursor(445, 20) ;
    writeString("VGA 4-bit color") ;
    setCursor(445, 30) ;
    writeString("Tested on picoW") ;
    setCursor(270, 20) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("GATT Server Test") ;
    setCursor(65, 80) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("DDS Frequency: ") ;
    sprintf(video_buffer, "%4.2f", freq) ;
    writeStringBig(video_buffer) ;
    writeStringBig(" Hz") ;
    fillRect(280, 120, 370, 15, BLACK);
    setCursor(65, 120) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("String from client to Pico: ") ;
    // Update LED status
    fillRect(120, 140, 370, 15, BLACK);
    setCursor(65, 140) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("LED Status: ") ;
    writeStringBig(characteristic_e_tx) ;
    fillRect(120, 160, 370, 15, BLACK); // blue box
    setCursor(65, 160) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("Color selection: ") ;
    sprintf(video_buffer, "%d", atoi(characteristic_f_tx)) ;
    writeStringBig(characteristic_f_tx) ;
    fillCircle(320, 350, 100, 15) ;

    while(1) {
        // Wait for a bluetooth event (signaled by bluetooth write callback)
        PT_SEM_SAFE_WAIT(pt, &BLUETOOTH_READY) ;

        // Update DDS frequency
        freq = atof(characteristic_b_tx) ;
        phase_incr_main = (unsigned int) freq*two32overFs;

        // Update DDS frequency video display
        setCursor(65, 80) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("DDS Frequency: ") ;
        sprintf(video_buffer, "%4.2f", freq) ;
        writeStringBig(video_buffer) ;
        writeStringBig(" Hz   ") ;

        // Update received string on VGA
        fillRect(280, 120, 370, 15, BLACK); // blue box
        setCursor(65, 120) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("String from client to Pico: ") ;
        writeStringBig(characteristic_d_rx) ;

        // Update LED status
        fillRect(120, 140, 370, 15, BLACK); // blue box
        setCursor(65, 140) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("LED Status: ") ;
        writeStringBig(characteristic_e_tx) ;

        // Update color selection
        fillRect(120, 160, 370, 15, BLACK); // blue box
        setCursor(65, 160) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("Color selection: ") ;
        sprintf(video_buffer, "%d", atoi(characteristic_f_tx)) ;
        writeStringBig(characteristic_f_tx) ;
        fillCircle(320, 350, 100, atoi(characteristic_f_tx)) ;


    }

  PT_END(pt);
}

// Serial input thread
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);

    // Send string to phone
    fillRect(280, 100, 370, 15, BLACK); // blue box
    setCursor(65, 100) ;
    setTextColor2(WHITE, BLACK) ;
    writeStringBig("String from Pico to client: ") ;

      while(1) {
        // print prompt
        sprintf(pt_serial_out_buffer, "Input a string: ");
        // spawn a thread to do the non-blocking write
        serial_write ;
        // spawn a thread to do the non-blocking serial read
        serial_read ;
        // Send string over bluetooth
        memcpy(characteristic_c_tx, pt_serial_in_buffer, sizeof(pt_serial_in_buffer));
        set_characteristic_c_value(characteristic_c_tx) ;

        // Send string to phone
        fillRect(280, 100, 370, 15, BLACK); // blue box
        setCursor(65, 100) ;
        setTextColor2(WHITE, BLACK) ;
        writeStringBig("String from Pico to client: ") ;
        writeStringBig(characteristic_c_tx) ;


      } // END WHILE(1)
  PT_END(pt);
} // timer thread

int main() {
    // Initialize stdio
    stdio_init_all();

    // Initialize the VGA
    initVGA() ;

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Initialize L2CAP and security manager
    l2cap_init();
    sm_init();

    // Initialize ATT server, no general read/write callbacks
    // because we'll set one up for each service
    att_server_init(profile_data, NULL, NULL);   

    // Instantiate our custom service handler
    custom_service_server_init( characteristic_a_tx, characteristic_b_tx,
                                characteristic_c_tx, characteristic_d_rx,
                                characteristic_e_tx, characteristic_f_tx) ;

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);

     // Initialize SPI channel (channel, baud rate set to 20MHz)
    spi_init(SPI_PORT, 20000000) ;
    spi_set_format(SPI_PORT, 16, 0, 0, 0);

    // Setup the ISR-timing GPIO
    gpio_init(ISR_GPIO) ;
    gpio_set_dir(ISR_GPIO, GPIO_OUT);
    gpio_put(ISR_GPIO, 0) ;

    // Map SPI signals to GPIO ports
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI) ;

    // === build the sine lookup table =======
    // scaled to produce values between 0 and 4096
    int ii;
    for (ii = 0; ii < sine_table_size; ii++){
         sin_table[ii] = (int)(2047*sin((float)ii*6.283/(float)sine_table_size));
    }

    // Enable the interrupt for the alarm (we're using Alarm 0)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM) ;
    // Associate an interrupt handler with the ALARM_IRQ
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq) ;
    // Enable the alarm interrupt
    irq_set_enabled(ALARM_IRQ, true) ;
    // Write the lower 32 bits of the target time to the alarm register, arming it.
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY ;

    // Add threads, start threader
    pt_add_thread(protothread_ble);
    pt_add_thread(protothread_serial);
    pt_add_thread(protothread_heartbeat) ;
    pt_sched_method = SCHED_ROUND_ROBIN ;
    pt_schedule_start ;
    
}
