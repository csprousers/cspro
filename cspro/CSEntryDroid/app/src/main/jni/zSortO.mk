LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zSortO
ZSORTO_SRC_PATH         := ../../../../../zSortO

LOCAL_SRC_FILES         += $(ZSORTO_SRC_PATH)/SortableKeyDatabase.cpp
LOCAL_SRC_FILES         += $(ZSORTO_SRC_PATH)/Sorter.cpp
LOCAL_SRC_FILES         += $(ZSORTO_SRC_PATH)/SortSpec.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO SQLite zToolsO zUtilO zAppO zJson zUtilF zDictO zCaseO zDataO

include $(BUILD_STATIC_LIBRARY)
