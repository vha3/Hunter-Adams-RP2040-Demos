add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -Os)

add_executable(serial_worm)

target_sources(serial_worm PRIVATE serial_worm.c)

pico_set_linker_script(serial_worm ${CMAKE_CURRENT_SOURCE_DIR}/worm_bootloader.ld)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(serial_worm pico_stdlib hardware_dma hardware_uart hardware_flash hardware_watchdog hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(serial_worm)