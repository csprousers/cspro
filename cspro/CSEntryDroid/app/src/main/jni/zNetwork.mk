LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zNetwork
ZNETWORK_SRC_PATH       := ../../../../../zNetwork

LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/FileInfo.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/HeaderList.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO

include $(BUILD_STATIC_LIBRARY)
