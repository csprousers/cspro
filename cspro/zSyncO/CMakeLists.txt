project(zSyncO)

include(../WASM/Definitions.cmake)

# the optimization level for release builds fails for a few sync files that include many headers
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O0)
endif()

include_directories(${PROJECT_NAME} PUBLIC
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
    SyncMessage.cpp
    SyncObexHandler.cpp
    SyncRunner.cpp
    SyncRunnerActionInvoker.cpp
    SyncServiceFactory.cpp
)
