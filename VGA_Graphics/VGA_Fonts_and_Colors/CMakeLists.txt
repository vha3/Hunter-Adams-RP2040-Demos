# name anything you want
add_executable(graphics_text_4bit)

# must match with pio filename and executable name from above
pico_generate_pio_header(graphics_text_4bit ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(graphics_text_4bit ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(graphics_text_4bit ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(graphics_text_4bit PRIVATE 
	vga16_graphics_font.c 
	vga16_graphics.c
)

# must match with executable name
target_link_libraries(graphics_text_4bit PRIVATE 
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
pico_add_extra_outputs(graphics_text_4bit)

add_compile_options(-Ofast)
