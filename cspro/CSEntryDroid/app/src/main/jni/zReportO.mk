LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zReportO
ZREPORTO_SRC_PATH       := ../../../../../zReportO

LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77Report.cpp
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77ReportManager.cpp
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/Pre77ReportNodes.cpp
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/attribute.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/char_ref.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/error.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/parser.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/string_buffer.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/string_piece.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/tag.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/tokenizer.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/utf8.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/util.c
LOCAL_SRC_FILES         += $(ZREPORTO_SRC_PATH)/../external/gumbo/vector.c

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zUtilO zJson zToolsO zPlatformO SQLite

include $(BUILD_STATIC_LIBRARY)
