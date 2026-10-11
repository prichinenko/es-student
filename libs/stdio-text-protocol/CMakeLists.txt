add_library(stdio_text_protocol STATIC
    src/stdio-text-protocol.c
)

target_include_directories(stdio_text_protocol PUBLIC
    include
)

target_link_libraries(stdio_text_protocol PRIVATE
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(stdio_text_protocol PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)
