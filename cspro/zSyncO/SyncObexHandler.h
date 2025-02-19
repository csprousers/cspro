#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IObexResource.h>
#include <zNetwork/HeaderList.h>
#include <zAppO/SyncTypes.h>

class ApplicationPackageManager;
class DataRepository;
namespace Paradata { class Syncer; }
class SyncMessage;


class ISyncObexEngineAccessor
{
public:
    virtual ~ISyncObexEngineAccessor() { }

    // returns the data repository associated with the dictionary name or null if not found
    virtual DataRepository* GetDataRepository(const std::string& syncable_dictionary_name, const std::string& dictionary_name) = 0;

    // creates an application package manager, or null if not able to
    virtual std::unique_ptr<ApplicationPackageManager> CreateApplicationPackageManager() = 0;

    // returns the response of the OnSyncMessage callback, or std::nullopt if none
    virtual std::optional<SharableString> OnSyncMessage(const SyncMessage& sync_message) = 0;
};



class SYNC_API SyncObexHandler
{
public:
    SyncObexHandler(DeviceId device_id, std::string root_directory, std::unique_ptr<ISyncObexEngineAccessor> sync_obex_engine_accessor);
    ~SyncObexHandler();

    ObexResponseCode onConnect(const char* target, int targetSizeBytes);

    ObexResponseCode onDisconnect();

    ObexResponseCode onGet(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);

    ObexResponseCode onPut(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);

private:
    DataRepository* FindDataRepository(const std::string& syncable_dictionary_name, const HeaderList& request_headers) const;

    ObexResponseCode handleSyncPut(const std::string& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncGet(const std::string& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleDirectoryListing(const std::string& path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleFileGet(CString path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleFilePut(CString path, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncApp(const std::string& package_name, const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncMessage(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);

    ObexResponseCode handleSyncParadataStart(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataPut(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataGet(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataStop(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);

private:
    DeviceId m_deviceId;
    std::string m_rootDirectory;
    std::unique_ptr<ISyncObexEngineAccessor> m_syncObexEngineAccessor;
    std::unique_ptr<Paradata::Syncer> m_paradataSyncer;
};
