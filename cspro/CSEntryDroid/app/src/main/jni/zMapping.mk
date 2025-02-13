LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMapping
ZMAPPING_SRC_PATH       := ../../../../../zMapping

LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/CoordinateConverter.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/GeoJson.cpp


LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/geometry.hpp/include
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/variant/include
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/jsoncons

include $(BUILD_STATIC_LIBRARY)
