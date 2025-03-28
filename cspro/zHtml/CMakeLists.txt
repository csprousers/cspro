project(zHtml)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    AccessUrlSerializer.cpp
    CSHtmlDlgRunner.cpp
    CssProvider.cpp
    FileSystemVirtualFileMappingHandler.cpp
    HtmlDlgBaseRunner.cpp
    LocalhostUrl.cpp
    PortableLocalFileServer.cpp
    PortableLocalhost.cpp
    VirtualFileMappingHandlers.cpp
    WebViewSyncOperationMarker.cpp
)
