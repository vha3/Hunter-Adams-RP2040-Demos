add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -Os)

add_executable(Serial_Bootloader)

target_sources(Serial_Bootloader PRIVATE bootloader.c)

pico_set_linker_script(Serial_Bootloader ${CMAKE_CURRENT_SOURCE_DIR}/bootloader_memmap.ld)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Serial_Bootloader pico_stdlib hardware_dma hardware_uart hardware_flash hardware_watchdog hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(Serial_Bootloader)