add_library(systime STATIC
    src/systime.c
)

target_include_directories(systime PUBLIC
    include
)

target_link_libraries(systime PRIVATE
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(systime PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)
