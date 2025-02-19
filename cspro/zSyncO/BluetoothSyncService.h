#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ISyncService.h>
#include <zSyncO/ObexConstants.h>
#include <zNetwork/HeaderList.h>
#include <zAppO/SyncTypes.h>

class BluetoothObexConnection;
class IBluetoothAdapter;


// Sync service for Bluetooth peer-to-peer.

class BluetoothSyncService : public ISyncService
{
public:
    BluetoothSyncService(std::shared_ptr<IBluetoothAdapter> pAdapter, BluetoothDeviceInfo device_info);
    BluetoothSyncService(std::shared_ptr<IBluetoothAdapter> pAdapter, std::shared_ptr<LoginAccessor> login_accessor);
    ~BluetoothSyncService();

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
    ObexResponseCode GetDataChunk(const CDataDict& dictionary, const DeviceId& device_id, const std::string& universe,
                                  const std::string& last_server_revision, const std::string& last_case_uuid,
                                  const std::vector<std::string>& excludedServerRevisions,
                                  HeaderList& responseHeaders, std::string& responseBody);

    ObexResponseCode getFileWorker(const CString& type, const CString& remotePath, const CString& localPath,
                                   std::optional<HeaderList> requestHeaders = std::nullopt);
    void putFileWorker(const CString& type, const CString& localPath, const CString& remotePath);

private:
    std::shared_ptr<IBluetoothAdapter> m_pAdapter;
    std::unique_ptr<BluetoothObexConnection> m_pObexConnection;
    BluetoothDeviceInfo m_deviceInfo;
    std::shared_ptr<LoginAccessor> m_loginAccessor;
    bool m_bWasBluetoothEnabled;
    std::shared_ptr<SyncListener> m_syncListener;
    double m_serverCSProVersion;
};
