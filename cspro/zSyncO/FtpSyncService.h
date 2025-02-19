#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/FileBasedSyncService.h>
#include <zNetwork/LoginCredentials.h>

class FtpConnection;


// Sync service for FTP servers.

class SYNC_API FtpSyncService : public FileBasedSyncService
{
public:
    FtpSyncService(std::shared_ptr<FtpConnection> ftp_connection,
                   SyncConnectionString sync_connection_string, LoginCredentials login_credentials);

    // ISyncService overrides
    std::shared_ptr<ConnectResponse> Connect() override;
    void Disconnect() override;

protected:
    // FileBasedSyncService overrides
    void SetDownloadPffSyncServiceParams(PFF& pff) override;

private:
    std::shared_ptr<FtpConnection> m_ftpConnection;
    SyncConnectionString m_syncConnectionString;
    LoginCredentials m_loginCredentials;
};
