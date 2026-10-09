cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# set(PICO_SDK_PATH "/hdd/data3/microcontrollers/rp2040/mfti/module_1/pico-sdk")

set(PICO_BOARD pico_16m)
set(PICO_PLATFORM rp2040)
set(PICO_FLASH_SPI_CLKDIV 2)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led/led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/device/device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling/profiling.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol/stdio-text-protocol.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_unique_id
    pico_bootsel_via_double_reset
    pico_multicore
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/led
    ${CMAKE_CURRENT_SOURCE_DIR}/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/device
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_SDK_VERSION="${PICO_SDK_VERSION_STRING}"
)


pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

