add_executable(GPIO_Interrupt_Demo irq_timing.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(GPIO_Interrupt_Demo pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(GPIO_Interrupt_Demo)