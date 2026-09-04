cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(PICO_SDK_PATH "/hdd/data3/microcontrollers/rp2040/mfti/module_1/pico-sdk")

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(121_blink_reg)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
)

target_link_libraries(${PROJECT_NAME}
   pico_stdlib
)

target_link_libraries(${PROJECT_NAME} pico_stdlib)

pico_set_linker_script(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld
)

pico_add_extra_outputs(${PROJECT_NAME})

