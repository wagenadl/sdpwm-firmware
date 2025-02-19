## CMakeLists.txt for picoDAQ / DAW 2024 -*- Makefile -*-

# The order of declarations matters: The Pico SDK “import” must be
# included before the “project” declaration; the SDK “init” must be
# called after.

## Boiler plate cmake stuff
cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


######################################################################
## PROJECT DETAILS
project(picodaq)              # Project name
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O6")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O6")
pico_sdk_init()

add_executable(sigmadelta
    src/main.cpp
    src/reset.cpp
    src/ringbuffer.cpp
    src/core1.cpp
    src/usb.cpp
    src/led.cpp
    src/usb_descriptors.c
    )

pico_add_extra_outputs(sigmadelta)
target_link_libraries(sigmadelta pico_stdlib)
target_link_libraries(sigmadelta pico_multicore)
target_link_libraries(sigmadelta hardware_pwm)
target_link_libraries(sigmadelta hardware_dma)
target_link_libraries(sigmadelta hardware_irq)
target_link_libraries(sigmadelta tinyusb_device tinyusb_board)
target_include_directories(sigmadelta PUBLIC src)
pico_enable_stdio_usb(sigmadelta 0)
pico_enable_stdio_uart(sigmadelta 0)




