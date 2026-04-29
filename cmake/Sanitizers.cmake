include_guard(GLOBAL)

option(SNEMU_ASAN  "Enable AddressSanitizer"           OFF)
option(SNEMU_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

add_library(snemu_sanitizers INTERFACE)

set(_snemu_san_configs "Debug,RelWithDebInfo")

target_compile_options(snemu_sanitizers INTERFACE
        $<$<AND:$<BOOL:${SNEMU_ASAN}>,$<CONFIG:${_snemu_san_configs}>>:
            -fsanitize=address -fno-omit-frame-pointer>
        $<$<AND:$<BOOL:${SNEMU_UBSAN}>,$<CONFIG:${_snemu_san_configs}>>:
            -fsanitize=undefined -fno-sanitize-recover=undefined>)

target_link_options(snemu_sanitizers INTERFACE
        $<$<AND:$<BOOL:${SNEMU_ASAN}>,$<CONFIG:${_snemu_san_configs}>>:-fsanitize=address>
        $<$<AND:$<BOOL:${SNEMU_UBSAN}>,$<CONFIG:${_snemu_san_configs}>>:-fsanitize=undefined>)
