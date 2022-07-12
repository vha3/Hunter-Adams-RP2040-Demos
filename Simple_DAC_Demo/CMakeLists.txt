add_executable(dactest dactest.c)

pico_add_extra_outputs(dactest)

target_link_libraries(dactest pico_stdlib hardware_spi)