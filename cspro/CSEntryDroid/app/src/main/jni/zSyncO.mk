LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zSyncO
ZSYNCO_SRC_PATH         := ../../../../../zSyncO

LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/AppSyncParamRunner.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ApplicationPackage.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ApplicationPackageManager.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothChunk.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothSyncServerConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothObexConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/BluetoothObexServer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ConnectResponse.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/CSWebDataChunk.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/CSWebSyncServerConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DefaultChunk.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DropboxSyncServerConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/DropboxDB.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ExponentialBackoff.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/FtpSyncServerConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/JsonConverter.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/LocalFileSyncServerConnection.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexClient.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexConstants.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexHeader.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexPacket.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexPacketSerializer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/ObexServer.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/OauthTokenRequest.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/OauthTokenResponse.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncClient.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncCustomHeaders.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncObexHandler.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncErrorResponse.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncGetResponse.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncPutResponse.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncRequest.cpp
LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/SyncServerConnectionFactory.cpp

LOCAL_SRC_FILES         += $(ZSYNCO_SRC_PATH)/../external/easylogging/easylogging++.cc

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY
LOCAL_CFLAGS            += -DUSE_BINARY
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/easylogging
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/jsoncons
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/rxcpp
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zNetwork zCaseO zDataO zParadataO
LOCAL_LDFLAGS           :=
LOCAL_LDLIBS            :=
include $(BUILD_STATIC_LIBRARY)
