project(rtf2html_dll)

include(../WASM/Definitions.cmake)

add_compile_definitions(RTF2HTML_DLL_EXPORTS)

add_library(${PROJECT_NAME}
    fmt_opts.cpp
    rtf_keyword.cpp
    rtf_table.cpp
    rtf2html.cpp
)
