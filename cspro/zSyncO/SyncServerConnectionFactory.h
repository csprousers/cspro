#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncServerConnectionFactory.h>
struct IBluetoothAdapter;
struct IChooseBluetoothDeviceDialog;

// Factory class for creating server connections.
// Returns the appropriate connection implementation based on the
// url/serverDeviceName.
class SYNC_API SyncServerConnectionFactory : public ISyncServerConnectionFactory {
public:

    SyncServerConnectionFactory(IBluetoothAdapter* pBluetoothAdapter);

    virtual ISyncServerConnection* createCSWebConnection(CString hostUrl, CString username, CString password);

    virtual ISyncServerConnection* createCSWebConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    virtual ISyncServerConnection* createBluetoothConnection(const BluetoothDeviceInfo& deviceInfo);
    virtual ISyncServerConnection* createBluetoothConnection(IChooseBluetoothDeviceDialog* pChooseDeviceDialog);

    virtual ISyncServerConnection* createDropboxConnection(CString accessToken);
    virtual ISyncServerConnection* createDropboxConnection(IDropboxAuthDialog* pAuthDlg, ICredentialStore* pCredentialStore);

    virtual ISyncServerConnection* createFtpConnection(CString hostUrl, CString username, CString password);

    virtual ISyncServerConnection* createFtpConnection(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    virtual ISyncServerConnection* createLocalFileConnection(CString root_directory);

    virtual ISyncServerConnection* createDropboxLocalConnection();

    virtual void destroy(ISyncServerConnection* pConn);

private:
    IBluetoothAdapter* m_pBluetoothAdapter;
};
