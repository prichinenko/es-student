cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

add_compile_options(-Wall)

# set(PICO_SDK_PATH "/hdd/data3/microcontrollers/rp2040/mfti/module_1/pico-sdk")

set(PICO_BOARD pico_16m)
set(PICO_PLATFORM rp2040)
set(PICO_FLASH_SPI_CLKDIV 2)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# железо прибора: единственное место проекта с SDK
add_library(platform STATIC
    platform/platform.c
)

target_include_directories(platform
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

target_link_libraries(platform
    PRIVATE pico_stdlib
)

add_library(logic STATIC
    logic/api/api.c
    logic/api/api-callbacks.c
    logic/led-task/led-task.c
    logic/button-task/button-task.c
    logic/pi-task/pi-task.c
)

target_include_directories(logic
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/logic
)

target_link_libraries(logic
    PUBLIC  stdio_text_protocol
    PRIVATE platform
    PRIVATE systime
    PRIVATE device
    PRIVATE logging
    PRIVATE profiling
)


add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/led/led.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/api/api.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/api/api-callbacks.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/button-task/button-task.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/pi-task/pi-task.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/platform/platform.c
)


target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    # pico_unique_id
    pico_bootsel_via_double_reset
    pico_multicore
    platform
    logic
    device
    logging
    profiling
    stdio_text_protocol
    systime
)

# выключатель профилирования: 0 — вызовов секундомеров в образе нет
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # ${CMAKE_CURRENT_SOURCE_DIR}/led
    # ${CMAKE_CURRENT_SOURCE_DIR}/api
    # ${CMAKE_CURRENT_SOURCE_DIR}/led-task
    # ${CMAKE_CURRENT_SOURCE_DIR}/button-task
    # ${CMAKE_CURRENT_SOURCE_DIR}/pi-task
    # ${CMAKE_CURRENT_SOURCE_DIR}/platform
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

