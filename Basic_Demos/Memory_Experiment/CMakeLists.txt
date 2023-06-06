add_executable(mem_experiment memory_experimentation.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(mem_experiment pico_stdlib)

# pico_set_binary_type(mem_experiment no_flash)

# create map/bin/hex file etc.
pico_add_extra_outputs(mem_experiment)