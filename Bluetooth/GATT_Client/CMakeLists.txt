add_executable(GATT_Client
    client.c
    )
target_link_libraries(GATT_Client
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_sync
    )
target_include_directories(GATT_Client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )
target_compile_definitions(GATT_Client PRIVATE
    RUNNING_AS_CLIENT=1
)

pico_add_extra_outputs(GATT_Client)