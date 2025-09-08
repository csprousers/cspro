project(zMultimediaO)

include(../WASM/Definitions.cmake)

set(GPAC_SRC_PATH ${EXTERNAL_DIRECTORY}/gpac/src)
set(LIBEXIF_SRC_PATH ${EXTERNAL_DIRECTORY}/libexif)
set(QRCODEGEN_SRC_PATH ${EXTERNAL_DIRECTORY}/qrcodegen)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/gpac/include
)

add_library(${PROJECT_NAME}
    BmpFile.cpp
    ExifReader.cpp
    Icon.cpp
    Image.cpp
    Mp4File.cpp
    QRCode.cpp

    ${GPAC_SRC_PATH}/isomedia/avc_ext.c
    ${GPAC_SRC_PATH}/isomedia/box_code_3gpp.c
    ${GPAC_SRC_PATH}/isomedia/box_code_apple.c
    ${GPAC_SRC_PATH}/isomedia/box_code_base.c
    ${GPAC_SRC_PATH}/isomedia/box_code_drm.c
    ${GPAC_SRC_PATH}/isomedia/box_code_meta.c
    ${GPAC_SRC_PATH}/isomedia/box_funcs.c
    ${GPAC_SRC_PATH}/isomedia/data_map.c
    ${GPAC_SRC_PATH}/isomedia/drm_sample.c
    ${GPAC_SRC_PATH}/isomedia/iff.c
    ${GPAC_SRC_PATH}/isomedia/isom_intern.c
    ${GPAC_SRC_PATH}/isomedia/isom_read.c
    ${GPAC_SRC_PATH}/isomedia/isom_store.c
    ${GPAC_SRC_PATH}/isomedia/isom_write.c
    ${GPAC_SRC_PATH}/isomedia/media.c
    ${GPAC_SRC_PATH}/isomedia/media_odf.c
    ${GPAC_SRC_PATH}/isomedia/meta.c
    ${GPAC_SRC_PATH}/isomedia/movie_fragments.c
    ${GPAC_SRC_PATH}/isomedia/sample_descs.c
    ${GPAC_SRC_PATH}/isomedia/stbl_read.c
    ${GPAC_SRC_PATH}/isomedia/stbl_write.c
    ${GPAC_SRC_PATH}/isomedia/track.c
    ${GPAC_SRC_PATH}/isomedia/tx3g.c
    ${GPAC_SRC_PATH}/media_tools/av_parsers.c
    ${GPAC_SRC_PATH}/media_tools/isom_tools.c
    ${GPAC_SRC_PATH}/odf/descriptors.c
    ${GPAC_SRC_PATH}/odf/desc_private.c
    ${GPAC_SRC_PATH}/odf/odf_code.c
    ${GPAC_SRC_PATH}/odf/odf_codec.c
    ${GPAC_SRC_PATH}/odf/odf_command.c
    ${GPAC_SRC_PATH}/odf/slc.c
    ${GPAC_SRC_PATH}/utils/alloc.c
    ${GPAC_SRC_PATH}/utils/base_encoding.c
    ${GPAC_SRC_PATH}/utils/bitstream.c
    ${GPAC_SRC_PATH}/utils/configfile.c
    ${GPAC_SRC_PATH}/utils/constants.c
    ${GPAC_SRC_PATH}/utils/error.c
    ${GPAC_SRC_PATH}/utils/list.c
    ${GPAC_SRC_PATH}/utils/module.c
    ${GPAC_SRC_PATH}/utils/os_config_init.c
    ${GPAC_SRC_PATH}/utils/os_divers.c
    ${GPAC_SRC_PATH}/utils/os_file.c
    ${GPAC_SRC_PATH}/utils/os_module.c
    ${GPAC_SRC_PATH}/utils/sha1.c
    ${GPAC_SRC_PATH}/utils/url.c
    ${GPAC_SRC_PATH}/utils/utf.c
    ${GPAC_SRC_PATH}/utils/xml_parser.c

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
