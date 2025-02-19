LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zNetwork
ZNETWORK_SRC_PATH       := ../../../../../zNetwork

LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/CSWebConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/DropboxConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/FileBasedConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/FtpConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/HeaderList.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/HttpConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/LocalFileConnection.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/LoginAccessor.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/LoginCredentials.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/OAuth2Authorizer.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/ObjectSerialization.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/SyncCredentialStore.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/SyncException.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/SyncListener.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/SyncLog.cpp
LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/SyncLogSyncListener.cpp

LOCAL_SRC_FILES         += $(ZNETWORK_SRC_PATH)/../external/easylogging/easylogging++.cc

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zUtilO zJson zZipO zMessageO

include $(BUILD_STATIC_LIBRARY)
