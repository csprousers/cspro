#include "stdafx.h"
#include "LocalFileSyncService.h"
#include <zNetwork/LocalFileConnection.h>


LocalFileSyncService::LocalFileSyncService(std::shared_ptr<LocalFileConnection> local_file_connection, const char* const local_file_sync_type)
    :   FileBasedSyncService(local_file_sync_type, local_file_connection, false),
        m_localFileConnection(std::move(local_file_connection))
{
}


LocalFileSyncService::LocalFileSyncService(const SyncConnectionString& sync_connection_string, const char* const local_file_sync_type/* = "Local File"*/)
    :   LocalFileSyncService(std::make_unique<LocalFileConnection>(sync_connection_string), local_file_sync_type)
{
}


std::unique_ptr<DeviceId> LocalFileSyncService::GetServerDeviceIdOverride() const
{
    return nullptr;
}


std::shared_ptr<ConnectResponse> LocalFileSyncService::Connect()
{
    m_localFileConnection->Connect();

    std::unique_ptr<DeviceId> server_device_id_override = GetServerDeviceIdOverride();
    std::string directory_file_url = Encoders::ToFileUrl(PortableFunctions::PathRemoveTrailingSlash(m_localFileConnection->GetRootDirectory()));

    if( server_device_id_override == nullptr )
        server_device_id_override = std::make_unique<DeviceId>(directory_file_url);

    return std::make_unique<ConnectResponse>(std::move(*server_device_id_override), std::move(directory_file_url));
}


void LocalFileSyncService::Disconnect()
{
}


void LocalFileSyncService::SetDownloadPffSyncServiceParams(PFF& pff)
{
    pff.SetSyncService(SyncConnectionString::CreateLocalFilesSyncConnectionString(m_localFileConnection->GetRootDirectory()));
}
