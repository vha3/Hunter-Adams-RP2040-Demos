add_executable(beep_beep beep_beep.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(beep_beep pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_spi hardware_sync)

# create map/bin/hex file etc.
pico_add_extra_outputs(beep_beep)