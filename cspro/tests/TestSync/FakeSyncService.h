#pragma once

#include <zUtilO/TemporaryFile.h>
#include <zNetwork/ConnectResponse.h>
#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/ISyncService.h>
#include <zSyncO/NetworkDataChunk.h>
#include <zSyncO/SyncDictionaryInfo.h>


// Fake sync service that always give the same response.

class FakeSyncService : public ISyncService
{
public:
    FakeSyncService()
    {
    }

    FakeSyncService(const std::vector<std::shared_ptr<Case>>& responseCases)
        :   m_responseCases(responseCases)
    {
    }

    size_t GetNumberOfCasesInLastPutCases() const
    {
        return m_numberOfCasesInLastPutCases;
    }

    // Connect to remote sync service
    std::shared_ptr<ConnectResponse> Connect() override
    {
        return std::make_unique<ConnectResponse>("myserver");
    }

    // Disconnect from remote sync service
    void Disconnect() override
    {
    }

    // Send request to download cases from sync service
    // Throws SyncException.
    SyncGetResponse GetCases(std::shared_ptr<const CaseAccess> /*case_access*/,
                             const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                             const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) override
    {
        m_lastServerRevision = last_server_revision;

        return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete,
                               std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(m_responseCases)),
                               m_responseServerRevision);
    }

    // Send request to upload cases to sync service
    // Throws SyncException.
    SyncPutResponse PutCases(std::shared_ptr<const CaseAccess> /*case_access*/,
                             const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                             const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) override
    {
        m_lastServerRevision = last_server_revision;
        m_numberOfCasesInLastPutCases = cases.size();

        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, m_responseServerRevision);
    }

    IDataChunk& GetChunk() override
    {
        return m_dataChunk;
    }

    std::vector<SyncDictionaryInfo> GetDictionaries() override
    {
        return { };
    }

    std::string GetDictionary(const std::string& /*dictionary_name*/) override
    {
        return std::string();
    }

    void PutDictionary(const CDataDict& /*dictionary*/) override
    {
    }

    void DeleteDictionary(const std::string& /*dictionary_name*/) override
    {
    }

    bool GetFile(const std::string& /*remote_file_path*/, const std::string& /*local_file_path*/, const std::string& /*existing_file_md5 = std::string()*/) override
    {
        return true;
    }

    std::unique_ptr<TemporaryFile> GetFileIfExists(const std::string& /*remote_file_path*/) override
    {
        return nullptr;
    }

    void PutFile(const std::string& /*local_file_path*/, const std::string& /*remote_path*/) override
    {
    }

    std::vector<FileInfo> GetDirectoryListing(const std::string& /*remote_directory_path*/, bool /*request_file_md5s*/) override
    {
        return { };
    }

    std::vector<ApplicationPackage> ListApplicationPackages() override
    {
        return std::vector<ApplicationPackage>();
    }

    bool DownloadApplicationPackage(const std::string& /*package_name*/, const std::string& /*local_file_path*/,
                                    const ApplicationPackage* /*current_package*/, const std::string* /*current_package_signature*/) override
    {
        return false;
    }

    void UploadApplicationPackage(const std::string& /*local_file_path*/, const std::string& /*package_name*/, const std::string& /*package_spec_json*/) override
    {
    }

    void DeleteApplication(const std::string& /*package_name*/) override
    {
    }

    std::optional<JsonNode> SendSyncMessage(const DeviceId& /*device_id*/, const SyncMessage& /*sync_message*/) override
    {
        return std::nullopt;
    }

    std::string StartParadataSync(const std::string& /*log_uuid*/) override
    {
        return std::string();
    }

    void PutParadata(const std::string& /*paradata_log_file_path*/) override
    {
    }

    std::vector<TemporaryFile> GetParadata() override
    {
        return { };
    }

    void StopParadataSync() override
    {
    }

    std::shared_ptr<SyncListener> GetSharedSyncListener() override
    {
        return nullptr;
    }

    void SetSyncListener(std::shared_ptr<SyncListener> /*sync_listener*/) override
    {
    }

    std::shared_ptr<FileBasedConnection> GetFileBasedConnection() override
    {
        return nullptr;
    }

 private:
    NetworkDataChunk m_dataChunk;

    const std::string m_responseServerRevision = "1";

    std::string m_lastServerRevision;
    size_t m_numberOfCasesInLastPutCases = 0;
    std::vector<std::shared_ptr<Case>> m_responseCases;
};
