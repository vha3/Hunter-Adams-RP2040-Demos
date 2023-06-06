add_executable(irq_timing irq_timing.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(irq_timing pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(irq_timing)