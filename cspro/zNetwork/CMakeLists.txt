project(zNetwork)

set(EASYLOGGING_DIRECTORY ${EXTERNAL_DIRECTORY}/easylogging)

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

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

set_source_files_properties(
    ${EASYLOGGING_DIRECTORY}/easylogging++.cc
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)
