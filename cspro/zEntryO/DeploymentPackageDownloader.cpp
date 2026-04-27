#include "StdAfx.h"
#include "DeploymentPackageDownloader.h"
#include <zPlatformO/PlatformInterface.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zSyncO/DialogBasedSyncListener.h>
#include <zSyncO/SyncLoginAccessor.h>
#include <zSyncO/SyncServiceFactory.h>


DeploymentPackageDownloader::DeploymentPackageDownloader(ApplicationPackageManager application_package_manager)
    :   m_applicationPackageManager(std::move(application_package_manager))
{
    m_syncClient = std::make_unique<SyncClient>(GetDeviceId(), std::make_unique<SyncServiceFactory>(std::make_unique<SyncLoginAccessor>()));
    m_syncClient->SetSyncListener(std::make_unique<DialogBasedSyncListener>(nullptr));
}


SyncClient::SyncResult DeploymentPackageDownloader::ConnectToServer(const SyncConnectionString& sync_connection_string)
{
    return m_syncClient->Connect(sync_connection_string);
}


void DeploymentPackageDownloader::Disconnect()
{
    m_syncClient->Disconnect();
}


SyncClient::SyncResult DeploymentPackageDownloader::List(std::vector<ApplicationPackage>& packages)
{
    SyncClient::SyncResult result = m_syncClient->ListApplicationPackages(packages);

    if( result == SyncClient::SyncResult::SYNC_OK )
    {
        // Add in installed versions of existing packages
        const std::vector<ApplicationPackage> installed_packages = m_applicationPackageManager.GetInstalledApplications();

        for( ApplicationPackage& server_package : packages )
        {
            const auto& installed_package = std::find_if(installed_packages.begin(), installed_packages.end(),
                                                         [&server_package](const auto& cp) { return ( server_package.GetName() == cp.GetName() ); });

            if( installed_package != installed_packages.end() )
                server_package.SetInstalledVersionBuildTime(installed_package->GetBuildTime());
        }
    }

    return result;
}


SyncClient::SyncResult DeploymentPackageDownloader::Install(const std::string& package_name, const bool force_full_update)
{
    return m_syncClient->DownloadApplicationPackage(m_applicationPackageManager, package_name, force_full_update);
}


SyncClient::SyncResult DeploymentPackageDownloader::ListUpdatable(std::vector<ApplicationPackage>& packages)
{
    const std::vector<ApplicationPackage> installed = m_applicationPackageManager.GetInstalledApplications();

    std::map<std::string, std::vector<ApplicationPackage>> apps_grouped_by_server;
    for (const ApplicationPackage& p : installed)
    {
        if (!p.GetServerUrl().empty()) {
            apps_grouped_by_server[p.GetServerUrl()].emplace_back(p);
        }
    }

    std::vector<ApplicationPackage> updatable;
    for (const auto& apps_for_server : apps_grouped_by_server) {
        auto result = ConnectToServer(apps_for_server.first);
        if (result == SyncClient::SyncResult::SYNC_CANCELED)
            return SyncClient::SyncResult::SYNC_CANCELED;

        if (result == SyncClient::SyncResult::SYNC_OK) {
            std::vector<ApplicationPackage> server_packages;
            result = m_syncClient->ListApplicationPackages(server_packages);
            m_syncClient->Disconnect();
            if (result == SyncClient::SyncResult::SYNC_CANCELED)
                return SyncClient::SyncResult::SYNC_CANCELED;

            const auto& client_packages = apps_for_server.second;
            for (const ApplicationPackage& client_package : client_packages) {
                auto server_package = std::find_if(server_packages.begin(), server_packages.end(),
                                                   [&client_package](const auto& sp) { return ( client_package.GetName() == sp.GetName() ); });
                if (server_package != server_packages.end() && server_package->GetBuildTime() > client_package.GetBuildTime()) {
                    server_package->SetInstalledVersionBuildTime(client_package.GetBuildTime());
                    server_package->SetDeploymentType(client_package.GetDeploymentType());
                    server_package->SetServerUrl(client_package.GetServerUrl());
                    updatable.emplace_back(*server_package);
                }
            }
        }
    }

    packages = updatable;

    return SyncClient::SyncResult::SYNC_OK;
}


SyncClient::SyncResult DeploymentPackageDownloader::Update(const std::string& package_name, const std::string& server_url)
{
    const SyncClient::SyncResult result = ConnectToServer(server_url);

    if( result != SyncClient::SyncResult::SYNC_OK )
        return result;

    return Install(package_name, false);
}
