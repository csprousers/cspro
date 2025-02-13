LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mp4v2
LOCAL_SRC_FILES := $(LOCAL_PATH)/external/$(TARGET_ARCH_ABI)/libmp4v2.a
include $(PREBUILT_STATIC_LIBRARY)

