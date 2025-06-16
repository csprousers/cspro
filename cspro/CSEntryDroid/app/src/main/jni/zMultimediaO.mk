LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMultimediaO
ZMULTIMEDIAO_SRC_PATH   := ../../../../../zMultimediaO
LIBEXIF_SRC_PATH        := ../../../../../external/libexif
QRCODEGEN_SRC_PATH      := ../../../../../external/qrcodegen

LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/BmpFile.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/ExifReader.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Icon.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Image.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Reader.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Writer.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/QRCode.cpp

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
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/mp4v2/include
LOCAL_STATIC_LIBRARIES  := zToolsO zUtilO mp4v2 zlib

include $(BUILD_STATIC_LIBRARY)
