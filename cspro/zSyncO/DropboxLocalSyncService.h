#pragma once

#include <zSyncO/LocalFileSyncService.h>


// Sync service for Dropbox using the Dropbox directory on the client, allowing
// for quicker interaction with Dropbox files.

class DropboxLocalSyncService : public LocalFileSyncService
{
public:
    DropboxLocalSyncService(std::string local_dropbox_directory);

protected:
    // LocalFileSyncService overrides
    std::unique_ptr<DeviceId> GetServerDeviceIdOverride() const override;
};
