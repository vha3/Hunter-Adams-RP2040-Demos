# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(VGA_Fonts_and_Colors_Demo C CXX ASM)

# initialize the sdk
pico_sdk_init()

# name anything you want
add_executable(VGA_Fonts_and_Colors_Demo)

# must match with pio filename and executable name from above
pico_generate_pio_header(VGA_Fonts_and_Colors_Demo ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(VGA_Fonts_and_Colors_Demo ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(VGA_Fonts_and_Colors_Demo ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(VGA_Fonts_and_Colors_Demo PRIVATE 
	vga16_graphics_font.c 
	vga16_graphics.c
)

# must match with executable name
target_link_libraries(VGA_Fonts_and_Colors_Demo PRIVATE 
	pico_stdlib 
	pico_bootsel_via_double_reset 
	hardware_pio 
	hardware_dma 
	hardware_adc 
	hardware_sync
	hardware_irq
	pico_multicore
)

# must match with executable name
pico_add_extra_outputs(VGA_Fonts_and_Colors_Demo)

add_compile_options(-Ofast)
