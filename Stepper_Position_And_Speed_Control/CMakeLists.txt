add_executable(pio_stepper)

pico_generate_pio_header(pio_stepper ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(pio_stepper ${CMAKE_CURRENT_LIST_DIR}/pacer.pio)
pico_generate_pio_header(pio_stepper ${CMAKE_CURRENT_LIST_DIR}/counter.pio)

target_sources(pio_stepper PRIVATE stepper.c)

target_link_libraries(pio_stepper PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(pio_stepper)
