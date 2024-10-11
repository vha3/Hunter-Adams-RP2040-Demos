add_executable(Audio_Beep_Synth_Multicore multitest.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(Audio_Beep_Synth_Multicore pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_sync hardware_spi hardware_timer)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_Beep_Synth_Multicore)