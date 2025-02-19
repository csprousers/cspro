#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncService.h>

class CSWebConnection;
class FileBasedParadataSyncer;
class HeaderList;
class HttpConnection;
struct HttpResponse;
class LoginCredentials;
class NetworkDataChunk;
class SyncCaseSerializer;


// Sync service for CSWeb.

class SYNC_API CSWebSyncService : public ISyncService
{
protected:
    CSWebSyncService(std::unique_ptr<CSWebConnection> csweb_connection);

public:
    CSWebSyncService(std::unique_ptr<HttpConnection> http_connection,
                     SyncConnectionString sync_connection_string, LoginCredentials login_credentials);
    ~CSWebSyncService();

    CSWebConnection& GetCSWebConnection()                       { return *m_cswebConnection; }
    std::shared_ptr<CSWebConnection> GetSharedCSWebConnection() { return m_cswebConnection; }

    // ISyncService overrides
    std::shared_ptr<ConnectResponse> Connect() override;
    void Disconnect() override;

    // --------------------------------------------------------------------------
    // data functionality
    // --------------------------------------------------------------------------
    SyncGetResponse GetCases(std::shared_ptr<const CaseAccess> case_access,
                             const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                             const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions) override;

    SyncPutResponse PutCases(std::shared_ptr<const CaseAccess> case_access,
                             cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* sync_binary_data_upload_manager,
                             const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision) override;


    IDataChunk& GetChunk() override;

    std::vector<SyncDictionaryInfo> GetDictionaries() override;
    std::string GetDictionary(const std::string& dictionary_name) override;
    void PutDictionary(const CDataDict& dictionary) override;
    void DeleteDictionary(const std::string& dictionary_name) override;

    // --------------------------------------------------------------------------
    // file functionality
    // --------------------------------------------------------------------------
    bool GetFile(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5) override;
    std::unique_ptr<TemporaryFile> GetFileIfExists(const std::string& remote_file_path) override;
    void PutFile(const std::string& local_file_path, const std::string& remote_path) override;
    std::vector<FileInfo> GetDirectoryListing(const std::string& remote_path, bool request_file_md5s) override;

    // --------------------------------------------------------------------------
    // application functionality
    // --------------------------------------------------------------------------
    std::vector<ApplicationPackage> ListApplicationPackages() override;
    bool DownloadApplicationPackage(const std::string& package_name, const std::string& local_file_path,
                                    const ApplicationPackage* current_package, const std::string* current_package_signature) override;
    void UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name, const std::string& package_spec_json) override;
    void DeleteApplication(const std::string& package_name) override;

    // --------------------------------------------------------------------------
    // message functionality
    // --------------------------------------------------------------------------
    std::optional<JsonNode> SendSyncMessage(const DeviceId& device_id, const SyncMessage& sync_message) override;

    // --------------------------------------------------------------------------
    // paradata functionality
    // --------------------------------------------------------------------------
    std::string StartParadataSync(const std::string& log_uuid) override;
    void PutParadata(const std::string& paradata_log_file_path) override;
    std::vector<TemporaryFile> GetParadata() override;
    void StopParadataSync() override;

    // --------------------------------------------------------------------------
    // other functionality
    // --------------------------------------------------------------------------
    std::shared_ptr<SyncListener> GetSharedSyncListener() override;
    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) override;
    std::shared_ptr<FileBasedConnection> GetFileBasedConnection() override;

private:
    static std::string GetHeaderFallbackToEtag(const HeaderList& response_headers, std::string_view header_sv);

    // extracts chunk max revision from response headers
    static std::string GetChunkMaxRevision(const HeaderList& response_headers);

    // extracts server revision from response headers
    static std::string GetServerRevision(const HeaderList& response_headers);

    SyncGetResponse DownloadServerCases(const std::string& url, std::shared_ptr<const CaseAccess> case_access,
                                        const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                        const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions);

    SyncPutResponse UploadClientCases(const std::string& url, const DeviceId& device_id, const std::string& last_server_revision,
                                      const std::string& case_data, size_t num_cases);

    int SendUploadClientCasesRequest(const std::string& url, const std::string& client_cases_json, const DeviceId& device_id, const std::string& last_server_revision,
                                     HeaderList& response_headers, std::string& response_body);

    HttpResponse SendDownloadServerCasesRequest(const std::string& url, const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                                const std::string& last_case_uuid, const std::vector<std::string>& exclude_revisions);

private:
    std::shared_ptr<CSWebConnection> m_cswebConnection;
    std::shared_ptr<NetworkDataChunk> m_dataChunk;
    std::unique_ptr<FileBasedParadataSyncer> m_paradataSyncer;
};
