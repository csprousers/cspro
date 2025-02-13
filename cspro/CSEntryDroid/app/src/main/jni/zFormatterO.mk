LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zFormatterO
ZFORMATTERO_SRC_PATH    := ../../../../../zFormatterO

LOCAL_SRC_FILES         += $(ZFORMATTERO_SRC_PATH)/QuestionnaireContentCreator.cpp
LOCAL_SRC_FILES         += $(ZFORMATTERO_SRC_PATH)/QuestionnaireViewer.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY=1
LOCAL_CFLAGS            += -DUSE_BINARY=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zHtml zAppO zDictO zFormO Engine zCaseO

include $(BUILD_STATIC_LIBRARY)
