#include "stdafx.h"
#include "DropboxLocalSyncService.h"


DropboxLocalSyncService::DropboxLocalSyncService(std::string local_dropbox_directory)
    :   LocalFileSyncService(SyncConnectionString::CreateLocalFilesSyncConnectionString(std::move(local_dropbox_directory)), "Dropbox (Local)")
{
}


std::unique_ptr<DeviceId> DropboxLocalSyncService::GetServerDeviceIdOverride() const
{
    return std::make_unique<DeviceId>("Dropbox");
}
