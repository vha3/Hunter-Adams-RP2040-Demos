add_executable(VGA_Conways_Game_of_Life)

# must match with pio filename and executable name from above
pico_generate_pio_header(VGA_Conways_Game_of_Life ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(VGA_Conways_Game_of_Life ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(VGA_Conways_Game_of_Life ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(VGA_Conways_Game_of_Life PRIVATE conway.c vga16_graphics.c)

# must match with executable name
target_link_libraries(VGA_Conways_Game_of_Life PRIVATE pico_stdlib hardware_pio hardware_dma)

# must match with executable name
pico_add_extra_outputs(VGA_Conways_Game_of_Life)
