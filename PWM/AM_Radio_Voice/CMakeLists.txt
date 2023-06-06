add_executable(am_demo)

# must match with executable name and source file names
target_sources(am_demo PRIVATE am-demo.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(am_demo pico_stdlib pico_multicore hardware_pwm hardware_dma hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(am_demo)