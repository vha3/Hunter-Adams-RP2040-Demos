add_executable(pwm_demo)

# must match with executable name and source file names
target_sources(pwm_demo PRIVATE pwm-demo.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(pwm_demo pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_pwm hardware_irq)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_demo)