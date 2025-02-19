project(zNetwork)

include(../WASM/Definitions.cmake)

set(EASYLOGGING_DIRECTORY ${EXTERNAL_DIRECTORY}/easylogging)

include_directories(${PROJECT_NAME} PUBLIC
    ${EASYLOGGING_DIRECTORY}
)

add_library(${PROJECT_NAME}
    CSWebConnection.cpp
    DropboxConnection.cpp
    FileBasedConnection.cpp
    FtpConnection.cpp
    HeaderList.cpp
    HttpConnection.cpp
    LocalFileConnection.cpp
    LoginAccessor.cpp
    LoginCredentials.cpp
    OAuth2Authorizer.cpp
    ObjectSerialization.cpp
    SyncCredentialStore.cpp
    SyncException.cpp
    SyncListener.cpp
    SyncLog.cpp
    SyncLogSyncListener.cpp

    ${EASYLOGGING_DIRECTORY}/easylogging++.cc
)
