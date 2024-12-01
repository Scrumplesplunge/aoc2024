include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(pico main.cpp)
target_link_libraries(pico
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)
pico_enable_stdio_usb(pico 1)
pico_enable_stdio_uart(pico 0)
pico_add_extra_outputs(pico)
