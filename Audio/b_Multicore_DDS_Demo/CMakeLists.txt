add_executable(Audio_Multicore_DDS_Demo multicore_dds.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(Audio_Multicore_DDS_Demo pico_stdlib pico_multicore hardware_sync hardware_spi hardware_timer hardware_irq)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_Multicore_DDS_Demo)