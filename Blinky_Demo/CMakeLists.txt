add_executable(blinky)

target_sources(blinky PRIVATE blinky.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(blinky pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(blinky)

