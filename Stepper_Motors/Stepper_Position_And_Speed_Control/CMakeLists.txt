add_executable(Stepper_Motors_position_and_speed)

pico_generate_pio_header(Stepper_Motors_position_and_speed ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(Stepper_Motors_position_and_speed ${CMAKE_CURRENT_LIST_DIR}/pacer.pio)
pico_generate_pio_header(Stepper_Motors_position_and_speed ${CMAKE_CURRENT_LIST_DIR}/counter.pio)

target_sources(Stepper_Motors_position_and_speed PRIVATE stepper.c)

target_link_libraries(Stepper_Motors_position_and_speed PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(Stepper_Motors_position_and_speed)
