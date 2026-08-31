project(zMarkdown)

set(MD4C_DIR ${EXTERNAL_DIR}/md4c)

add_library(${PROJECT_NAME}
    HtmlParserCallback.cpp
    Markdown.cpp

    ${MD4C_DIR}/md4c.c
    ${MD4C_DIR}/md4c-html.c
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h>"
)
