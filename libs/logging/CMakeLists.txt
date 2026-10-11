add_library(logging STATIC
    src/logging.c
)

target_include_directories(logging PUBLIC
    include
)

target_link_libraries(logging PRIVATE
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(logging PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)
