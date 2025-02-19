LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zXml
ZXML_SRC_PATH           := ../../../../../zXml

LOCAL_SRC_FILES         += $(ZXML_SRC_PATH)/SimpleXml.cpp
LOCAL_SRC_FILES         += $(ZXML_SRC_PATH)/../external/pugixml/pugixml.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
