add_executable(multicore_dds multicore_dds.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(multicore_dds pico_stdlib pico_multicore hardware_sync hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_dds)