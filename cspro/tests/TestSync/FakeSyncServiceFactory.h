#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ISyncServiceFactory.h>
#include <tests/TestSync/FakeSyncService.h>


// Factory that returns a FakeSyncService.

class FakeSyncServiceFactory : public ISyncServiceFactory
{
public:
    FakeSyncServiceFactory(const std::vector<std::shared_ptr<Case>>& responseCases)
        :   m_fakeSyncService(nullptr),
            m_responseCases(responseCases)
    {
    }

    FakeSyncService* getServer()
    {
        return m_fakeSyncService;
    }

    std::unique_ptr<ISyncService> createFakeSyncService()
    {
        auto fake_sync_service = std::make_unique<FakeSyncService>(m_responseCases);
        m_fakeSyncService = fake_sync_service.get();
        return fake_sync_service;
    }

    std::unique_ptr<ISyncService> CreateCSWebSyncService(SyncConnectionString /*sync_connection_string*/,
                                                         std::unique_ptr<LoginCredentials> /*login_credentials = nullptr*/) override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateBluetoothSyncService(BluetoothDeviceInfo /*device_info*/) override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateBluetoothSyncService() override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateDropboxSyncService(cs::cref_optional<SyncConnectionString> /*sync_connection_string*/) override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateDropboxLocalSyncService(std::string /*local_dropbox_directory*/) override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateFtpSyncService(SyncConnectionString /*sync_connection_string*/,
                                                       std::unique_ptr<LoginCredentials> /*login_credentials = nullptr*/) override
    {
        return createFakeSyncService();
    }

    std::unique_ptr<ISyncService> CreateLocalFileSyncService(const SyncConnectionString& /*sync_connection_string*/) override
    {
        return createFakeSyncService();
    }

private:
    FakeSyncService* m_fakeSyncService;
    std::vector<std::shared_ptr<Case>> m_responseCases;
};
