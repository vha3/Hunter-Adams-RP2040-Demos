add_executable(conway)

# must match with pio filename and executable name from above
pico_generate_pio_header(conway ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(conway ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(conway ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(conway PRIVATE conway.c vga_graphics.c)

# must match with executable name
target_link_libraries(conway PRIVATE pico_stdlib hardware_pio hardware_dma)

# must match with executable name
pico_add_extra_outputs(conway)
