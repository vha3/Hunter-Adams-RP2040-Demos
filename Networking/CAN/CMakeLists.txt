add_executable(CAN_Bus)

pico_generate_pio_header(CAN_Bus ${CMAKE_CURRENT_LIST_DIR}/can.pio)

target_sources(CAN_Bus PRIVATE can_demo.c)

target_link_libraries(CAN_Bus PRIVATE pico_stdlib pico_multicore hardware_pio hardware_dma hardware_watchdog)

pico_add_extra_outputs(CAN_Bus)
