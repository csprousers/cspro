LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zAppO
ZAPPO_SRC_PATH          := ../../../../../zAppO

LOCAL_SRC_FILES         := $(ZAPPO_SRC_PATH)/AppFileType.cpp
LOCAL_SRC_FILES         := $(ZAPPO_SRC_PATH)/Application.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/AppSyncParameters.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/CodeFile.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/DictionaryDescription.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/EnumJsonSerializers.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/LabelSet.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Language.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/LogicSettings.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/MappingDefines.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/OccurrenceLabels.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/PFF.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Pre80SpecFile.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Properties/ApplicationProperties.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Properties/JsonProperties.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Properties/MappingProperties.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Properties/MappingTileProviderProperties.cpp
LOCAL_SRC_FILES         += $(ZAPPO_SRC_PATH)/Properties/ParadataProperties.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../zAppO
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson

include $(BUILD_STATIC_LIBRARY)
