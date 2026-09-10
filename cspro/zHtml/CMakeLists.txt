project(zHtml)

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
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # gumbo-parser
    ${THIRD_PARTY_PREBUILT_LIB_DIR}/libgumbo-parser${THIRD_PARTY_LIB_EXT}

    # md4c
    ${THIRD_PARTY_PREBUILT_LIB_DIR}/libmd4c${THIRD_PARTY_LIB_EXT}
)
