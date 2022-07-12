add_executable(protothreads_test)

# must match with executable name and source file names
target_sources(protothreads_test PRIVATE Protothreads_test_1.c)

# must match with executable name
target_link_libraries(protothreads_test PRIVATE pico_stdlib pico_bootsel_via_double_reset pico_multicore)

# must match with executable name
pico_add_extra_outputs(protothreads_test)

add_compile_options(-Ofast)
