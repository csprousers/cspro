LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zJson
ZJSON_SRC_PATH          := ../../../../../zJson

LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/IncrementalJsonObjectArrayParser.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/Json.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonFormattingOptions.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonNode.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonObjectCreator.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonSpecFile.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonStream.cpp
LOCAL_SRC_FILES         += $(ZJSON_SRC_PATH)/JsonWriter.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO

include $(BUILD_STATIC_LIBRARY)
