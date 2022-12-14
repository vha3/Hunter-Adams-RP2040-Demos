add_executable(udp_transmitter)

pico_generate_pio_header(udp_transmitter ${CMAKE_CURRENT_LIST_DIR}/udp_transmit.pio)

target_sources(udp_transmitter PRIVATE udp_tx_demo.c)

target_link_libraries(udp_transmitter PRIVATE pico_stdlib pico_unique_id hardware_pio hardware_dma hardware_pwm)

pico_add_extra_outputs(udp_transmitter)
