cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(pio_ws2812 INTERFACE)
target_sources(pio_ws2812 PUBLIC pio-ws2812-rp2040/sdk/ws2812.c)

target_include_directories(pio_ws2812 INTERFACE pio-ws2812-rp2040/sdk)
pico_generate_pio_header(pio_ws2812 ${CMAKE_CURRENT_LIST_DIR}/pio-ws2812-rp2040/sdk/ws2812.pio)

target_link_libraries(pio_ws2812 INTERFACE
    pico_stdlib
    hardware_pio
    hardware_clocks
)
