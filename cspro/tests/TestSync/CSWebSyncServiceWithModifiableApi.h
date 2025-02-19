#pragma once

#include <zNetwork/CSWebConnection.h>
#include <zSyncO/CSWebSyncService.h>


class CSWebConnectionWithModifiableApi : public CSWebConnection
{
public:
    using CSWebConnection::CSWebConnection;

    double GetApiVersion() const
    {
        return m_apiVersion;
    }

    void SetApiVersion(double version)
    {
        if( !m_originalApiVersion.has_value() )
            m_originalApiVersion = m_apiVersion;

        m_apiVersion = version;
    }

    void RestoreApiVersion()
    {
        if( m_originalApiVersion.has_value() )
            m_apiVersion = *m_originalApiVersion;
    }

private:
    std::optional<double> m_originalApiVersion;
};


class CSWebSyncServiceWithModifiableApi : public CSWebSyncService
{
public:
    CSWebSyncServiceWithModifiableApi(std::unique_ptr<HttpConnection> http_connection, SyncConnectionString sync_connection_string, LoginCredentials login_credentials)
        :   CSWebSyncService(std::make_unique<CSWebConnectionWithModifiableApi>(std::move(http_connection), std::move(sync_connection_string), std::move(login_credentials)))
    {
    }

    CSWebConnectionWithModifiableApi& GetCSWebConnection()
    {
        return assert_cast<CSWebConnectionWithModifiableApi&>(CSWebSyncService::GetCSWebConnection());
    }
};
