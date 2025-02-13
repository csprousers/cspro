LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMultimediaO
ZMULTIMEDIAO_SRC_PATH   := ../../../../../zMultimediaO

LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/BmpFile.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Image.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Reader.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/Mp4Writer.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/QRCode.cpp
LOCAL_SRC_FILES         += $(ZMULTIMEDIAO_SRC_PATH)/../external/qrcodegen/qrcodegen.cpp


LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/mp4v2/include
LOCAL_STATIC_LIBRARIES  := zToolsO zUtilO mp4v2 zlib

include $(BUILD_STATIC_LIBRARY)
