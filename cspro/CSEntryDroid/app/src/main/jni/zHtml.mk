LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zHtml
ZHTML_SRC_PATH          := ../../../../../zHtml

LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/AccessUrlSerializer.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/CSHtmlDlgRunner.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/CssProvider.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/FileSystemVirtualFileMappingHandler.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/HtmlDlgBaseRunner.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/LocalhostUrl.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/PortableLocalFileServer.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/PortableLocalhost.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/VirtualFileMappingHandlers.cpp
LOCAL_SRC_FILES         += $(ZHTML_SRC_PATH)/WebViewSyncOperationMarker.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zMessageO

include $(BUILD_STATIC_LIBRARY)
