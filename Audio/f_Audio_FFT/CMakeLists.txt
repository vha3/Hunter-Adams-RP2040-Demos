add_executable(Audio_FFT_Demo)

# must match with pio filename and executable name from above
pico_generate_pio_header(Audio_FFT_Demo ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(Audio_FFT_Demo ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(Audio_FFT_Demo ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(Audio_FFT_Demo PRIVATE fft.c vga16_graphics.c)

# must match with executable name
target_link_libraries(Audio_FFT_Demo PRIVATE pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_pio hardware_dma hardware_adc hardware_irq)

# must match with executable name
pico_add_extra_outputs(Audio_FFT_Demo)