#pragma once

struct BluetoothDeviceInfo;
class ISyncService;
class LoginAccessor;
class LoginCredentials;
class SyncConnectionString;


// Interface for a factory class for creating sync services.

class ISyncServiceFactory
{
public:
    virtual ~ISyncServiceFactory() { }

    virtual std::unique_ptr<ISyncService> CreateCSWebSyncService(SyncConnectionString sync_connection_string,
                                                                 std::unique_ptr<LoginCredentials> login_credentials = nullptr) = 0;

    virtual std::unique_ptr<ISyncService> CreateBluetoothSyncService(BluetoothDeviceInfo device_info) = 0;
    virtual std::unique_ptr<ISyncService> CreateBluetoothSyncService() = 0;

    virtual std::unique_ptr<ISyncService> CreateDropboxSyncService(cs::cref_optional<SyncConnectionString> sync_connection_string) = 0;
    virtual std::unique_ptr<ISyncService> CreateDropboxLocalSyncService(std::string local_dropbox_directory) = 0;

    virtual std::unique_ptr<ISyncService> CreateFtpSyncService(SyncConnectionString sync_connection_string,
                                                               std::unique_ptr<LoginCredentials> login_credentials = nullptr) = 0;

    virtual std::unique_ptr<ISyncService> CreateLocalFileSyncService(const SyncConnectionString& sync_connection_string) = 0;
};
