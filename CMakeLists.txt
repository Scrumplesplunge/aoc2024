cmake_minimum_required(VERSION 3.30)

# Configuring pico-sdk has to happen before `project(...)`.
set(PICO_BOARD pico_w)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pico-sdk")
include(third_party/pico-sdk/pico_sdk_init.cmake)

project(aoc2024 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
add_compile_options(
    -Wall -Wextra
    -Wno-psabi  # Disable warnings about ABI changes since older GCC versions.
)
pico_sdk_init()

add_subdirectory(common)
add_subdirectory(pico)
