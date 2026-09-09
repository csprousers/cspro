project(zMultimediaO)

set(QRCODEGEN_DIR ${EXTERNAL_DIR}/qrcodegen)

add_library(${PROJECT_NAME}
    BmpFile.cpp
    ExifReader.cpp
    Icon.cpp
    Image.cpp
    Mp4File.cpp
    QRCode.cpp
    WebMFile.cpp

    ${QRCODEGEN_DIR}/qrcodegen.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${THIRD_PARTY_INCLUDE_DIR}/webm
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

set_source_files_properties(
    ${QRCODEGEN_DIR}/qrcodegen.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # gpac
    ${THIRD_PARTY_LIB_DIR}/libgpac${THIRD_PARTY_LIB_EXT}

    # libexif
    ${THIRD_PARTY_LIB_DIR}/libexif${THIRD_PARTY_LIB_EXT}

    # libwebm
    ${THIRD_PARTY_LIB_DIR}/libwebm${THIRD_PARTY_LIB_EXT}

    # libwebp
    ${THIRD_PARTY_LIB_DIR}/libsharpyuv${THIRD_PARTY_LIB_EXT}
    ${THIRD_PARTY_LIB_DIR}/libwebp${THIRD_PARTY_LIB_EXT}

    # zlib
    ${THIRD_PARTY_LIB_DIR}/libz${THIRD_PARTY_LIB_EXT}
)
