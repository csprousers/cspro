project(zMultimediaO)

set(GPAC_DIR ${EXTERNAL_DIR}/gpac)
set(GPAC_SRC_DIR ${GPAC_DIR}/src)
set(LIBEXIF_DIR ${EXTERNAL_DIR}/libexif)
set(QRCODEGEN_DIR ${EXTERNAL_DIR}/qrcodegen)

add_library(${PROJECT_NAME}
    BmpFile.cpp
    ExifReader.cpp
    Icon.cpp
    Image.cpp
    Mp4File.cpp
    QRCode.cpp
    WebMFile.cpp

    ${GPAC_SRC_DIR}/isomedia/avc_ext.c
    ${GPAC_SRC_DIR}/isomedia/box_code_3gpp.c
    ${GPAC_SRC_DIR}/isomedia/box_code_apple.c
    ${GPAC_SRC_DIR}/isomedia/box_code_base.c
    ${GPAC_SRC_DIR}/isomedia/box_code_drm.c
    ${GPAC_SRC_DIR}/isomedia/box_code_meta.c
    ${GPAC_SRC_DIR}/isomedia/box_funcs.c
    ${GPAC_SRC_DIR}/isomedia/data_map.c
    ${GPAC_SRC_DIR}/isomedia/drm_sample.c
    ${GPAC_SRC_DIR}/isomedia/iff.c
    ${GPAC_SRC_DIR}/isomedia/isom_intern.c
    ${GPAC_SRC_DIR}/isomedia/isom_read.c
    ${GPAC_SRC_DIR}/isomedia/isom_store.c
    ${GPAC_SRC_DIR}/isomedia/isom_write.c
    ${GPAC_SRC_DIR}/isomedia/media.c
    ${GPAC_SRC_DIR}/isomedia/media_odf.c
    ${GPAC_SRC_DIR}/isomedia/meta.c
    ${GPAC_SRC_DIR}/isomedia/movie_fragments.c
    ${GPAC_SRC_DIR}/isomedia/sample_descs.c
    ${GPAC_SRC_DIR}/isomedia/stbl_read.c
    ${GPAC_SRC_DIR}/isomedia/stbl_write.c
    ${GPAC_SRC_DIR}/isomedia/track.c
    ${GPAC_SRC_DIR}/isomedia/tx3g.c
    ${GPAC_SRC_DIR}/media_tools/av_parsers.c
    ${GPAC_SRC_DIR}/media_tools/isom_tools.c
    ${GPAC_SRC_DIR}/odf/descriptors.c
    ${GPAC_SRC_DIR}/odf/desc_private.c
    ${GPAC_SRC_DIR}/odf/odf_code.c
    ${GPAC_SRC_DIR}/odf/odf_codec.c
    ${GPAC_SRC_DIR}/odf/odf_command.c
    ${GPAC_SRC_DIR}/odf/slc.c
    ${GPAC_SRC_DIR}/utils/alloc.c
    ${GPAC_SRC_DIR}/utils/base_encoding.c
    ${GPAC_SRC_DIR}/utils/bitstream.c
    ${GPAC_SRC_DIR}/utils/configfile.c
    ${GPAC_SRC_DIR}/utils/constants.c
    ${GPAC_SRC_DIR}/utils/error.c
    ${GPAC_SRC_DIR}/utils/list.c
    ${GPAC_SRC_DIR}/utils/module.c
    ${GPAC_SRC_DIR}/utils/os_config_init.c
    ${GPAC_SRC_DIR}/utils/os_divers.c
    ${GPAC_SRC_DIR}/utils/os_file.c
    ${GPAC_SRC_DIR}/utils/os_module.c
    ${GPAC_SRC_DIR}/utils/sha1.c
    ${GPAC_SRC_DIR}/utils/url.c
    ${GPAC_SRC_DIR}/utils/utf.c
    ${GPAC_SRC_DIR}/utils/xml_parser.c

    ${LIBEXIF_DIR}/exif-byte-order.c
    ${LIBEXIF_DIR}/exif-content.c
    ${LIBEXIF_DIR}/exif-data.c
    ${LIBEXIF_DIR}/exif-entry.c
    ${LIBEXIF_DIR}/exif-format.c
    ${LIBEXIF_DIR}/exif-gps-ifd.c
    ${LIBEXIF_DIR}/exif-ifd.c
    ${LIBEXIF_DIR}/exif-loader.c
    ${LIBEXIF_DIR}/exif-log.c
    ${LIBEXIF_DIR}/exif-mem.c
    ${LIBEXIF_DIR}/exif-mnote-data.c
    ${LIBEXIF_DIR}/exif-tag.c
    ${LIBEXIF_DIR}/exif-utils.c
    ${LIBEXIF_DIR}/canon/exif-mnote-data-canon.c
    ${LIBEXIF_DIR}/canon/mnote-canon-entry.c
    ${LIBEXIF_DIR}/canon/mnote-canon-tag.c
    ${LIBEXIF_DIR}/fuji/exif-mnote-data-fuji.c
    ${LIBEXIF_DIR}/fuji/mnote-fuji-entry.c
    ${LIBEXIF_DIR}/fuji/mnote-fuji-tag.c
    ${LIBEXIF_DIR}/olympus/exif-mnote-data-olympus.c
    ${LIBEXIF_DIR}/olympus/mnote-olympus-entry.c
    ${LIBEXIF_DIR}/olympus/mnote-olympus-tag.c
    ${LIBEXIF_DIR}/pentax/exif-mnote-data-pentax.c
    ${LIBEXIF_DIR}/pentax/mnote-pentax-entry.c
    ${LIBEXIF_DIR}/pentax/mnote-pentax-tag.c

    ${QRCODEGEN_DIR}/qrcodegen.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIR}
    ${GPAC_DIR}/include
    ${THIRD_PARTY_INCLUDE_DIR}/webm
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)

set_source_files_properties(
    ${QRCODEGEN_DIR}/qrcodegen.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # libwebm
    ${THIRD_PARTY_LIB_DIR}/libwebm${THIRD_PARTY_LIB_EXT}

    # libwebp
    ${THIRD_PARTY_LIB_DIR}/libsharpyuv${THIRD_PARTY_LIB_EXT}
    ${THIRD_PARTY_LIB_DIR}/libwebp${THIRD_PARTY_LIB_EXT}
)
