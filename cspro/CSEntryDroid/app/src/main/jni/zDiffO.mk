LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zDiffO
ZDIFFO_SRC_PATH         := ../../../../../zDiffO

LOCAL_SRC_FILES         += $(ZDIFFO_SRC_PATH)/Differ.cpp
LOCAL_SRC_FILES         += $(ZDIFFO_SRC_PATH)/DiffSpec.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zAppO zJson zUtilF zDictO zCaseO zDataO

include $(BUILD_STATIC_LIBRARY)
