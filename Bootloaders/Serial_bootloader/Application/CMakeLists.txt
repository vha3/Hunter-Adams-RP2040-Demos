add_executable(bootloader_blinky)

target_sources(bootloader_blinky PRIVATE bootloader_blinky.c)

pico_set_linker_script(bootloader_blinky ${CMAKE_CURRENT_SOURCE_DIR}/application_memmap.ld)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(bootloader_blinky pico_stdlib hardware_dma hardware_uart hardware_flash hardware_irq hardware_watchdog)

# create map/bin/hex file etc.
pico_add_extra_outputs(bootloader_blinky)