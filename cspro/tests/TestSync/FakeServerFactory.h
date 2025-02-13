#pragma once

#include <zSyncO/ISyncServerConnectionFactory.h>
#include "FakeServer.h"
class CDataDict;

// Server factory that returns a FakeServer
class FakeServerFactory : public ISyncServerConnectionFactory {

public:

    FakeServerFactory(const std::vector<std::shared_ptr<Case>>& responseCases)
        : m_pFakeServer(nullptr),
          m_responseCases(responseCases)
    {
    }

    virtual ISyncServerConnection* createCSWebConnection(CString hostUrl, CString username, CString password)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createCSWebConnection(CString, ILoginDialog*, ICredentialStore*)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createBluetoothConnection(const BluetoothDeviceInfo& )
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createBluetoothConnection(IChooseBluetoothDeviceDialog*)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createDropboxConnection(CString oauthToken)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createDropboxConnection(IDropboxAuthDialog*, ICredentialStore*)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createFtpConnection(CString hostUrl, CString username, CString password)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createFtpConnection(CString, ILoginDialog*, ICredentialStore*)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createLocalFileConnection(CString root_directory)
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual ISyncServerConnection* createDropboxLocalConnection()
    {
        m_pFakeServer = new FakeServer(m_responseCases);
        return m_pFakeServer;
    }

    virtual void destroy(ISyncServerConnection* pConn)
    {
        delete pConn;
    }

    FakeServer* getServer()
    {
        return m_pFakeServer;
    }

private:
    FakeServer* m_pFakeServer;
    std::vector<std::shared_ptr<Case>> m_responseCases;
};
