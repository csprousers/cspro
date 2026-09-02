project(zHtml)

set(GUMBO_DIR ${EXTERNAL_DIR}/gumbo)

add_library(${PROJECT_NAME}
    AccessUrlSerializer.cpp
    CSHtmlDlgRunner.cpp
    CssProvider.cpp
    FileSystemVirtualFileMappingHandler.cpp
    HtmlDlgBaseRunner.cpp
    HtmlishSanitizer.cpp
    HtmlTextConverter.cpp
    LocalhostUrl.cpp
    PortableLocalFileServer.cpp
    PortableLocalhost.cpp
    PortableSimpleServer.cpp
    Pre77ReportGumboAccessor.cpp
    VirtualFileMappingHandlers.cpp
    WebViewSyncOperationMarker.cpp

    ${GUMBO_DIR}/attribute.c
    ${GUMBO_DIR}/char_ref.c
    ${GUMBO_DIR}/error.c
    ${GUMBO_DIR}/parser.c
    ${GUMBO_DIR}/string_buffer.c
    ${GUMBO_DIR}/string_piece.c
    ${GUMBO_DIR}/tag.c
    ${GUMBO_DIR}/tokenizer.c
    ${GUMBO_DIR}/utf8.c
    ${GUMBO_DIR}/util.c
    ${GUMBO_DIR}/vector.c
)

if(WIN32)
    # for Gumbo's dummy strings.h
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GUMBO_DIR}
    )
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # md4c
    ${THIRD_PARTY_LIB_DIR}/libmd4c${THIRD_PARTY_LIB_EXT}
)
