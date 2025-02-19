#include "stdafx.h"
#include "SyncServiceFactory.h"
#include "BluetoothObexConnection.h"
#include "BluetoothSyncService.h"
#include "CSWebSyncService.h"
#include "DropboxLocalSyncService.h"
#include "DropboxSyncService.h"
#include "FtpSyncService.h"
#include "LocalFileSyncService.h"
#include <zNetwork/DropboxConnection.h>
#include <zNetwork/FtpConnection.h>
#include <zNetwork/HttpConnection.h>


SyncServiceFactory::SyncServiceFactory(std::shared_ptr<LoginAccessor> login_accessor)
    :   m_loginAccessor(std::move(login_accessor))
{
    ASSERT(m_loginAccessor != nullptr);
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateCSWebSyncService(SyncConnectionString sync_connection_string,
                                                                         std::unique_ptr<LoginCredentials> login_credentials/* = nullptr*/)
{
    return std::make_unique<CSWebSyncService>(m_loginAccessor->CreateHttpConnection(),
                                              std::move(sync_connection_string),
                                              ( login_credentials != nullptr ) ? std::move(*login_credentials) : LoginCredentials(m_loginAccessor));
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateBluetoothSyncService(BluetoothDeviceInfo device_info)
{
    return std::make_unique<BluetoothSyncService>(m_loginAccessor->GetBluetoothAdapter(), std::move(device_info));
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateBluetoothSyncService()
{
    return std::make_unique<BluetoothSyncService>(m_loginAccessor->GetBluetoothAdapter(), m_loginAccessor);
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateDropboxSyncService(cs::cref_optional<SyncConnectionString> sync_connection_string)
{
    return std::make_unique<DropboxSyncService>(std::make_unique<DropboxConnection>(m_loginAccessor, std::move(sync_connection_string)));
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateDropboxLocalSyncService(std::string local_dropbox_directory)
{
    return std::make_unique<DropboxLocalSyncService>(std::move(local_dropbox_directory));
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateFtpSyncService(SyncConnectionString sync_connection_string,
                                                                       std::unique_ptr<LoginCredentials> login_credentials/* = nullptr*/)
{
    return std::make_unique<FtpSyncService>(m_loginAccessor->CreateFtpConnection(),
                                            std::move(sync_connection_string),
                                            ( login_credentials != nullptr ) ? std::move(*login_credentials) : LoginCredentials(m_loginAccessor));
}


std::unique_ptr<ISyncService> SyncServiceFactory::CreateLocalFileSyncService(const SyncConnectionString& sync_connection_string)
{
    return std::make_unique<LocalFileSyncService>(sync_connection_string);
}
