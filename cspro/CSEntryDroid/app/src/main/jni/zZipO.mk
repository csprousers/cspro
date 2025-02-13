LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zZipO
ZZIPO_SRC_PATH          := ../../../../../zZipO

LOCAL_SRC_FILES         := $(ZZIPO_SRC_PATH)/IZip.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/miniz_inclusion.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/ZipUtility.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/ZipZL.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO

include $(BUILD_STATIC_LIBRARY)
