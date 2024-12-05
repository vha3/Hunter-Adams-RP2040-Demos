#include <stdio.h>
//
// #include "C:\Users\16073\OneDrive\Documents\Pico-v1.5.0\FatFs_SPI\include\f_util.h"
// #include "C:\Program Files\Raspberry Pi\Pico SDK v1.5.0\pico-sdk\lib\tinyusb\lib\fatfs\source\ff.h"
// #include "stdlib.h"
// #include "C:\Users\16073\OneDrive\Documents\Pico-v1.5.0\FatFs_SPI\include\rtc.h"
// //

#include "..\SD_Card\FatFs_SPI\sd_driver\hw_config.h"

#include "..\SD_Card\FatFs_SPI\include\f_util.h"
#include "E:\picoSDK\pico-sdk\lib\tinyusb\lib\fatfs\source\ff.h"
#include "pico/stdlib.h"
#include "..\SD_Card\FatFs_SPI\include\rtc.h"
#include "..\SD_Card\FatFs_SPI\sd_driver\hw_config.h"

int main()
{
    stdio_init_all();
    time_init();

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr)
        panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char *const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0)
    {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
    {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

    puts("Goodbye, world!");
    for (;;)
        ;
}
