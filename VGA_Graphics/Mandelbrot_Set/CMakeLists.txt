# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(VGA_Mandelbrot_Set C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(VGA_Mandelbrot_Set)

# must match with pio filename and executable name from above
pico_generate_pio_header(VGA_Mandelbrot_Set ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(VGA_Mandelbrot_Set ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(VGA_Mandelbrot_Set ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(VGA_Mandelbrot_Set PRIVATE mandelbrot_fixvfloat.c vga16_graphics.c)

# must match with executable name
target_link_libraries(VGA_Mandelbrot_Set PRIVATE pico_stdlib pico_bootsel_via_double_reset pico_multicore hardware_pio hardware_dma)

# must match with executable name
pico_add_extra_outputs(VGA_Mandelbrot_Set)
