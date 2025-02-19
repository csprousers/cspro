LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zBridgeO
ZBRIDGEO_SRC_PATH       := ../../../../../zBridgeO

LOCAL_SRC_FILES         += $(ZBRIDGEO_SRC_PATH)/NPFF.cpp
LOCAL_SRC_FILES         += $(ZBRIDGEO_SRC_PATH)/RunApl.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DUSE_BINARY=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zAppO zDictO zFormO

include $(BUILD_STATIC_LIBRARY)
