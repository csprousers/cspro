LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    	:= zPlatformO
ZPLATFORMO_SRC_PATH := ../../../../../zPlatformO

LOCAL_SRC_FILES		+= $(ZPLATFORMO_SRC_PATH)/PlatformInterface.cpp
LOCAL_SRC_FILES 	+= $(ZPLATFORMO_SRC_PATH)/CSProStdioFile.cpp
LOCAL_SRC_FILES		+= $(ZPLATFORMO_SRC_PATH)/MobileStringConversion.cpp
LOCAL_SRC_FILES		+= $(ZPLATFORMO_SRC_PATH)/util_snprintf.cpp
LOCAL_SRC_FILES		+= $(ZPLATFORMO_SRC_PATH)/PortableFStream.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS    	+= -DUNICODE=1
LOCAL_CFLAGS        += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
