#include "stdafx.h"
#include "SyncServerConnectionFactory.h"
#include "BluetoothObexConnection.h"
#include "BluetoothSyncServerConnection.h"
#include "CSWebSyncServerConnection.h"
#include "DropboxSyncServerConnection.h"
#include "FtpSyncServerConnection.h"
#include "ISyncServerConnection.h"
#include "LocalFileSyncServerConnection.h"
#include <zNetwork/IFtpConnection.h>
#include <zNetwork/IHttpConnection.h>

#ifdef WIN_DESKTOP
#include "DropboxLocalSyncServerConnection.h"
#include "WinBluetoothAdapter.h"
#include <zNetwork/CurlFtpConnection.h>
#include <zNetwork/CurlHttpConnection.h>
#endif


namespace {

    // Extract protocol portion of url i.e. http, https, ftp...
    CString getUrlProtocol(CString url)
    {
        int protocolEnd = url.Find(_T("://"));
        if (protocolEnd > 0)
            return url.Left(protocolEnd);
        else
            return CString();
    }
}

SyncServerConnectionFactory::SyncServerConnectionFactory(IBluetoothAdapter* pBluetoothAdapter):
    m_pBluetoothAdapter(pBluetoothAdapter)
{
}

std::shared_ptr<IHttpConnection> createHttpConnection()
{
#ifdef ANDROID
    return std::shared_ptr<IHttpConnection>(PlatformInterface::GetInstance()->GetApplicationInterface()->CreateAndroidHttpConnection());
#elif defined(WIN_DESKTOP)
    return std::make_shared<CurlHttpConnection>();
#elif defined(WASM)
    throw CSProException("WASM_TODO -- Not implemented: createHttpConnection");
#else
    throw CSProException("Not implemented: createHttpConnection");
#endif
}

std::shared_ptr<IFtpConnection> createIFtpConnection()
{
#ifdef ANDROID
    return std::shared_ptr<IFtpConnection>(PlatformInterface::GetInstance()->GetApplicationInterface()->CreateAndroidFtpConnection());
#elif defined(WIN_DESKTOP)
    return std::make_shared<CurlFtpConnection>();
#elif defined(WASM)
    throw CSProException("WASM_TODO -- Not implemented: createIFtpConnection");
#else
    throw CSProException("Not implemented: createIFtpConnection");
#endif
}

ISyncServerConnection* SyncServerConnectionFactory::createCSWebConnection(CString hostUrl, CString username, CString password)
{
    return new CSWebSyncServerConnection(createHttpConnection(), hostUrl, username, password);
}

ISyncServerConnection * SyncServerConnectionFactory::createCSWebConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore)
{
    return new CSWebSyncServerConnection(createHttpConnection(), hostUrl, pLoginDlg, pCredentialStore);
}

ISyncServerConnection* SyncServerConnectionFactory::createBluetoothConnection(const BluetoothDeviceInfo& deviceInfo)
{
    BluetoothObexConnection* pBluetoothConnection = new BluetoothObexConnection(m_pBluetoothAdapter);
    return new BluetoothSyncServerConnection(m_pBluetoothAdapter, pBluetoothConnection, deviceInfo);
}

ISyncServerConnection * SyncServerConnectionFactory::createBluetoothConnection(IChooseBluetoothDeviceDialog* pChooseDeviceDialog)
{
    BluetoothObexConnection* pBluetoothConnection = new BluetoothObexConnection(m_pBluetoothAdapter);
    return new BluetoothSyncServerConnection(m_pBluetoothAdapter, pBluetoothConnection, pChooseDeviceDialog);
}

ISyncServerConnection* SyncServerConnectionFactory::createDropboxConnection(CString accessToken)
{
    return new DropboxSyncServerConnection(createHttpConnection(), accessToken);
}

ISyncServerConnection* SyncServerConnectionFactory::createDropboxConnection(IDropboxAuthDialog* pAuthDlg, ICredentialStore* pCredentialStore)
{
    return new DropboxSyncServerConnection(createHttpConnection(), pAuthDlg, pCredentialStore);
}

ISyncServerConnection* SyncServerConnectionFactory::createFtpConnection(CString hostUrl, CString username, CString password)
{
    return new FtpSyncServerConnection(createIFtpConnection(), hostUrl, username, password);
}

ISyncServerConnection * SyncServerConnectionFactory::createFtpConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore)
{
    return new FtpSyncServerConnection(createIFtpConnection(), hostUrl, pLoginDlg, pCredentialStore);
}

ISyncServerConnection* SyncServerConnectionFactory::createLocalFileConnection(CString root_directory)
{
    return new LocalFileSyncServerConnection(root_directory);
}

ISyncServerConnection* SyncServerConnectionFactory::createDropboxLocalConnection()
{
#ifdef WIN_DESKTOP
    return new DropboxLocalSyncServerConnection();
#else
    return nullptr;
#endif
}

void SyncServerConnectionFactory::destroy(ISyncServerConnection* pConn)
{
    delete pConn;
}
