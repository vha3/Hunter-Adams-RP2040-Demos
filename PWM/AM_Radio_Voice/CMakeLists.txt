add_executable(PWM_Voice_over_Radio)

# must match with executable name and source file names
target_sources(PWM_Voice_over_Radio PRIVATE am-demo.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(PWM_Voice_over_Radio pico_stdlib pico_multicore hardware_pwm hardware_dma hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(PWM_Voice_over_Radio)