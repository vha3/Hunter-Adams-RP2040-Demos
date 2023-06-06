add_executable(can_transciever)

pico_generate_pio_header(can_transciever ${CMAKE_CURRENT_LIST_DIR}/can.pio)

target_sources(can_transciever PRIVATE can_demo.c)

target_link_libraries(can_transciever PRIVATE pico_stdlib pico_multicore hardware_pio hardware_dma hardware_watchdog)

pico_add_extra_outputs(can_transciever)
