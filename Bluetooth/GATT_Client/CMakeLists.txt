set(PICO_BOARD pico_w CACHE STRING "Board type")

# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(GATT_Client C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(GATT_Client)

target_sources(GATT_Client
                PRIVATE
                client.c)

target_link_libraries(GATT_Client
    pico_stdlib
    pico_sync
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    pico_bootsel_via_double_reset
    hardware_sync
    )
target_include_directories(GATT_Client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )
target_compile_definitions(GATT_Client PRIVATE
    RUNNING_AS_CLIENT=1
)

pico_add_extra_outputs(GATT_Client)