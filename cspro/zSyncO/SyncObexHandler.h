#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IObexResource.h>
#include <zNetwork/HeaderList.h>
#include <zAppO/SyncTypes.h>

struct IDataRepositoryRetriever;
namespace Paradata { class Syncer; }


struct ISyncEngineFunctionCaller
{
    virtual ~ISyncEngineFunctionCaller() { }

    virtual std::optional<CString> onSyncMessage(const CString& message_key, const CString& message_value) = 0;
};



class SYNC_API SyncObexHandler
{
public:
    SyncObexHandler(DeviceId deviceId, IDataRepositoryRetriever* pRepoRetriever, CString rootPath, ISyncEngineFunctionCaller* sync_engine_function_caller);
    ~SyncObexHandler();

    ObexResponseCode onConnect(const char* target, int targetSizeBytes);

    ObexResponseCode onDisconnect();

    ObexResponseCode onGet(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);

    ObexResponseCode onPut(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);

private:
    ObexResponseCode handleSyncPut(const std::wstring& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncGet(const std::wstring& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleDirectoryListing(CString path, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleFileGet(CString path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleFilePut(CString path, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncApp(const CString& app_name, const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncMessage(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);

    ObexResponseCode handleSyncParadataStart(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataPut(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataGet(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);
    ObexResponseCode handleSyncParadataStop(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource);

private:
    DeviceId m_deviceId;
    IDataRepositoryRetriever* m_pDataRepositoryRetriever;
    CString m_rootPath;
    ISyncEngineFunctionCaller* m_syncEngineFunctionCaller;
    std::unique_ptr<Paradata::Syncer> m_paradataSyncer;
};
