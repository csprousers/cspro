#pragma once

#include <zSyncO/FileBasedSyncService.h>

class LocalFileConnection;
class SyncConnectionString;


// Sync service for using a directory on the client, allowing for quicker interaction
// with files (such as from Dropbox, or a FTP server mapped to a local drive).

class LocalFileSyncService : public FileBasedSyncService
{
private:
    LocalFileSyncService(std::shared_ptr<LocalFileConnection> local_file_connection, const char* local_file_sync_type);

public:
    LocalFileSyncService(const SyncConnectionString& sync_connection_string, const char* local_file_sync_type = "Local File");

    // ISyncService overrides
    std::shared_ptr<ConnectResponse> Connect() override;
    void Disconnect() override;

protected:
    // FileBasedSyncService overrides
    void SetDownloadPffSyncServiceParams(PFF& pff) override;

protected:
    virtual std::unique_ptr<DeviceId> GetServerDeviceIdOverride() const;

private:
    std::shared_ptr<LocalFileConnection> m_localFileConnection;
};
