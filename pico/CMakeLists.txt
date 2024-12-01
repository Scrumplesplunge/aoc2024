include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(pico main.cpp)
target_link_libraries(pico
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    solve      # Provides weak symbols for DayXX.
    solutions  # Provides strong symbols for DayXX.
    tcp
)
pico_enable_stdio_usb(pico 1)
pico_enable_stdio_uart(pico 0)
pico_add_extra_outputs(pico)

add_library(schedule schedule.cpp schedule.hpp)
target_link_libraries(schedule
    pico_cyw43_arch_lwip_threadsafe_background_headers
)

add_library(solve solve.cpp solve.hpp)
target_link_libraries(solve coro tcp)

add_library(tcp tcp.cpp tcp.hpp)
target_link_libraries(tcp
    coro
    delete_with
    pico_cyw43_arch_lwip_threadsafe_background_headers
    schedule
)
