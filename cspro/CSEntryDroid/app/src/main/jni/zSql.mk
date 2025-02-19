LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zSql
ZSQL_SRC_PATH           := ../../../../../zSql
SQLITE_SRC_PATH         := ../../../../../external/SQLite

LOCAL_SRC_FILES         += $(ZSQL_SRC_PATH)/DB.cpp
LOCAL_SRC_FILES         += $(ZSQL_SRC_PATH)/Exception.cpp
LOCAL_SRC_FILES         += $(ZSQL_SRC_PATH)/SQLiteHelpers.cpp
LOCAL_SRC_FILES         += $(ZSQL_SRC_PATH)/Statement.cpp
LOCAL_SRC_FILES         += $(ZSQL_SRC_PATH)/Transaction.cpp
LOCAL_SRC_FILES         += $(SQLITE_SRC_PATH)/sqlite3.c

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DZSQL_EXPORTS
LOCAL_CFLAGS            += -DSQLITE_TEMP_STORE=3 # use memory for temp files since Android doesn't have real temp directory
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO

include $(BUILD_STATIC_LIBRARY)
