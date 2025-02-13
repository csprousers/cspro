LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zAction
ZACTION_SRC_PATH        := ../../../../../zAction

LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/ActionFunctionMapping.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/ActionInvoker.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Application.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Caller.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Clipboard.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Data.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Dictionary.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Encoding.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/File.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Hash.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/JsonExecutor.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Localhost.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Logic.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Message.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Path.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Settings.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/Sqlite.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/System.cpp
LOCAL_SRC_FILES         += $(ZACTION_SRC_PATH)/UserInterface.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO SQLite zToolsO zJson zUtilO zHtml zMessageO zUtilF zDictO zFormO zAppO zParadataO zDataO zLogicO

include $(BUILD_STATIC_LIBRARY)
