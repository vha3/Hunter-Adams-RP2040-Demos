add_executable( gatt_server_demo 
                server_demo.c
                VGA/vga16_graphics.c
                )

# must match with pio filename and executable name from above
pico_generate_pio_header(gatt_server_demo ${CMAKE_CURRENT_LIST_DIR}/VGA/hsync.pio)
pico_generate_pio_header(gatt_server_demo ${CMAKE_CURRENT_LIST_DIR}/VGA/vsync.pio)
pico_generate_pio_header(gatt_server_demo ${CMAKE_CURRENT_LIST_DIR}/VGA/rgb.pio)



target_link_libraries(gatt_server_demo
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_irq
    hardware_spi
    hardware_timer
    hardware_sync
    hardware_pio
    hardware_dma
    )
target_include_directories(gatt_server_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )
pico_btstack_make_gatt_header(gatt_server_demo PRIVATE "${CMAKE_CURRENT_LIST_DIR}/GATT_Service/server_demo_gattfile.gatt")

pico_add_extra_outputs(gatt_server_demo)

