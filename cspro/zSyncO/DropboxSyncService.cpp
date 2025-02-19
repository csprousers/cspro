#include "stdafx.h"
#include "DropboxSyncService.h"
#include <zNetwork/DropboxConnection.h>


DropboxSyncService::DropboxSyncService(std::shared_ptr<DropboxConnection> dropbox_connection)
    :   FileBasedSyncService("Dropbox", dropbox_connection, true),
        m_dropboxConnection(std::move(dropbox_connection))
{
    ASSERT(m_dropboxConnection != nullptr);
}


std::shared_ptr<ConnectResponse> DropboxSyncService::Connect()
{
    std::string account_email = m_dropboxConnection->Connect();

    return std::make_unique<ConnectResponse>(DeviceId("Dropbox"), "Dropbox", std::move(account_email));
}


void DropboxSyncService::Disconnect()
{
    m_dropboxConnection->Disconnect();
}


void DropboxSyncService::SetDownloadPffSyncServiceParams(PFF& pff)
{
    pff.SetSyncService(SyncConnectionString::CreateDropboxSyncConnectionString());
}
