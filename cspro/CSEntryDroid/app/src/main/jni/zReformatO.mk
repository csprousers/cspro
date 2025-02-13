LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zReformatO
ZREFORMATO_SRC_PATH     := ../../../../../zReformatO

LOCAL_SRC_FILES         += $(ZREFORMATO_SRC_PATH)/Reformatter.cpp
LOCAL_SRC_FILES         += $(ZREFORMATO_SRC_PATH)/ToolReformatter.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zAppO zUtilF zDictO zCaseO zDataO

include $(BUILD_STATIC_LIBRARY)
