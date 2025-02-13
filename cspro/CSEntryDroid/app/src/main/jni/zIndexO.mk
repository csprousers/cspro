LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zIndexO
ZINDEXOO_SRC_PATH       := ../../../../../zIndexO

LOCAL_SRC_FILES         += $(ZINDEXOO_SRC_PATH)/Indexer.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO SQLite zToolsO zUtilO zAppO zUtilF zDictO zCaseO zDataO zListingO

include $(BUILD_STATIC_LIBRARY)
