add_executable(Memory_Experiment memory_experimentation.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Memory_Experiment pico_stdlib)

# pico_set_binary_type(Memory_Experiment no_flash)

# create map/bin/hex file etc.
pico_add_extra_outputs(Memory_Experiment)