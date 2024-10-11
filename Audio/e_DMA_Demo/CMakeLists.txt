add_executable(Audio_DMA_Demo dma-demo.c)

target_link_libraries(Audio_DMA_Demo pico_stdlib hardware_dma hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_DMA_Demo)

