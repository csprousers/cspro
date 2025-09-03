LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMultimediaO
ZMULTIMEDIAO_SRC_PATH   := ../../../../../zMultimediaO
GPAC_SRC_PATH           := ../../../../../external/gpac/src
LIBEXIF_SRC_PATH        := ../../../../../external/libexif
QRCODEGEN_SRC_PATH      := ../../../../../external/qrcodegen

LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/BmpFile.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/ExifReader.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Icon.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Image.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Accessor.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Reader.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Writer.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/QRCode.cpp

LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/avc_ext.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_code_3gpp.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_code_apple.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_code_base.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_code_drm.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_code_meta.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/box_funcs.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/data_map.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/drm_sample.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/iff.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/isom_intern.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/isom_read.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/isom_store.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/isom_write.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/media.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/media_odf.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/meta.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/movie_fragments.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/sample_descs.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/stbl_read.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/stbl_write.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/track.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/isomedia/tx3g.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/media_tools/av_parsers.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/media_tools/isom_tools.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/descriptors.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/desc_private.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/odf_code.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/odf_codec.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/odf_command.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/odf/slc.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/alloc.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/base_encoding.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/bitstream.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/configfile.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/constants.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/error.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/list.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/module.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/os_config_init.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/os_divers.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/os_file.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/os_module.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/sha1.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/url.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/utf.c
LOCAL_SRC_FILES         += $(GPAC_SRC_PATH)/utils/xml_parser.c

LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-byte-order.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-content.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-data.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-entry.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-format.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-gps-ifd.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-ifd.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-loader.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-log.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-mem.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-mnote-data.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-tag.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/exif-utils.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/canon/exif-mnote-data-canon.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/canon/mnote-canon-entry.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/canon/mnote-canon-tag.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/fuji/exif-mnote-data-fuji.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/fuji/mnote-fuji-entry.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/fuji/mnote-fuji-tag.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/olympus/exif-mnote-data-olympus.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/olympus/mnote-olympus-entry.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/olympus/mnote-olympus-tag.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/pentax/exif-mnote-data-pentax.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/pentax/mnote-pentax-entry.c
LOCAL_SRC_FILES         += $(LIBEXIF_SRC_PATH)/pentax/mnote-pentax-tag.c

LOCAL_SRC_FILES         += $(QRCODEGEN_SRC_PATH)/qrcodegen.cpp


include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/gpac/include
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/mp4v2/include
LOCAL_STATIC_LIBRARIES  := zToolsO zUtilO mp4v2 zlib

include $(BUILD_STATIC_LIBRARY)
