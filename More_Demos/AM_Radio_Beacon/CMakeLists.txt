add_executable(am_beacon)

# must match with executable name and source file names
target_sources(am_beacon PRIVATE am-beacon.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(am_beacon pico_stdlib pico_multicore hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(am_beacon)