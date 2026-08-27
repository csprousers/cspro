project(zHtml)

set(GUMBO_DIRECTORY ${EXTERNAL_DIRECTORY}/gumbo)

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

if(WIN32)
    # for Gumbo's dummy strings.h
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GUMBO_DIRECTORY}
    )
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
