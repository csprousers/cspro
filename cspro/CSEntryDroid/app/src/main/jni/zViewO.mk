LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zViewO
ZVIEWO_SRC_PATH         := ../../../../../zViewO

LOCAL_SRC_FILES         += $(ZVIEWO_SRC_PATH)/MarkdownViewInput.cpp
LOCAL_SRC_FILES         += $(ZVIEWO_SRC_PATH)/ViewInput.cpp
LOCAL_SRC_FILES         += $(ZVIEWO_SRC_PATH)/ViewInputCreator.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
