LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= rtf2html
RTF2HTML_SRC_PATH       := ../../../../../rtf2html_dll

LOCAL_SRC_FILES         += $(RTF2HTML_SRC_PATH)/fmt_opts.cpp
LOCAL_SRC_FILES         += $(RTF2HTML_SRC_PATH)/rtf_keyword.cpp
LOCAL_SRC_FILES         += $(RTF2HTML_SRC_PATH)/rtf_table.cpp
LOCAL_SRC_FILES         += $(RTF2HTML_SRC_PATH)/rtf2html.cpp

LOCAL_CFLAGS    		+= -DANDROID=1
LOCAL_CFLAGS    		+= -DUNICODE=1
LOCAL_CFLAGS    		+= -D_UNICODE=1
LOCAL_CFLAGS            += -DRTF2HTML_DLL_EXPORTS

include $(BUILD_STATIC_LIBRARY)
