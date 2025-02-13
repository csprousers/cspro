project(zReportO)

include(../WASM/Definitions.cmake)

set(GUMBO_DIRECTORY ${EXTERNAL_DIRECTORY}/gumbo)

add_library(${PROJECT_NAME}
    Pre77Report.cpp
    Pre77ReportManager.cpp
    Pre77ReportNodes.cpp

    ${GUMBO_DIRECTORY}/attribute.c
    ${GUMBO_DIRECTORY}/char_ref.c
    ${GUMBO_DIRECTORY}/error.c
    ${GUMBO_DIRECTORY}/parser.c
    ${GUMBO_DIRECTORY}/string_buffer.c
    ${GUMBO_DIRECTORY}/string_piece.c
    ${GUMBO_DIRECTORY}/tag.c
    ${GUMBO_DIRECTORY}/tokenizer.c
    ${GUMBO_DIRECTORY}/utf8.c
    ${GUMBO_DIRECTORY}/util.c
    ${GUMBO_DIRECTORY}/vector.c
)
