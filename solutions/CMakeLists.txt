include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../pico")

# The solutions are exposed as an OBJECT library which exposes strong linker
# symbols for each DayXX function. These override the weak symbols defined in
# pico/solve.cpp.
add_library(solutions OBJECT
    day01.cpp day02.cpp day03.cpp day04.cpp day05.cpp day06.cpp day07.cpp
)
target_link_libraries(solutions
    PRIVATE coro tcp pico_cyw43_arch_lwip_threadsafe_background_headers
)
