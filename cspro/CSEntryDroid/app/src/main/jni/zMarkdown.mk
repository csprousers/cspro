LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zMarkdown
ZMARKDOWN_SRC_PATH      := ../../../../../zMarkdown
MD4C_SRC_PATH           := ../../../../../external/md4c

LOCAL_SRC_FILES         += $(ZMARKDOWN_SRC_PATH)/HtmlParserCallback.cpp
LOCAL_SRC_FILES         += $(ZMARKDOWN_SRC_PATH)/Markdown.cpp

LOCAL_SRC_FILES         += $(MD4C_SRC_PATH)/entity.c
LOCAL_SRC_FILES         += $(MD4C_SRC_PATH)/md4c.c
LOCAL_SRC_FILES         += $(MD4C_SRC_PATH)/md4c-html.c

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO

include $(BUILD_STATIC_LIBRARY)
