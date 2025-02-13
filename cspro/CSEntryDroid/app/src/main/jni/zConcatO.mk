LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zConcatO
ZCONCAT_SRC_PATH        := ../../../../../zConcatO

LOCAL_SRC_FILES         += $(ZCONCAT_SRC_PATH)/CaseConcatenator.cpp
LOCAL_SRC_FILES         += $(ZCONCAT_SRC_PATH)/Concatenator.cpp
LOCAL_SRC_FILES         += $(ZCONCAT_SRC_PATH)/CSConcatReporter.cpp
LOCAL_SRC_FILES         += $(ZCONCAT_SRC_PATH)/DuplicateCaseChecker.cpp
LOCAL_SRC_FILES         += $(ZCONCAT_SRC_PATH)/TextConcatenator.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO SQLite zToolsO zUtilO zAppO zUtilF zDictO zCaseO zDataO

include $(BUILD_STATIC_LIBRARY)
