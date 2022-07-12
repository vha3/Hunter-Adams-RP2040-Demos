cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(hunter_pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(HUNTER_PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

# Add multi example
add_subdirectory(Lab_1_Week_1)
add_subdirectory(Lab_1_Week_3)
add_subdirectory(Lab_2_Week_1)
add_subdirectory(Lab_3_Week_1)
add_subdirectory(Lab_3_Week_2)
add_subdirectory(Protothreads_Demo)
add_subdirectory(Stepper_Position_And_Speed_Control)
add_subdirectory(Stepper_Position_Control)
add_subdirectory(Stepper_Speed_Control)
add_subdirectory(VGA_Graphics_Demo)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )