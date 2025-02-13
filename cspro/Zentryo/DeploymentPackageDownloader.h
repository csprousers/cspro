#pragma once
#include <Zentryo/zEntryO.h>
#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/SyncClient.h>

struct IBluetoothAdapter;
struct ISyncListener;
struct ISyncServerConnectionFactory;
struct ICredentialStore;
struct ILoginDialog;
struct IDropboxAuthDialog;
struct IChooseBluetoothDeviceDialog;
class SyncClient;

///<summary>Manage process of choosing and downloading an application deployment package.</summary>
class CLASS_DECL_ZENTRYO DeploymentPackageDownloader {
public:

    DeploymentPackageDownloader();

    ~DeploymentPackageDownloader();

    SyncClient::SyncResult ConnectToServer(const CString& server);
    SyncClient::SyncResult ConnectToServer(const CString &server, const CString &username, const CString &password);
    void Disconnect();

    SyncClient::SyncResult List(std::vector<ApplicationPackage>& packages);

    SyncClient::SyncResult Install(const CString& packageName, bool forceFullUpdate);

    SyncClient::SyncResult ListUpdatable(std::vector<ApplicationPackage>& packages);

    SyncClient::SyncResult  Update(const CString& package_name, const CString& server_url);

private:
    IBluetoothAdapter* m_pBluetoothAdapter;
    ISyncListener* m_pSyncListener;
    ISyncServerConnectionFactory* m_pSyncServerConnectionFactory;
    ICredentialStore* m_pSyncCredentialStore;
    ILoginDialog* m_pLoginDlg;
    IDropboxAuthDialog* m_pDropboxAuthDlg;
    IChooseBluetoothDeviceDialog* m_pChooseBluetoothDlg;
    ISyncListener* m_pListener;
    SyncClient* m_pSyncClient;
};
