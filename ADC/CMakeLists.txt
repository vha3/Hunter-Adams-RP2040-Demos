cmake_minimum_required(VERSION 3.12)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(simple_adc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(	  simple_adc
    		      ADC_demo.c
        	)

# pull in common dependencies
target_link_libraries(simple_adc 
    pico_stdlib 
    pico_sync
    hardware_sync
    hardware_clocks
    hardware_vreg
    hardware_clocks
    hardware_adc
    )

# create map/bin/hex file etc.
pico_add_extra_outputs(simple_adc)
