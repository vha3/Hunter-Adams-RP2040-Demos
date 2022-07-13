add_executable(mandelbrot-fixvfloat)

# must match with pio filename and executable name from above
pico_generate_pio_header(mandelbrot-fixvfloat ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(mandelbrot-fixvfloat ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(mandelbrot-fixvfloat ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(mandelbrot-fixvfloat PRIVATE mandelbrot_fixvfloat.c vga_graphics.c)

# must match with executable name
target_link_libraries(mandelbrot-fixvfloat PRIVATE pico_stdlib pico_multicore hardware_pio hardware_dma)

# must match with executable name
pico_add_extra_outputs(mandelbrot-fixvfloat)
