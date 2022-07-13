add_executable(dma-demo dma-demo.c)

target_link_libraries(dma-demo pico_stdlib hardware_dma hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(dma-demo)

