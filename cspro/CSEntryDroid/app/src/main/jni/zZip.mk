LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zZip
SOLUTION_SRC_PATH       := $(JNI_PATH)/../../../../..
ZZIP_SRC_PATH           := $(SOLUTION_SRC_PATH)/zZip
MINIZ_SRC_PATH          := $(SOLUTION_SRC_PATH)/external/miniz

LOCAL_SRC_FILES         += $(ZZIP_SRC_PATH)/ZipException.cpp
LOCAL_SRC_FILES         += $(ZZIP_SRC_PATH)/ZipFile.cpp
LOCAL_SRC_FILES         += $(ZZIP_SRC_PATH)/ZLib.cpp

LOCAL_SRC_FILES         += $(MINIZ_SRC_PATH)/miniz.c

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO

include $(BUILD_STATIC_LIBRARY)
