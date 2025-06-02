LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMapping
SOLUTION_SRC_PATH       := $(JNI_PATH)/../../../../..
ZMAPPING_SRC_PATH       := $(SOLUTION_SRC_PATH)/zMapping

LOCAL_C_INCLUDES        += $(SOLUTION_SRC_PATH)/external
LOCAL_C_INCLUDES        += $(SOLUTION_SRC_PATH)/external/geometry.hpp/include
LOCAL_C_INCLUDES        += $(SOLUTION_SRC_PATH)/external/variant/include

LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/CoordinateConverter.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/GeoJson.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/HtmlMapUI.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/MBTilesReader.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/OfflineTileProvider.cpp
LOCAL_SRC_FILES         += $(ZMAPPING_SRC_PATH)/TPKReader.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
