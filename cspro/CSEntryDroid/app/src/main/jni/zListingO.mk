LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zListingO
ZLISTINGO_SRC_PATH      := ../../../../../zListingO

LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/CsvLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/DataFileLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/ErrorLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/ExcelLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/HtmlLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/JsonLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/Lister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/ListingHelpers.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/TextLister.cpp
LOCAL_SRC_FILES         += $(ZLISTINGO_SRC_PATH)/TextWriteFile.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zHtml zAppO zDictO zCaseO zDataO zBridgeO zExcelO

include $(BUILD_STATIC_LIBRARY)
