add_executable(Protothreads_Demo)

# must match with executable name and source file names
target_sources(Protothreads_Demo PRIVATE Protothreads_test_1_3.c)

# must match with executable name
target_link_libraries(Protothreads_Demo PRIVATE pico_stdlib pico_bootsel_via_double_reset pico_multicore)

# must match with executable name
pico_add_extra_outputs(Protothreads_Demo)

add_compile_options(-Ofast)
