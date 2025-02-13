LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zJavaScript
ZJAVASCRIPT_SRC_PATH    := ../../../../../zJavaScript
QUICKJS_SRC_PATH        := ../../../../../external/QuickJS

LOCAL_SRC_FILES         += $(ZJAVASCRIPT_SRC_PATH)/ActionInvokerJS.cpp
LOCAL_SRC_FILES         += $(ZJAVASCRIPT_SRC_PATH)/Executor.cpp
LOCAL_SRC_FILES         += $(ZJAVASCRIPT_SRC_PATH)/QuickJSAccess.cpp
LOCAL_SRC_FILES         += $(ZJAVASCRIPT_SRC_PATH)/Value.cpp
LOCAL_SRC_FILES         += $(QUICKJS_SRC_PATH)/cutils.c
LOCAL_SRC_FILES         += $(QUICKJS_SRC_PATH)/libbf.c
LOCAL_SRC_FILES         += $(QUICKJS_SRC_PATH)/libregexp.c
LOCAL_SRC_FILES         += $(QUICKJS_SRC_PATH)/libunicode.c
LOCAL_SRC_FILES         += $(QUICKJS_SRC_PATH)/quickjs.c

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zJson

include $(BUILD_STATIC_LIBRARY)
