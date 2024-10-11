add_executable(PWM_Radio_Beacon)

# must match with executable name and source file names
target_sources(PWM_Radio_Beacon PRIVATE am-beacon.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(PWM_Radio_Beacon pico_stdlib pico_multicore hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(PWM_Radio_Beacon)