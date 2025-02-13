#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/BluetoothDeviceInfo.h>
struct ISyncServerConnection;
struct ILoginDialog;
struct ICredentialStore;
struct IDropboxAuthDialog;
struct IChooseBluetoothDeviceDialog;

// Factory class for creating sync server connections
struct SYNC_API ISyncServerConnectionFactory {

    virtual ISyncServerConnection* createCSWebConnection(CString hostUrl, CString username, CString password) = 0;
    virtual ISyncServerConnection* createCSWebConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore) = 0;

    virtual ISyncServerConnection* createBluetoothConnection(const BluetoothDeviceInfo& deviceInfo) = 0;
    virtual ISyncServerConnection* createBluetoothConnection(IChooseBluetoothDeviceDialog* pChooseDeviceDialog) = 0;

    virtual ISyncServerConnection* createDropboxConnection(CString oauthToken) = 0;
    virtual ISyncServerConnection* createDropboxConnection(IDropboxAuthDialog* pAuthDlg, ICredentialStore* pCredentialStore) = 0;

    virtual ISyncServerConnection* createFtpConnection(CString hostUrl, CString username, CString password) = 0;
    virtual ISyncServerConnection* createFtpConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore) = 0;

    virtual ISyncServerConnection* createLocalFileConnection(CString root_directory) = 0;

    virtual ISyncServerConnection* createDropboxLocalConnection() = 0;

    virtual void destroy(ISyncServerConnection* pConn) = 0;

    virtual ~ISyncServerConnectionFactory() {};
};
