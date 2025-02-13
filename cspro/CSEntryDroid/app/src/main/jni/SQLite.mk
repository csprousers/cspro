LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= SQLite
SQLITE_SRC_PATH			:= ../../../../../SQLite

LOCAL_SRC_FILES			+= $(SQLITE_SRC_PATH)/../external/SQLite/sqlite3.c
LOCAL_SRC_FILES			+= $(SQLITE_SRC_PATH)/SQLiteHelpers.cpp

LOCAL_CFLAGS    		:= -DSQLITE_EXPORTS
LOCAL_CFLAGS    		+= -DANDROID=1
LOCAL_CFLAGS			+= -DSQLITE_TEMP_STORE=3 # use memory for temp files since Android doesn't have real temp directory

include $(BUILD_STATIC_LIBRARY)
