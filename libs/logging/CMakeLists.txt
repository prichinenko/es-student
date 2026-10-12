add_library(logging STATIC
    src/log.c
)

target_include_directories(logging PUBLIC
    include
)

target_compile_definitions(logging PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)
