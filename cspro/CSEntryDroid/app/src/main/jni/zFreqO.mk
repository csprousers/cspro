LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zFreqO
ZFREQO_SRC_PATH         := ../../../../../zFreqO

LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/ExcelFrequencyPrinter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/Frequency.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/FrequencyCounter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/FrequencyPrinterEntry.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/FrequencyPrinterHelpers.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/FrequencyPrinterOptions.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/HtmlFrequencyPrinter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/JsonFileFrequencyPrinter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/JsonFrequencyPrinter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/JsonStringFrequencyPrinter.cpp
LOCAL_SRC_FILES         += $(ZFREQO_SRC_PATH)/TextFrequencyPrinter.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zHtml zAppO zDictO zExcelO

include $(BUILD_STATIC_LIBRARY)
