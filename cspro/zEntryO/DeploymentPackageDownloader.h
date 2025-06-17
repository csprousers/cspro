#pragma once

#include <zEntryO/zEntryO.h>
#include <zSyncO/ApplicationPackageManager.h>
#include <zSyncO/SyncClient.h>


// Manage process of choosing and downloading an application deployment package.

class CLASS_DECL_ZENTRYO DeploymentPackageDownloader
{
public:
    DeploymentPackageDownloader(ApplicationPackageManager application_package_manager);

    SyncClient::SyncResult ConnectToServer(const SyncConnectionString& sync_connection_string);
    void Disconnect();

    SyncClient::SyncResult List(std::vector<ApplicationPackage>& packages);

    SyncClient::SyncResult Install(const std::string& package_name, bool force_full_update);

    SyncClient::SyncResult ListUpdatable(std::vector<ApplicationPackage>& packages);

    SyncClient::SyncResult Update(const std::string& package_name, const std::string& server_url);

private:
    ApplicationPackageManager m_applicationPackageManager;
    std::shared_ptr<IBluetoothAdapter> m_bluetoothAdapter;
    std::unique_ptr<SyncClient> m_syncClient;
};
