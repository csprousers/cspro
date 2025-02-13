LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zPackO
ZPACKO_SRC_PATH         := ../../../../../zPackO

LOCAL_SRC_FILES         += $(ZPACKO_SRC_PATH)/PackEntry.cpp
LOCAL_SRC_FILES         += $(ZPACKO_SRC_PATH)/Packer.cpp
LOCAL_SRC_FILES         += $(ZPACKO_SRC_PATH)/PackSpec.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zAppO zJson zZipO zAppO zDictO zFormO zDataO

include $(BUILD_STATIC_LIBRARY)
