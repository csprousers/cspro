LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zlib
ZLIB_SRC_PATH	        := ../../../../../external/zlib

LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/adler32.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/compress.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/crc32.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/deflate.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/gzclose.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/gzlib.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/gzread.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/gzwrite.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/infback.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/inffast.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/inflate.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/inftrees.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/trees.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/uncompr.c
LOCAL_SRC_FILES         += $(ZLIB_SRC_PATH)/zutil.c

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
