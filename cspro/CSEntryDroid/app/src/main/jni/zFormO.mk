LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zFormO
ZFORMO_SRC_PATH         := ../../../../../zFormO

LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Block.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Box.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Definitions.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/DragOptions.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Field.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/FieldColors.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Form.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/FormBase.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/FormFile.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Group.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/ItemBase.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Level.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/RepeatedItemSerializer.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Roster.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/RosterCells.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/RosterColumn.cpp
LOCAL_SRC_FILES         += $(ZFORMO_SRC_PATH)/Text.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY=1
LOCAL_CFLAGS            += -DUSE_BINARY=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zUtilF zJson zAppO zDictO

include $(BUILD_STATIC_LIBRARY)
