LOCAL_PATH  := $(call my-dir)

#Savy for Crystax NDK debug build in Android Studio -*FOR USE ONLY WITH DEBUG*
ifeq ($(APP_OPTIM),debug)
    cmd-strip :=
    LOCAL_CPPFLAGS += -gdwarf-2 #Only needed if you use GCC, not Clang
endif

include $(LOCAL_PATH)/Engine.mk
include $(LOCAL_PATH)/mp4v2.mk
include $(LOCAL_PATH)/rtf2html.mk
include $(LOCAL_PATH)/SQLite.mk
include $(LOCAL_PATH)/yamlcpp.mk
include $(LOCAL_PATH)/zAction.mk
include $(LOCAL_PATH)/zAppO.mk
include $(LOCAL_PATH)/zBridgeO.mk
include $(LOCAL_PATH)/zCaseO.mk
include $(LOCAL_PATH)/zConcatO.mk
include $(LOCAL_PATH)/zDataO.mk
include $(LOCAL_PATH)/zDictO.mk
include $(LOCAL_PATH)/zDiffO.mk
include $(LOCAL_PATH)/zEngineF.mk
include $(LOCAL_PATH)/zEngineO.mk
include $(LOCAL_PATH)/zExcelO.mk
include $(LOCAL_PATH)/zExportO.mk
include $(LOCAL_PATH)/zFormatterO.mk
include $(LOCAL_PATH)/zFormO.mk
include $(LOCAL_PATH)/zFreqO.mk
include $(LOCAL_PATH)/zHtml.mk
include $(LOCAL_PATH)/zIndexO.mk
include $(LOCAL_PATH)/zJavaScript.mk
include $(LOCAL_PATH)/zJson.mk
include $(LOCAL_PATH)/zlib.mk
include $(LOCAL_PATH)/zListingO.mk
include $(LOCAL_PATH)/zLogicO.mk
include $(LOCAL_PATH)/zMapping.mk
include $(LOCAL_PATH)/zMessageO.mk
include $(LOCAL_PATH)/zMultimediaO.mk
include $(LOCAL_PATH)/zNetwork.mk
include $(LOCAL_PATH)/zPackO.mk
include $(LOCAL_PATH)/zParadataO.mk
include $(LOCAL_PATH)/zPlatformO.mk
include $(LOCAL_PATH)/zReformatO.mk
include $(LOCAL_PATH)/zReportO.mk
include $(LOCAL_PATH)/zSortO.mk
include $(LOCAL_PATH)/zSyncO.mk
include $(LOCAL_PATH)/zToolsO.mk
include $(LOCAL_PATH)/zUtilF.mk
include $(LOCAL_PATH)/zUtilO.mk
include $(LOCAL_PATH)/zZipO.mk


include $(CLEAR_VARS)

LOCAL_MODULE            := CSEntry
CSENTRY_SRC_PATH        := src

LOCAL_SRC_FILES         := $(CSENTRY_SRC_PATH)/JNIHelpers.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/ActionInvoker.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/ActionInvokerPortableRunner.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidApplicationInterface.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidBluetoothAdapter.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidBluetoothObexTransport.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidEngineInterface.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidFtpConnection.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidHttpConnection.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/AndroidMapUI.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/GeometryJni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_bridge_CNPifFile.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_engine_EngineInterface_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_form_CDEField_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_form_EntryPage_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_smartsync_http_IStreamWrapper_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_smartsync_http_OStreamWrapper_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_smartsync_p2p_AndroidBluetoothAdapter_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/gov_census_cspro_smartsync_SyncListenerWrapper_jni.cpp
LOCAL_SRC_FILES         += $(CSENTRY_SRC_PATH)/PortableLocalhostAndroid.cpp


LOCAL_LDLIBS            := -llog -landroid
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY=1
LOCAL_CFLAGS            += -DUSE_BINARY=1
LOCAL_CFLAGS            += -DANDROID=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/easylogging
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/rxcpp
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/geometry.hpp/include
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/variant/include
LOCAL_STATIC_LIBRARIES  := Engine zEngineF zEngineO zAction zLogicO zFormatterO zSyncO zNetwork zFormO zReportO zFreqO zParadataO zListingO zExcelO zBridgeO zDiffO zReformatO zIndexO zConcatO zSortO zPackO zDataO zExportO zCaseO zDictO zZipO zMapping zMessageO zAppO zMultimediaO zHtml zUtilF zUtilO zJavaScript zJson zToolsO zPlatformO SQLite mp4v2 zlib

include $(BUILD_SHARED_LIBRARY)
