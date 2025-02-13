LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zParadataO
ZPARADATAO_SRC_PATH     := ../../../../../zParadataO

LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/ApplicationEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/CaseEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/Concatenator.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/DataRepositoryEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/DeviceStateEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/ExternalApplicationEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/FieldEntryEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/FieldInfo.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/FieldMovementEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/FieldValidationEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/GpsEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/GuiConcatenator.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/ImputeEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/KeyingInstance.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/LanguageChangeEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/Log.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/Logger.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/MessageEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/NoteEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/OperatorSelectionEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/PropertyEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/SessionEvent.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/Syncer.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/Table.cpp
LOCAL_SRC_FILES         += $(ZPARADATAO_SRC_PATH)/TableDefinitions.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zAppO zUtilF zUtilO zToolsO zPlatformO SQLite

include $(BUILD_STATIC_LIBRARY)
