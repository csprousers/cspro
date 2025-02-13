LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zUtilF
ZUTILF_SRC_PATH         := ../../../../../zUtilF

LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/ChoiceDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/HtmlDialogFunctionRunner.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/ImageCaptureDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/ImageViewDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/NoteEditDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/ProcessSummaryDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/ProgressDlgFactory.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/SelectDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/SelectFileDlg.cpp
LOCAL_SRC_FILES         += $(ZUTILF_SRC_PATH)/TextInputDlg.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zJson zHtml

include $(BUILD_STATIC_LIBRARY)
