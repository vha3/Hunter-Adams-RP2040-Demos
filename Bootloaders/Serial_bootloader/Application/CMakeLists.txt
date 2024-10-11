add_executable(Serial_Bootloader_application)

target_sources(Serial_Bootloader_application PRIVATE bootloader_blinky.c)

pico_set_linker_script(Serial_Bootloader_application ${CMAKE_CURRENT_SOURCE_DIR}/application_memmap.ld)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Serial_Bootloader_application pico_stdlib hardware_dma hardware_uart hardware_flash hardware_irq hardware_watchdog)

# create map/bin/hex file etc.
pico_add_extra_outputs(Serial_Bootloader_application)