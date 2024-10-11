add_executable(Stepper_Motors_position)

pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/counter.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/stepper1.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/counter1.pio)

target_sources(Stepper_Motors_position PRIVATE stepper.c)

target_link_libraries(Stepper_Motors_position PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(Stepper_Motors_position)
