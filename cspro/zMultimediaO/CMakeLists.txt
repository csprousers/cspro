project(zMultimediaO)

include(../WASM/Definitions.cmake)

set(LIBEXIF_SRC_PATH ${EXTERNAL_DIRECTORY}/libexif)
set(QRCODEGEN_SRC_PATH ${EXTERNAL_DIRECTORY}/qrcodegen)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/mp4v2/include
)

add_library(${PROJECT_NAME}
    BmpFile.cpp
    ExifReader.cpp
    Icon.cpp
    Image.cpp
    Mp4Reader.cpp
    Mp4Writer.cpp
    QRCode.cpp

    ${LIBEXIF_SRC_PATH}/exif-byte-order.c
    ${LIBEXIF_SRC_PATH}/exif-content.c
    ${LIBEXIF_SRC_PATH}/exif-data.c
    ${LIBEXIF_SRC_PATH}/exif-entry.c
    ${LIBEXIF_SRC_PATH}/exif-format.c
    ${LIBEXIF_SRC_PATH}/exif-gps-ifd.c
    ${LIBEXIF_SRC_PATH}/exif-ifd.c
    ${LIBEXIF_SRC_PATH}/exif-loader.c
    ${LIBEXIF_SRC_PATH}/exif-log.c
    ${LIBEXIF_SRC_PATH}/exif-mem.c
    ${LIBEXIF_SRC_PATH}/exif-mnote-data.c
    ${LIBEXIF_SRC_PATH}/exif-tag.c
    ${LIBEXIF_SRC_PATH}/exif-utils.c
    ${LIBEXIF_SRC_PATH}/canon/exif-mnote-data-canon.c
    ${LIBEXIF_SRC_PATH}/canon/mnote-canon-entry.c
    ${LIBEXIF_SRC_PATH}/canon/mnote-canon-tag.c
    ${LIBEXIF_SRC_PATH}/fuji/exif-mnote-data-fuji.c
    ${LIBEXIF_SRC_PATH}/fuji/mnote-fuji-entry.c
    ${LIBEXIF_SRC_PATH}/fuji/mnote-fuji-tag.c
    ${LIBEXIF_SRC_PATH}/olympus/exif-mnote-data-olympus.c
    ${LIBEXIF_SRC_PATH}/olympus/mnote-olympus-entry.c
    ${LIBEXIF_SRC_PATH}/olympus/mnote-olympus-tag.c
    ${LIBEXIF_SRC_PATH}/pentax/exif-mnote-data-pentax.c
    ${LIBEXIF_SRC_PATH}/pentax/mnote-pentax-entry.c
    ${LIBEXIF_SRC_PATH}/pentax/mnote-pentax-tag.c

    ${QRCODEGEN_SRC_PATH}/qrcodegen.cpp
)
