if (PICO_CYW43_SUPPORTED) # set by PICO_BOARD=pico_w
    if (NOT TARGET pico_cyw43_arch)
        message("Skipping Pico W examples as support is not available. Set PICO_BOARD=pico_w to enable")
    else()
        add_subdirectory(UDP)
    endif()
endif()