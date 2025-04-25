
/*
 FatFs license
 Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*
 * This code borrows heavily from the Mbed SDBlockDevice:
 *       https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html
 *       mbed-os/components/storage/blockdevice/COMPONENT_SD/SDBlockDevice.cpp
 *
 * Editor: Carl Kugler (carlk3@gmail.com)
 *
 * Remember your ABCs: "Always Be Cobbling!"
 *
 */

/*TINYUSB license (for the CLI commnads)
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Introduction
 * ------------
 * SD and MMC cards support a number of interfaces, but common to them all
 * is one based on SPI. Since we already have the mbed SPI Interface, it will
 * be used for SD cards.
 *
 * The main reference I'm using is Chapter 7, "SPI Mode" of:
 *  http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
 *
 * SPI Startup
 * -----------
 * The SD card powers up in SD mode. The start-up procedure is complicated
 * by the requirement to support older SDCards in a backwards compatible
 * way with the new higher capacity variants SDHC and SDHC.
 *
 * The following figures from the specification with associated text describe
 * the SPI mode initialisation process:
 *  - Figure 7-1: SD Memory Card State Diagram (SPI mode)
 *  - Figure 7-2: SPI Mode Initialization Flow
 *
 * Firstly, a low initial clock should be selected (in the range of 100-
 * 400kHZ). After initialisation has been completed, the switch to a
 * higher clock speed can be made (e.g. 1MHz). Newer cards will support
 * higher speeds than the default _transfer_sck defined here.
 *
 * Next, note the following from the SDCard specification (note to
 * Figure 7-1):
 *
 *  In any of the cases CMD1 is not recommended because it may be difficult for
 * the host to distinguish between MultiMediaCard and SD Memory Card
 *
 * Hence CMD1 is not used for the initialisation sequence.
 *
 * The SPI interface mode is selected by asserting CS low and sending the
 * reset command (CMD0). The card will respond with a (R1) response.
 * In practice many cards initially respond with 0xff or invalid data
 * which is ignored. Data is read until a valid response is received
 * or the number of re-reads has exceeded a maximim count. If a valid
 * response is not received then the CMD0 can be retried. This
 * has been found to successfully initialise cards where the SPI master
 * (on MCU) has been reset but the SDCard has not, so the first
 * CMD0 may be lost.
 *
 * CMD8 is optionally sent to determine the voltage range supported, and
 * indirectly determine whether it is a version 1.x SD/non-SD card or
 * version 2.x. I'll just ignore this for now.
 *
 * ACMD41 is repeatedly issued to initialise the card, until "in idle"
 * (bit 0) of the R1 response goes to '0', indicating it is initialised.
 *
 * You should also indicate whether the host supports High Capicity cards,
 * and check whether the card is high capacity - i'll also ignore this.
 *
 * SPI Protocol
 * ------------
 * The SD SPI protocol is based on transactions made up of 8-bit words, with
 * the host starting every bus transaction by asserting the CS signal low. The
 * card always responds to commands, data blocks and errors.
 *
 * The protocol supports a CRC, but by default it is off (except for the
 * first reset CMD0, where the CRC can just be pre-calculated, and CMD8)
 * I'll leave the CRC off I think!
 *
 * Standard capacity cards have variable data block sizes, whereas High
 * Capacity cards fix the size of data block to 512 bytes. I'll therefore
 * just always use the Standard Capacity cards with a block size of 512 bytes.
 * This is set with CMD16.
 *
 * You can read and write single blocks (CMD17, CMD25) or multiple blocks
 * (CMD18, CMD25). For simplicity, I'll just use single block accesses. When
 * the card gets a read command, it responds with a response token, and then
 * a data token or an error.
 *
 * SPI Command Format
 * ------------------
 * Commands are 6-bytes long, containing the command, 32-bit argument, and CRC.
 *
 * +---------------+------------+------------+-----------+----------+--------------+
 * | 01 | cmd[5:0] | arg[31:24] | arg[23:16] | arg[15:8] | arg[7:0] | crc[6:0] |
 * 1 |
 * +---------------+------------+------------+-----------+----------+--------------+
 *
 * As I'm not using CRC, I can fix that byte to what is needed for CMD0 (0x95)
 *
 * All Application Specific commands shall be preceded with APP_CMD (CMD55).
 *
 * SPI Response Format
 * -------------------
 * The main response format (R1) is a status byte (normally zero). Key flags:
 *  idle - 1 if the card is in an idle state/initialising
 *  cmd  - 1 if an illegal command code was detected
 *
 *    +-------------------------------------------------+
 * R1 | 0 | arg | addr | seq | crc | cmd | erase | idle |
 *    +-------------------------------------------------+
 *
 * R1b is the same, except it is followed by a busy signal (zeros) until
 * the first non-zero byte when it is ready again.
 *
 * Data Response Token
 * -------------------
 * Every data block written to the card is acknowledged by a byte
 * response token
 *
 * +----------------------+
 * | xxx | 0 | status | 1 |
 * +----------------------+
 *              010 - OK!
 *              101 - CRC Error
 *              110 - Write Error
 *
 * Single Block Read and Write
 * ---------------------------
 *
 * Block transfers have a byte header, followed by the data, followed
 * by a 16-bit CRC. In our case, the data will always be 512 bytes.
 *
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 * | 0xFE | data[0] | data[1] |        | data[n] | crc[15:8] | crc[7:0] |
 * +------+---------+---------+- -  - -+---------+-----------+----------+
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

// ==================================================
// === toggle25 thread on core 0
// ==================================================
// the on-board LED blinks
static PT_THREAD(protothread_toggle25(struct pt *pt))
{
    PT_BEGIN(pt);
    static bool LED_state = false;

    // set up LED p25 to blink
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, true);
    // data structure for interval timer
    PT_INTERVAL_INIT();

    while (1)
    {
        // yield time 0.5 second
        // PT_YIELD_usec(100000) ;
        PT_YIELD_INTERVAL(500000);

        // toggle the LED on PICO
        LED_state = LED_state ? false : true;
        gpio_put(25, LED_state);
        //
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // blink thread

// ===========================================
// serial and file i/o
// ===========================================
// The command interpreter is largely copied from
// the tinyusb disribution.
// https://github.com/hathach/tinyusb/blob/master/examples/host/msc_file_explorer/src/main.c
// See license at top of file
// ===========================================
static PT_THREAD(protothread_file(struct pt *pt))
{
    PT_BEGIN(pt);
    // static char serial_buffer[40];
    static char cmd[16], arg1[64], arg2[16], arg3[16];
    static char *token;
    // float farg;
    // int arg;
    // static long long start_time;

    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];

    // Default datetime set to Sunday 01 January 00:00:00 2000
    datetime_t t = {
        .year = 2000,
        .month = 01,
        .day = 01,
        .dotw = 0, // 0 is Sunday, so 5 is Friday
        .hour = 00,
        .min = 00,
        .sec = 00};
    rtc_init();
    rtc_set_datetime(&t);
    sleep_ms(100);

    static int logging_frequency_ms = 5000; // Default logging frequency [ms] -- 5 seconds

    // Initialize SD card
    if (!sd_init_driver())
    {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true);
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true);
    }

    while (1)
    {
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

        // ===
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
            // Bruce's methods
            // printf("write filename number -- text data write -- writes ints to number\n\r");
            // printf("read filename -- text data read -- ints (from write command)\n\r");
            // printf("writebin filename range  -- demo binary data write -- ints to range\n\r");
            // printf("readbin filename range  -- demo binary data read -- ints to range\n\r");
            // printf("plot filename -- data to VGA\n\r") ;
            printf("*****\n\r");
        }
        
        // ===
        if (strcmp(cmd, "setdate") == 0)
        {
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
                static int calc_info[] = {0,3,2,5,0,3,5,1,4,6,2,4};
                int calc_year = atoi(year) - (atoi(month) < 3);

                datetime_t t = {
                    .year = atoi(year),
                    .month = atoi(month),
                    .day = atoi(day),
                    .dotw = (calc_year + calc_year/4 - calc_year/100 + calc_year/400 + calc_info[atoi(month)-1] + atoi(day)) % 7, // 0 is Sunday, so 5 is Friday
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
                    char *DATA = "12.2,10,70,45\n";
                    UINT wr_count = 0;

                    rtc_get_datetime(&t);
                    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
                    sleep_ms(100);

                    printf("writing to file...\n");
                    f_write(&f_dst, datetime_str, strlen(datetime_str), &wr_count);
                    sleep_ms(20);
                    f_write(&f_dst, ",", strlen(","), &wr_count);
                    sleep_ms(20);
                    f_write(&f_dst, DATA, strlen(DATA), &wr_count);
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


        // // === Bruce's Original write
        // if (strcmp(cmd, "write") == 0)
        // {
        //     FIL f_dst;
        //     if (FR_OK != f_open(&f_dst, arg1, FA_WRITE | FA_CREATE_ALWAYS))
        //     {
        //         printf("cannot create '%s'\r\n", arg1);
        //     }
        //     else
        //     {
        //         // uint8_t buf[32] = "Data from Pico\n\r";
        //         UINT wr_count = 0;
        //         uint32_t range;
        //         sscanf(arg2, "%d\0  ", &range);
        //         sprintf(arg2, "%d ", range);
        //         if (FR_OK != f_write(&f_dst, arg2, strlen(arg2), &wr_count))
        //         {
        //             printf("cannot write to '%s'\r\n", arg1);
        //         }

        //         for (int i = 0; i <= range; i++)
        //         {
        //             uint8_t data[10];
        //             sprintf(data, "%d\n", i);
        //             f_write(&f_dst, data, strlen(data), &wr_count);
        //         }
        //     }
        //     f_close(&f_dst);
        // }

        // // ===
        // if (strcmp(cmd, "writebin") == 0)
        // {
        //     FIL f_dst;
        //     if (FR_OK != f_open(&f_dst, arg1, FA_WRITE | FA_CREATE_ALWAYS))
        //     {
        //         printf("cannot create '%s'\r\n", arg1);
        //     }
        //     else
        //     {
        //         UINT wr_count = 0;
        //         // if ( FR_OK != f_write(&f_dst, buf, strlen(buf), &wr_count) )
        //         // {
        //         //     printf("cannot write to '%s'\r\n", arg1);
        //         // }
        //         uint32_t range;
        //         sscanf(arg2, "%d", &range);
        //         for (int i = 0; i <= range; i += 1)
        //         {
        //             data_array[i] = 2 * i;
        //         }
        //         // total byte count is range*4
        //         start_time = PT_GET_TIME_usec();
        //         f_write(&f_dst, data_array, range * 4, &wr_count);
        //         printf("uSec/int =  %lld  \n", (PT_GET_TIME_usec() - start_time) / range);
        //         // for checking read
        //         for (int i = 0; i <= range; i += 1)
        //         {
        //             data_array[i] = 0;
        //         }
        //     }
        //     f_close(&f_dst);
        // }

        // // ===
        // if (strcmp(cmd, "read") == 0)
        // {
        //     FIL fi;
        //     if (FR_OK != f_open(&fi, arg1, FA_READ))
        //     {
        //         printf("%s: No such file or directory\r\n", arg1);
        //     }
        //     else
        //     {
        //         uint8_t buf[100];
        //         UINT count = 0;
        //         // modify ff.c line 6848
        //         // turn on f_gets option in ffconf.h
        //         //
        //         // get number of data points
        //         int file_count;
        //         f_gets(buf, sizeof(buf), &fi);
        //         sscanf(buf, "%d", &file_count);
        //         // read and plot
        //         for (int i = 0; i <= file_count; i++)
        //         {
        //             f_gets(buf, sizeof(buf), &fi);
        //             printf("%s", buf);
        //         }
        //     }
        //     f_close(&fi);
        // }

        // // ===
        // if (strcmp(cmd, "readbin") == 0)
        // {
        //     FIL fi;
        //     if (FR_OK != f_open(&fi, arg1, FA_READ))
        //     {
        //         printf("%s: No such file or directory\r\n", arg1);
        //     }
        //     else
        //     {
        //         // uint8_t buf[512];
        //         uint32_t range;
        //         sscanf(arg2, "%d", &range);
        //         start_time = PT_GET_TIME_usec();
        //         UINT count = 0;
        //         while ((FR_OK == f_read(&fi, data_array, range * 4, &count)) && (count > 0))
        //         {
        //             // for(UINT c = 0; c < count; c++)
        //             // {
        //             //     const uint8_t ch = buf[c];
        //             //     putchar(ch);
        //             // }
        //         }
        //         printf("uSec/int =  %lld  \n", (PT_GET_TIME_usec() - start_time) / range);
        //         // just sample the file start
        //         for (int i = 0; i <= 10; i += 1)
        //         {
        //             printf("%d\n", data_array[i]);
        //         }
        //     }
        //     f_close(&fi);
        // }

        // NEVER exit while
    } // END WHILE(1)
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
    pt_add_thread(protothread_toggle25);

    //
    // === initalize the scheduler ===============
    pt_sched_method = SCHED_PRIORITY;
    pt_schedule_start;
    // NEVER exits
    // ===========================================
} // end main