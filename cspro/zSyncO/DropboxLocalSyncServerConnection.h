#pragma once
#include "LocalFileSyncServerConnection.h"

// Local file connection for fast download of files from Dropbox
class DropboxLocalSyncServerConnection : public LocalFileSyncServerConnection
{
public:

    DropboxLocalSyncServerConnection();

    // Connect to remote server
    // Throws SyncException.
    virtual ConnectResponse* connect();
};

