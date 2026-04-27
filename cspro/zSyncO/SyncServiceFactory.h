#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncServiceFactory.h>


// Factory class for creating synchronization services.

class SYNC_API SyncServiceFactory : public ISyncServiceFactory
{
public:
    // If login_accessor is null, it will be set to an instance of SyncLoginAccessor.
    SyncServiceFactory(std::shared_ptr<LoginAccessor> login_accessor = nullptr);

    std::unique_ptr<ISyncService> CreateCSWebSyncService(SyncConnectionString sync_connection_string,
                                                         std::unique_ptr<LoginCredentials> login_credentials = nullptr) override;

    std::unique_ptr<ISyncService> CreateBluetoothSyncService(BluetoothDeviceInfo device_info) override;
    std::unique_ptr<ISyncService> CreateBluetoothSyncService() override;

    std::unique_ptr<ISyncService> CreateDropboxSyncService(cs::cref_optional<SyncConnectionString> sync_connection_string) override;
    std::unique_ptr<ISyncService> CreateDropboxLocalSyncService(std::string local_dropbox_directory) override;

    std::unique_ptr<ISyncService> CreateFtpSyncService(SyncConnectionString sync_connection_string,
                                                       std::unique_ptr<LoginCredentials> login_credentials = nullptr) override;

    std::unique_ptr<ISyncService> CreateLocalFileSyncService(const SyncConnectionString& sync_connection_string) override;

private:
    std::shared_ptr<LoginAccessor> m_loginAccessor;
};
