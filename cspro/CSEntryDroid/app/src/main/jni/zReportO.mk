LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zReportO
ZREPORTO_SRC_PATH       := ../../../../../zReportO

LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77Report.cpp
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77ReportManager.cpp
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77ReportNodes.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zUtilO zJson zToolsO zPlatformO zSql

include $(BUILD_STATIC_LIBRARY)
