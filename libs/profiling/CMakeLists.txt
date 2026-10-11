add_library(profiling STATIC
    src/profiling.c
)

target_include_directories(profiling PUBLIC
    include
)

target_link_libraries(profiling PRIVATE
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(profiling PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)
