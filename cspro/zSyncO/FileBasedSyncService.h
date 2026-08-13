#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncService.h>

class FileBasedParadataSyncer;
class NetworkDataChunk;
class PFF;


// --------------------------------------------------------------------------
// FileBasedSyncService
//
// Subclass of ISyncService that implements file-based data syncs.
//
// These are shared routines for doing smart data sync using only file
// operations using a sync service subclass such as DropboxSyncService or
// FtpSyncService.
//
// File-based smart sync stores JSON data in the same format used to sync to
// CSWeb in files on the server. Each time cases are uploaded to the server
// from the client, all new/modified cases since the last sync are uploaded
// in a new file. Past sync files that have already been uploaded to the
// server are never modified or overwritten. So for every sync PUT from a
// client, a new file is created on the server. This means that cases that
// are updated multiple times will be duplicated on the server but when
// downloaded they will be merged correctly based on the UUID and vector
// clock.
//
// Files are stored in the directory:
//     - CSPro/DataSync/<dictionary_name>/data
//
// A copy of the dictionary is stored in:
//     - CSPro/DataSync/<dictionary_name>/dict/dictionary.dcf
//
// Files are named <deviceid>$<random UUID>
//
// The random UUID is added to avoid overwriting an existing file.
//
// When doing a GET, the client first does a directory listing and then
// only downloads files with modified dates more recent than that of
// the last sync.
// --------------------------------------------------------------------------

class SYNC_API FileBasedSyncService : public ISyncService
{
protected:
    FileBasedSyncService(std::string sync_service_description, std::shared_ptr<FileBasedConnection> file_based_connection, bool is_connection_network_based);

public:
    ~FileBasedSyncService();

    // ISyncService overrides

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

protected:
    // Set parameters in PFF to download from this sync service.
    virtual void SetDownloadPffSyncServiceParams(PFF& pff) = 0;

private:
    // Calls Download to read the file, returning the contents as a string.
    std::string FileReadText(const std::string& remote_file_path);

    // Calls Upload to write a file from the contents of a string.
    template<typename T>
    void FileWriteText(const std::string& remote_file_path, T&& text);

    std::vector<std::shared_ptr<Case>> GetCasesFromZipFile(const std::string& dictionary_name, const std::string& universe, SyncCaseSerializer& sync_case_serializer,
                                                           const std::string& case_data, const std::string& zip_filename);
    std::vector<std::shared_ptr<Case>> GetCasesFromV2CaseData(std::shared_ptr<const CaseAccess> case_access, const std::string& universe, std::string& case_data);

    static void FilterCasesByUniverse(std::vector<std::shared_ptr<Case>>& cases, const std::string& universe);

    // Uploads the binary data and returns a JSON string containing metadata of the uploaded data.
    std::string UploadCaseBinaryData(const SyncBinaryDataUploadManager& sync_binary_data_upload_manager, const std::string& dictionary_name);

    void UploadDictionaryWorker(const CDataDict& dictionary, bool upload_even_if_exists);
    void UploadDictionaryIfNeeded(const CDataDict& dictionary);

    static std::string GetDictionaryDirectory(const std::string& dictionary_name);
    static std::string GetDictionaryFilePath(const std::string& dictionary_name);
    static std::string GetDataDirectory(const std::string& dictionary_name);
    static std::string GetDataBinaryDataDirectory(const std::string& dictionary_name);
    static std::string GetDataFilePath(const std::string& data_path, const DeviceId& device_id, const std::string& server_revision);
    static const char* GetAppsDirectory();
    static std::string GetFilePathFromPackageName(const std::string& package_name, std::string* package_spec_file_path);

    // Parses the data filename, returning the device ID and server revision. The device ID will be empty on error.
    static std::tuple<DeviceId, std::string> ParseDataFilename(const std::string& filename);

protected:
    std::shared_ptr<SyncListener> m_syncListener;

private:
    std::shared_ptr<FileBasedConnection> m_fileBasedConnection; // non-null
    std::string m_syncServiceDescription;
    std::shared_ptr<NetworkDataChunk> m_dataChunk;
    std::set<std::string> m_dictionariesUploaded;
    std::unique_ptr<std::tuple<int64_t, size_t>> m_syncMessageCounter; // timestamp, messages at this timestamp
    std::unique_ptr<FileBasedParadataSyncer> m_paradataSyncer;
};
