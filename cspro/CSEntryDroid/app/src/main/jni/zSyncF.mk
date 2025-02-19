LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zSyncF
ZSYNCF_SRC_PATH         := ../../../../../zSyncF

LOCAL_SRC_FILES         += $(ZSYNCF_SRC_PATH)/DialogBasedSyncListener.cpp
LOCAL_SRC_FILES         += $(ZSYNCF_SRC_PATH)/SyncLoginAccessor.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zSyncO

include $(BUILD_STATIC_LIBRARY)
