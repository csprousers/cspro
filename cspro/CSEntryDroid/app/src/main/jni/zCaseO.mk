LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zCaseO
ZCASEO_SRC_PATH         := ../../../../../zCaseO

LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/BinaryCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/Case.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseAccess.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseIO.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseItemIndex.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseItemPrinter.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseItemReference.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseJsonSerializer.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseLevel.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/CaseRecord.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/FixedWidthNumericCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/FixedWidthNumericWithStringBufferCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/FixedWidthStringCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/ItemSubitemSyncTask.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/NumericCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/Pre74_Case.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/StringBasedCaseConstructionReporter.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/StringCaseItem.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/TextToCaseConverter.cpp
LOCAL_SRC_FILES         += $(ZCASEO_SRC_PATH)/VectorClock.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zJson zUtilO zAppO zDictO zMessageO

include $(BUILD_STATIC_LIBRARY)
