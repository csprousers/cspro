project(zSyncO)

include(../WASM/Definitions.cmake)

set(EASYLOGGING_DIRECTORY ${EXTERNAL_DIRECTORY}/easylogging)

# the optimization level for release builds fails for a few sync files that include many headers
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O0)
endif()

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/jsoncons
    ${EXTERNAL_DIRECTORY}/rxcpp
    ${EASYLOGGING_DIRECTORY}
)

add_library(${PROJECT_NAME}
    AppSyncParamRunner.cpp
    ApplicationPackage.cpp
    ApplicationPackageManager.cpp
    BluetoothChunk.cpp
    BluetoothSyncServerConnection.cpp
    BluetoothObexConnection.cpp
    BluetoothObexServer.cpp
    ConnectResponse.cpp
    CSWebDataChunk.cpp
    CSWebSyncServerConnection.cpp
    DefaultChunk.cpp
    DropboxSyncServerConnection.cpp
    DropboxDB.cpp
    ExponentialBackoff.cpp
    FtpSyncServerConnection.cpp
    JsonConverter.cpp
    LocalFileSyncServerConnection.cpp
    ObexClient.cpp
    ObexConstants.cpp
    ObexHeader.cpp
    ObexPacket.cpp
    ObexPacketSerializer.cpp
    ObexServer.cpp
    OauthTokenRequest.cpp
    OauthTokenResponse.cpp
    SyncClient.cpp
    SyncCustomHeaders.cpp
    SyncObexHandler.cpp
    SyncErrorResponse.cpp
    SyncGetResponse.cpp
    SyncPutResponse.cpp
    SyncRequest.cpp
    SyncServerConnectionFactory.cpp

    ${EASYLOGGING_DIRECTORY}/easylogging++.cc
)
