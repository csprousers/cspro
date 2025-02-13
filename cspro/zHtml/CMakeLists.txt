project(zHtml)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    AccessUrlSerializer.cpp
    CSHtmlDlgRunner.cpp
    FileSystemVirtualFileMappingHandler.cpp
    HtmlDlgBaseRunner.cpp
    LocalhostUrl.cpp
    VirtualFileMappingHandlers.cpp
    WebViewSyncOperationMarker.cpp
)
