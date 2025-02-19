#include "stdafx.h"
#include "FtpSyncService.h"
#include <zNetwork/FtpConnection.h>


FtpSyncService::FtpSyncService(std::shared_ptr<FtpConnection> ftp_connection,
                               SyncConnectionString sync_connection_string, LoginCredentials login_credentials)
    :   FileBasedSyncService("FTP", ftp_connection, true),
        m_ftpConnection(std::move(ftp_connection)),
        m_syncConnectionString(std::move(sync_connection_string)),
        m_loginCredentials(std::move(login_credentials))
{
    ASSERT(m_ftpConnection != nullptr);
    ASSERT(m_syncConnectionString.GetType() == SyncServiceType::Ftp);
}


std::shared_ptr<ConnectResponse> FtpSyncService::Connect()
{
    std::string username;
    m_ftpConnection->Connect(m_syncConnectionString, m_loginCredentials, &username);

    // FTP servers don't have an ID so we just use the URL as the device ID
    return std::make_unique<ConnectResponse>(m_ftpConnection->GetProvidedUrl(), m_ftpConnection->GetProvidedUrl(), std::move(username));
}


void FtpSyncService::Disconnect()
{
    m_ftpConnection->Disconnect();
}


void FtpSyncService::SetDownloadPffSyncServiceParams(PFF& pff)
{
    // only use the URL so that any properties (like a password) are not exposed
    pff.SetSyncService(m_syncConnectionString.GetUrl());
}
