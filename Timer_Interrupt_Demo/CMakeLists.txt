add_executable(timer_irq timer_irq.c)

pico_add_extra_outputs(timer_irq)

target_link_libraries(timer_irq pico_stdlib)