cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(ov7670_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include(FetchContent)
FetchContent_Declare(
    camera_pico_ov7670
    GIT_REPOSITORY https://github.com/usedbytes/camera-pico-ov7670.git
    GIT_TAG main
)
FetchContent_MakeAvailable(camera_pico_ov7670)

add_executable(ov7670_test
    src/main.c
)

# Output over USB serial (no physical UART wiring needed)
pico_enable_stdio_usb(ov7670_test 1)
pico_enable_stdio_uart(ov7670_test 0)

pico_add_extra_outputs(ov7670_test)

target_link_libraries(ov7670_test
    camera
    pico_stdlib
    hardware_i2c
    hardware_gpio
)
