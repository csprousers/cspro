#include "StdAfx.h"
#include "DeploymentPackageDownloader.h"
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncServerConnectionFactory.h>
#include <zSyncO/SyncCredentialStore.h>
#include <zSyncO/SyncClient.h>
#include <zUtilO/TemporaryFile.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zPlatformO/PlatformInterface.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/IDropboxAuthDialog.h>
#include <zSyncO/IChooseBluetoothDeviceDialog.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/ISyncListener.h>
#ifndef WIN_DESKTOP
#include <zSyncO/ObexConstants.h>
#endif
#include <engine/SyncListener.h>
#ifdef WIN_DESKTOP
#include <zSyncF/ChooseBluetoothDeviceDialog.h>
#include <zSyncF/DropboxAuthDialog.h>
#include <zSyncF/LoginDialog.h>
#endif

namespace {

#ifndef WIN_DESKTOP
    class CLoginDialog : public ILoginDialog
    {

        std::optional<std::tuple<CString, CString>> Show(const CString& server, bool show_invalid_error) override
        {
            CString username, password;
            std::tuple<CString, CString*, CString*> message_parameters = std::make_tuple(
                server,
                &username,
                &password);
            auto credentials = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowLoginDialog(server, show_invalid_error);
            if (credentials)
                return std::make_optional(std::make_tuple(credentials->username, credentials->password));
            else
                return {};
        }

    };

    class DropboxAuthDialog : public IDropboxAuthDialog
    {

        CString Show(CString csClientId) override
        {
            return PlatformInterface::GetInstance()->GetApplicationInterface()->AuthorizeDropbox(csClientId);
        }

    };

    class ChooseBluetoothDeviceDialog : public IChooseBluetoothDeviceDialog
    {
    public:
        explicit ChooseBluetoothDeviceDialog(IBluetoothAdapter*)
        {}

        bool Show(BluetoothDeviceInfo& deviceInfo) override
        {
            auto result = PlatformInterface::GetInstance()->GetApplicationInterface()->ChooseBluetoothDevice(OBEX_SYNC_SERVICE_UUID);
             if (result) {
                 deviceInfo = *result;
                 return true;
             } else {
                 return false;
             }
        }
    };

#endif

    // the deployment package system message issuer will only be used for formatting
    class DeploymentPackageSystemMessageIssuer : public SystemMessageIssuer
    {
        void OnIssue(MessageType, int, const std::wstring&) override { }
    };

    // Deployment package error reporter for sync listener
    class DeploymentPackageSyncListenerErrorReporter : public ISyncListenerErrorReporter
    {
    public:
        void OnError(int message_number, va_list parg) override
        {
            CString formattedMsg = m_deploymentPackageSystemMessageIssuer.GetFormattedMessageVA(message_number, parg);
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(CString(), formattedMsg, MB_OK);
        }

        std::wstring Format(int message_number, va_list parg) override
        {
            return m_deploymentPackageSystemMessageIssuer.GetFormattedMessageVA(message_number, parg);
        }

    private:
        DeploymentPackageSystemMessageIssuer m_deploymentPackageSystemMessageIssuer;
    };
}


DeploymentPackageDownloader::DeploymentPackageDownloader()
{
#ifdef WIN_DESKTOP
    m_pBluetoothAdapter = new WinBluetoothAdapter();
#else
    m_pBluetoothAdapter = PlatformInterface::GetInstance()->GetApplicationInterface()->CreateAndroidBluetoothAdapter();
#endif
    m_pSyncServerConnectionFactory = new SyncServerConnectionFactory(m_pBluetoothAdapter);
    m_pSyncCredentialStore = new SyncCredentialStore();
    m_pChooseBluetoothDlg = new ChooseBluetoothDeviceDialog(m_pBluetoothAdapter);
    m_pLoginDlg = new CLoginDialog();
    m_pDropboxAuthDlg = new DropboxAuthDialog();
    m_pSyncListener = new SyncListener(std::make_unique<DeploymentPackageSyncListenerErrorReporter>());
    m_pSyncClient = new SyncClient(GetDeviceId(), m_pSyncServerConnectionFactory);
    m_pSyncClient->setListener(m_pSyncListener);
    m_pListener = nullptr;
}

DeploymentPackageDownloader::~DeploymentPackageDownloader()
{
    delete m_pSyncClient;
    delete m_pSyncListener;
    delete m_pSyncCredentialStore;
    delete m_pChooseBluetoothDlg;
    delete m_pLoginDlg;
    delete m_pDropboxAuthDlg;
}

SyncClient::SyncResult DeploymentPackageDownloader::ConnectToServer(const CString& server)
{
    return m_pSyncClient->connect(server, m_pLoginDlg, m_pDropboxAuthDlg, m_pSyncCredentialStore, m_pChooseBluetoothDlg);
}

SyncClient::SyncResult DeploymentPackageDownloader::ConnectToServer(const CString &server,
        const CString &username,
         const CString &password)
{
    return m_pSyncClient->connect(server, m_pLoginDlg, m_pDropboxAuthDlg, m_pSyncCredentialStore, m_pChooseBluetoothDlg,
            username, password);
}

void DeploymentPackageDownloader::Disconnect()
{
    m_pSyncClient->disconnect();
}

SyncClient::SyncResult DeploymentPackageDownloader::List(std::vector<ApplicationPackage>& packages)
{
    auto result = m_pSyncClient->listApplicationPackages(packages);
    if (result == SyncClient::SyncResult::SYNC_OK) {
        // Add in installed versions of existing packages
        auto installed_packages = m_pSyncClient->listInstalledApplicationPackages();

        for (ApplicationPackage& server_package : packages) {
            auto installed_package = std::find_if(installed_packages.begin(), installed_packages.end(), [&server_package](auto& cp) {return server_package.getName() == cp.getName();});
            if (installed_package != installed_packages.end()) {
                server_package.setInstalledVersionBuildTime(installed_package->getBuildTime());
            }
        }
    }
    return result;
}

SyncClient::SyncResult DeploymentPackageDownloader::Install(const CString& packageName, bool forceFullInstall)
{
    return m_pSyncClient->downloadApplicationPackage(packageName, forceFullInstall);
}

SyncClient::SyncResult DeploymentPackageDownloader::ListUpdatable(std::vector<ApplicationPackage>& packages)
{
    auto installed = m_pSyncClient->listInstalledApplicationPackages();

    std::map<CString, std::vector<ApplicationPackage>> apps_grouped_by_server;
    for (const auto& p : installed)
    {
        auto server_url = p.getServerUrl();

        if (!server_url.IsEmpty()) {
            apps_grouped_by_server[server_url].emplace_back(p);
        }
    }

    std::vector<ApplicationPackage> updatable;
    for (const auto& apps_for_server : apps_grouped_by_server) {
        auto result = ConnectToServer(apps_for_server.first);
        if (result == SyncClient::SyncResult::SYNC_CANCELED)
            return SyncClient::SyncResult::SYNC_CANCELED;

        if (result == SyncClient::SyncResult::SYNC_OK) {
            std::vector<ApplicationPackage> server_packages;
            result = m_pSyncClient->listApplicationPackages(server_packages);
            m_pSyncClient->disconnect();
            if (result == SyncClient::SyncResult::SYNC_CANCELED)
                return SyncClient::SyncResult::SYNC_CANCELED;

            const auto& client_packages = apps_for_server.second;
            for (const auto& client_package : client_packages) {
                auto server_package = std::find_if(server_packages.begin(), server_packages.end(), [&client_package](auto& sp) {return client_package.getName() == sp.getName();});
                if (server_package != server_packages.end() && server_package->getBuildTime() > client_package.getBuildTime()) {
                    server_package->setInstalledVersionBuildTime(client_package.getBuildTime());
                    server_package->setDeploymentType(client_package.getDeploymentType());
                    server_package->setServerUrl(client_package.getServerUrl());
                    updatable.emplace_back(*server_package);
                }
            }
        }
    }

    packages = updatable;
    return SyncClient::SyncResult::SYNC_OK;
}

SyncClient::SyncResult DeploymentPackageDownloader::Update(const CString &package_name, const CString &server_url)
{
    auto result = ConnectToServer(server_url);
    if (result != SyncClient::SyncResult::SYNC_OK)
        return result;

    return Install(package_name, false);
}
