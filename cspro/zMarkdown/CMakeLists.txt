project(zMarkdown)

add_library(${PROJECT_NAME}
    HtmlParserCallback.cpp
    Markdown.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # md4c
    ${THIRD_PARTY_LIB_DIR}/libmd4c${THIRD_PARTY_LIB_EXT}
)
