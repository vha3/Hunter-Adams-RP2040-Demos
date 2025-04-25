/**
 * Flux Chamber - SD card & Methane Sensor Integration
 * Authors: Grace Lo, Alina Wang
 * Date: 2025-04
 *
 *
 * HARDWARE CONNECTIONS
 * SERIAL
 * - GPIO 0 (TX)    -->     UART RX (white)
 * - GPIO 1 (RX)    -->     UART TX (green)
 * - RP2040 GND     -->     UART GND
 *
 * SD CARD
 * - Vin (5V)       -->     VBUS (pin 40)
 * - GND            -->     GND
 * - CLK            -->     SPI0_SCK (GPIO 2)
 * - DO             -->     SPI0_RX (GPIO 4)
 * - DI             -->     SPI0_TX (GPIO 3)
 * - CS             -->     SPI0_CSn (GPIO 5)
 * - CD             -->     GPIO 22
 *
 * ADC
 * - GPIO 26 (ADC 0)  <--   Methane Vout
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

//  FatFs sd card
#include "sd_card.h" // FatFs_SPI/sd_driver/
#include "ff.h"      // FatFs_SPI/ff14a/source/
FRESULT fr;
FATFS fs;

// protothreads
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
// protothreads header
#include "hardware/uart.h"
#include "pt_cornell_rp2040_v1_3.h"
// int data_array[1000];

// data logging
#include "pico/unique_id.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

// analog sensor
#include "hardware/gpio.h"
#include "hardware/adc.h"

// define GPIO connections
#define LED 25
#define ADC0_PIN 26

// ---------------- Initialize RTC variables ----------------
static char cmd[16], arg1[64], arg2[16], arg3[16];
static char *token;
char datetime_buf[256];
char *datetime_str = &datetime_buf[0];
// default datetime set to Sunday 01 January 00:00:00 2000
datetime_t t = {
    .year = 2000,
    .month = 01,
    .day = 01,
    .dotw = 0, // 0 is Sunday, so 5 is Friday
    .hour = 00,
    .min = 00,
    .sec = 00};

// ==================================================
// === toggle25 thread on core 0
// ==================================================
// the on-board LED blinks
static PT_THREAD(protothread_LED25Blink(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false;

    // set up LED p25 to blink
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, true);
    // data structure for interval timer
    PT_INTERVAL_INIT();

    while (1)
    {
        // yield time 0.5 second
        // PT_YIELD_usec(100000) ;
        PT_YIELD_INTERVAL(500000);

        // toggle the LED on PICO
        LED_state = LED_state ? false : true;
        gpio_put(LED, LED_state);
        //
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // blink thread

// ===========================================
// Setup Real Time Clock
// ===========================================
static PT_THREAD(protothread_RTC(struct pt *pt))
{
    PT_BEGIN(pt);
    rtc_init();
    rtc_set_datetime(&t);
    sleep_ms(100);

    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    sleep_ms(100);
    // printf("\n\r%s      \n", datetime_str);

    printf(">>");
    // spawn a thread to do the non-blocking serial read
    serial_read;
    // tokenize
    token = strtok(pt_serial_in_buffer, "  ");
    strcpy(cmd, token);
    token = strtok(NULL, "  ");
    strcpy(arg1, token);
    token = strtok(NULL, "  ");
    strcpy(arg2, token);
    token = strtok(NULL, "  ");
    strcpy(arg3, token);

    PT_END(pt);
}

// ===========================================
// serial and file i/o
// The command interpreter is based on the following
// https://github.com/hathach/tinyusb/blob/master/examples/host/msc_file_explorer/src/main.c
// ===========================================
static PT_THREAD(protothread_file(struct pt *pt))
{
    PT_BEGIN(pt);
    // static char cmd[16], arg1[64], arg2[16], arg3[16];
    // static char *token;
    // char datetime_buf[256];
    // char *datetime_str = &datetime_buf[0];

    // // Default datetime set to Sunday 01 January 00:00:00 2000
    // datetime_t t = {
    //     .year = 2000,
    //     .month = 01,
    //     .day = 01,
    //     .dotw = 0, // 0 is Sunday, so 5 is Friday
    //     .hour = 00,
    //     .min = 00,
    //     .sec = 00};
    // rtc_init();
    // rtc_set_datetime(&t);
    // sleep_ms(100);

    // TESTING
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(ADC0_PIN);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);

    // Default logging frequency [ms] -- 5 seconds
    static int logging_frequency_ms = 5000;

    // Initialize SD card
    if (!sd_init_driver())
    {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true)
            ;
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true)
            ;
    }

    // rtc_get_datetime(&t);
    // datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    // sleep_ms(100);
    // // printf("\n\r%s      \n", datetime_str);

    // printf(">>");
    // // spawn a thread to do the non-blocking serial read
    // serial_read;
    // // tokenize
    // token = strtok(pt_serial_in_buffer, "  ");
    // strcpy(cmd, token);
    // token = strtok(NULL, "  ");
    // strcpy(arg1, token);
    // token = strtok(NULL, "  ");
    // strcpy(arg2, token);
    // token = strtok(NULL, "  ");
    // strcpy(arg3, token);

    if (strcmp(cmd, "help") == 0)
    {
        printf("*****\n\r");
        printf("[1] setdate <yyy-mm-dd> <hh:mm:ss> -- set the RTC intialization of date/time\n\r");
        printf("[2] setfreq <frequency> -- set the data logging frequency in ms\n\r");
        printf("[3] write <filename> -- write the Pico unique ID, date/time, and data to file saved as .txt or .csv (specify format in filename)\n\r");
        printf("[4] print <filename> -- print the content of the filename\n\r");
        printf("[5] rm <filename> -- delinks (deletes) a file\n\r");
        printf("[6] ls <directory> -- list directory contents\n\r");
        printf("[7] cd <directory> -- changes current directory\n\r");
        printf("[8] mkdir <directory> -- new directory\n\r");
        printf("*****\n\r");
    }

    // ===
    if (strcmp(cmd, "setdate") == 0)
    {
        PT_YIELD(pt);
        static char year[16], month[16], day[16], hour[16], minute[16], second[16];
        static char *datetoken, *timetoken;
        char date, time;
        if (sscanf(arg1, "%s", &date) == 1 && sscanf(arg2, "%s", &time) == 1)
        {
            datetoken = strtok(&date, "-");
            strcpy(year, datetoken);
            datetoken = strtok(NULL, "-");
            strcpy(month, datetoken);
            datetoken = strtok(NULL, "-");
            strcpy(day, datetoken);

            timetoken = strtok(&time, ":");
            strcpy(hour, timetoken);
            timetoken = strtok(NULL, ":");
            strcpy(minute, timetoken);
            timetoken = strtok(NULL, ":");
            strcpy(second, timetoken);

            printf("Setting RTC to %s-%s-%s %s:%s:%s\n\r", &year, &month, &day, &hour, &minute, &second);

            // For day of the week calculation
            static int calc_info[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
            int calc_year = atoi(year) - (atoi(month) < 3);

            datetime_t t = {
                .year = atoi(year),
                .month = atoi(month),
                .day = atoi(day),
                .dotw = (calc_year + calc_year / 4 - calc_year / 100 + calc_year / 400 + calc_info[atoi(month) - 1] + atoi(day)) % 7, // 0 is Sunday, so 5 is Friday
                .hour = atoi(hour),
                .min = atoi(minute),
                .sec = atoi(second)};
            rtc_init();
            rtc_set_datetime(&t);
            sleep_ms(100);
        }
        else
        {
            printf("Invalid date format: %s %s\n\r", arg1, arg2);
        }
    }

    // ===
    if (strcmp(cmd, "setfreq") == 0)
    {
        int freq;
        if (sscanf(arg1, "%d", &freq) == 1 && freq > 0)
        {
            logging_frequency_ms = freq;
            printf("Logging frequency set to %d ms\n", logging_frequency_ms);
        }
        else
        {
            printf("Invalid frequency. Please enter a positive integer.\n");
        }
    }

    // ===
    if (strcmp(cmd, "write") == 0)
    {
        FIL f_dst;
        pico_unique_board_id_t PICO_ID;

        if (FR_OK != f_open(&f_dst, arg1, FA_WRITE | FA_OPEN_APPEND))
        {
            printf("Cannot create '%s'\r\n", arg1);
        }
        else
        {
            // Write header
            UINT wr_count = 0;

            pico_get_unique_board_id(&PICO_ID);
            char id_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2]; // Buffer for hexadecimal string
            for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
            {
                // Append each byte in hexadecimal format to the buffer
                sprintf(&id_str[i * 2], "%02X", PICO_ID.id[i]);
            }
            printf("ID is: %s \n", id_str);

            char *DATA_HEADER = "DATETIME,CH4,CO2,TEMP,HUM\n";

            f_write(&f_dst, "Pico ID,", strlen("Pico ID,"), &wr_count);
            sleep_ms(20);                                       // Delay for write to complete
            f_write(&f_dst, id_str, strlen(id_str), &wr_count); // Write the ID string to the file
            sleep_ms(20);
            f_write(&f_dst, "\n\r", strlen("\n\r"), &wr_count);
            sleep_ms(20);
            f_write(&f_dst, DATA_HEADER, strlen(DATA_HEADER), &wr_count);
            sleep_ms(20);

            // Write contents
            int test = 3;
            while (test > 0)
            {
                test--;
                sleep_ms(logging_frequency_ms);
                char *NEW_LINE = "\n";
                UINT wr_count = 0;

                rtc_get_datetime(&t);
                datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
                sleep_ms(100);

                printf("writing to file...\n");
                f_write(&f_dst, datetime_str, strlen(datetime_str), &wr_count);
                sleep_ms(20);
                f_write(&f_dst, ",", strlen(","), &wr_count);
                sleep_ms(20);

                uint16_t methane = adc_read();
                printf("CH4: %d\n", methane);
                char methane_str[10] = {0};
                sprintf(methane_str, "%d", methane);
                f_write(&f_dst, methane_str, strlen(methane_str), &wr_count);
                sleep_ms(20);

                f_write(&f_dst, NEW_LINE, strlen(NEW_LINE), &wr_count);
                sleep_ms(20);
            }
            printf("Exited while loop\n");
        }
        f_close(&f_dst);
    }

    // ===
    if (strcmp(cmd, "print") == 0)
    {
        FIL fi;
        if (FR_OK != f_open(&fi, arg1, FA_READ))
        {
            printf("%s: No such file or directory\r\n", arg1);
        }
        else
        {
            uint8_t buf[512];
            UINT count = 0;
            while ((FR_OK == f_read(&fi, buf, sizeof(buf), &count)) && (count > 0))
            {
                for (UINT c = 0; c < count; c++)
                {
                    const uint8_t ch = buf[c];
                    putchar(ch);
                }
            }
            printf("\n\r");
        }
        f_close(&fi);
    }

    // ===
    if (strcmp(cmd, "rm") == 0)
    {
        const char *fpath = arg1; // token count from 1
        if (FR_OK != f_unlink(fpath))
        {
            printf("cannot remove '%s': No such file or directory\r\n", fpath);
        }
    }

    // ===
    if (strcmp(cmd, "ls") == 0)
    {
        // default is current directory
        const char *dpath = ".";
        if (arg1)
            dpath = arg1;

        DIR dir;
        if (FR_OK != f_opendir(&dir, dpath))
        {
            printf("cannot access '%s': No such file or directory\r\n", dpath);
        }

        char path[256];
        if (FR_OK != f_getcwd(path, sizeof(path)))
        {
            printf("cannot get current working directory\r\n");
        }

        // puts(path);
        printf("Current directory: %s\n\r", path);

        FILINFO fno;
        while ((f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != 0))
        {
            if (fno.fname[0] != '.') // ignore . and .. entry
            {
                if (fno.fattrib & AM_DIR)
                {
                    // directory
                    printf("/%s\r\n", fno.fname);
                }
                else
                {
                    printf("%-40s", fno.fname);
                    if (fno.fsize < 1000)
                    {
                        printf("%llu B\r\n", fno.fsize);
                    }
                    else
                    {
                        printf("%llu KB\r\n", fno.fsize / 1000);
                    }
                }
            }
        }
        f_closedir(&dir);
    }

    // ===
    if (strcmp(cmd, "cd") == 0)
    {
        // default is current directory
        const char *dpath = arg1;

        if (FR_OK != f_chdir(dpath))
        {
            printf("%s: No such file or directory\r\n", dpath);
        }
    }

    // ===
    if (strcmp(cmd, "mkdir") == 0)
    {
        const char *dpath = arg1;
        if (FR_OK != f_mkdir(dpath))
        {
            printf("%s: cannot create this directory\r\n", dpath);
        }
    }

    PT_END(pt);
} // file thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main()
{
    //
    //  === add threads  ====================
    // for core 1
    // pt_add_thread(protothread_vga) ;
    // initialize datetime storing variables
    static char cmd[16], arg1[64], arg2[16], arg3[16];
    static char *token;
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    pt_add_thread(protothread_RTC);
    pt_add_thread(protothread_file);
    //
    // === initalize the scheduler ==========
    pt_sched_method = SCHED_ROUND_ROBIN;

    pt_schedule_start;
    // NEVER exits
    // ======================================
}

// ========================================
// === core 0 main
// ========================================
int main()
{

    // board_init();
    //  start the serial i/o
    stdio_init_all();

    // start core 1 threads
    multicore_reset_core1();
    multicore_launch_core1(&core1_main);

    // === config threads ========================
    // for core 0
    pt_add_thread(protothread_LED25Blink);

    //
    // === initalize the scheduler ===============
    pt_sched_method = SCHED_PRIORITY;
    pt_schedule_start;
    // NEVER exits
    // ===========================================
} // end main