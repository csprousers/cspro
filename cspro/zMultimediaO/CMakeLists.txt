project(zMultimediaO)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/mp4v2/include
)

add_library(${PROJECT_NAME}
    BmpFile.cpp
    Image.cpp
    Mp4Reader.cpp
    Mp4Writer.cpp
    QRCode.cpp

    ${EXTERNAL_DIRECTORY}/qrcodegen/qrcodegen.cpp
)
