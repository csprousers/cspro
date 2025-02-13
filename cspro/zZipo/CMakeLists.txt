project(zZipO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME} 
    IZip.cpp
    miniz_inclusion.cpp
    ZipUtility.cpp
    ZipZL.cpp
)
