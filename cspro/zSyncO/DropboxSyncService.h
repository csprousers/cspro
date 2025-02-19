#pragma once

#include <zSyncO/FileBasedSyncService.h>

class DropboxConnection;


// Sync service for Dropbox.

class DropboxSyncService : public FileBasedSyncService
{
public:
    DropboxSyncService(std::shared_ptr<DropboxConnection> dropbox_connection);

    // ISyncService overrides
    std::shared_ptr<ConnectResponse> Connect() override;
    void Disconnect() override;

protected:
    // FileBasedSyncService overrides
    void SetDownloadPffSyncServiceParams(PFF& pff) override;

private:
    std::shared_ptr<DropboxConnection> m_dropboxConnection;
};
