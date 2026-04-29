include_guard(GLOBAL)

add_library(snemu_compile_opts INTERFACE)

target_compile_definitions(snemu_compile_opts INTERFACE _GNU_SOURCE)

target_compile_options(snemu_compile_opts INTERFACE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wformat=2
            -Wstrict-prototypes
            -Wold-style-definition
            -Wmissing-prototypes
            -Wmissing-declarations
            -Wnull-dereference
            -Wdouble-promotion
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wsign-conversion
            -Wno-unused-parameter>
        $<$<C_COMPILER_ID:MSVC>:/W4 /permissive->)
