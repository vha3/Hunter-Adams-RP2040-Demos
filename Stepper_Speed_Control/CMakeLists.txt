add_executable(pio_stepper_speed_control)

pico_generate_pio_header(pio_stepper_speed_control ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(pio_stepper_speed_control ${CMAKE_CURRENT_LIST_DIR}/pacer.pio)
pico_generate_pio_header(pio_stepper_speed_control ${CMAKE_CURRENT_LIST_DIR}/counter.pio)
pico_generate_pio_header(pio_stepper_speed_control ${CMAKE_CURRENT_LIST_DIR}/stepper1.pio)
pico_generate_pio_header(pio_stepper_speed_control ${CMAKE_CURRENT_LIST_DIR}/pacer1.pio)


target_sources(pio_stepper_speed_control PRIVATE stepper.c)

target_link_libraries(pio_stepper_speed_control PRIVATE pico_stdlib hardware_pio hardware_dma hardware_irq)
pico_add_extra_outputs(pio_stepper_speed_control)
