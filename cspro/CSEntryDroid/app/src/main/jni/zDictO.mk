LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zDictO
ZDICTO_SRC_PATH         := ../../../../../zDictO

LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/CaptureInfo.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DDClass.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/Definitions.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictBase.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictionaryComparer.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictionaryValidator.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictItem.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictLevel.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictNamedBase.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictRecord.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictRelation.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictValue.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictValuePair.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/DictValueSet.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/ItemIndexHelper.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/Pre80SpecFile.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/ValueProcessor.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/ValueSetFixer.cpp
LOCAL_SRC_FILES         += $(ZDICTO_SRC_PATH)/ValueSetResponse.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY
LOCAL_CFLAGS            += -DUSE_BINARY
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zAppO

include $(BUILD_STATIC_LIBRARY)
