add_executable(picow_udp_beacon_background
        picow_udp_beacon.c
        )

target_include_directories(picow_udp_beacon_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_udp_beacon_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib hardware_sync
        )

pico_add_extra_outputs(picow_udp_beacon_background)

