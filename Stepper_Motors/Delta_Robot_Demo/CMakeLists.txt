add_executable(delta)

pico_generate_pio_header(delta ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(delta ${CMAKE_CURRENT_LIST_DIR}/counter.pio)
pico_generate_pio_header(delta ${CMAKE_CURRENT_LIST_DIR}/stepper1.pio)
pico_generate_pio_header(delta ${CMAKE_CURRENT_LIST_DIR}/counter1.pio)

target_sources(delta PRIVATE stepper.c)

target_link_libraries(delta PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(delta)
