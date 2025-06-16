LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zZipO
ZZIPO_SRC_PATH          := ../../../../../zZipO

LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/ZipException.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/ZipFile.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/ZLib.cpp
LOCAL_SRC_FILES         += $(ZZIPO_SRC_PATH)/../external/miniz/miniz.c

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO

include $(BUILD_STATIC_LIBRARY)
