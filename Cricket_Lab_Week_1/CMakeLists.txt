add_executable(multitest multitest.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(multitest pico_stdlib pico_multicore hardware_sync hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(multitest)