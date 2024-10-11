add_executable(Audio_Beep_Synth_Single_Core beep_beep.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(Audio_Beep_Synth_Single_Core pico_stdlib pico_bootsel_via_double_reset hardware_spi hardware_sync)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_Beep_Synth_Single_Core)