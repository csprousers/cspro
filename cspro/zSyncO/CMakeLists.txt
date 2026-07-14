project(zSyncO)

include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/rxcpp
)

add_library(${PROJECT_NAME}
    AppSyncParamRunner.cpp
    ApplicationPackage.cpp
    ApplicationPackageManager.cpp
    BarcodeCredentials.cpp
    BluetoothChunk.cpp
    BluetoothSyncService.cpp
    BluetoothObexConnection.cpp
    BluetoothObexServer.cpp
    CSWebSyncService.cpp
    DataSyncer.cpp
    DialogBasedSyncListener.cpp
    DropboxLocalSyncService.cpp
    DropboxSyncService.cpp
    FileBasedParadataSyncer.cpp
    FileBasedSyncService.cpp
    FtpSyncService.cpp
    JsonConverter.cpp
    LocalFileSyncService.cpp
    NetworkDataChunk.cpp
    ObexClient.cpp
    ObexConstants.cpp
    ObexHeader.cpp
    ObexPacket.cpp
    ObexPacketSerializer.cpp
    ObexServer.cpp
    SyncClient.cpp
    SyncDictionaryInfo.cpp
    SyncLoginAccessor.cpp
    SyncMessage.cpp
    SyncObexHandler.cpp
    SyncRunner.cpp
    SyncRunnerActionInvoker.cpp
    SyncServiceFactory.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
