add_executable(Blinky)

target_sources(Blinky PRIVATE blinky.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Blinky pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(Blinky)

