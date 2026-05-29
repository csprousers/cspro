LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zSyncO
ZSYNCO_SRC_PATH         := ../../../../../zSyncO

LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/AppSyncParamRunner.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ApplicationPackage.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ApplicationPackageManager.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BarcodeCredentials.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothChunk.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothObexConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothObexServer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/CSWebSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DataSyncer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DialogBasedSyncListener.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DropboxLocalSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DropboxSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/FileBasedParadataSyncer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/FileBasedSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/FtpSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/JsonConverter.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/LocalFileSyncService.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/NetworkDataChunk.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexClient.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexConstants.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexHeader.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexPacket.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexPacketSerializer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexServer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncClient.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncDictionaryInfo.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncLoginAccessor.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncMessage.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncObexHandler.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncRunner.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncRunnerActionInvoker.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncServiceFactory.cpp

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/rxcpp
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zNetwork zCaseO zDataO zParadataO

include $(BUILD_STATIC_LIBRARY)
