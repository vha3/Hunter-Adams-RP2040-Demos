add_executable(dactest)

target_sources(dactest PRIVATE dactest.c)

target_link_libraries(dactest pico_stdlib hardware_spi)

pico_add_extra_outputs(dactest)