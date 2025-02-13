LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMessageO
ZMESSAGEO_SRC_PATH      := ../../../../../zMessageO

LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/MessageEvaluator.cpp
LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/MessageFile.cpp
LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/MessageManager.cpp
LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/Messages.cpp
LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/SystemMessageIssuer.cpp
LOCAL_SRC_FILES         += $(ZMESSAGEO_SRC_PATH)/SystemMessages.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO

include $(BUILD_STATIC_LIBRARY)
