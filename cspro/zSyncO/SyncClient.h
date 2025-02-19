#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncService.h>
#include <zSyncO/SyncRunner.h>
#include <zUtilO/SyncConnectionString.h>
#include <zNetwork/LoginCredentials.h>

class ApplicationPackageManager;
class ISyncableDataRepository;
class ISyncServiceFactory;
class SyncHistoryEntry;


// Main client side interface to smart sync.
// Handles all communication with sync services as well as "smarts" about which cases to sync.

class SYNC_API SyncClient
{
public:
    ///<summary>Create a new sync client instance</summary>
    ///<param name="device_id">device id of client</param>
    ///<param name="sync_service_factory">factory for creating sync services</param>
    SyncClient(DeviceId device_id, std::unique_ptr<ISyncServiceFactory> sync_service_factory);

    // Create a SyncClient from an already-connected sync service.
    SyncClient(std::tuple<std::shared_ptr<ISyncService>, std::shared_ptr<ConnectResponse>> sync_runner_connection);

    SyncClient(const SyncClient&) = delete;
    SyncClient(SyncClient&&);

    ~SyncClient();

    enum class SyncResult
    {
        SYNC_OK = 1,
        SYNC_ERROR = 0,
        SYNC_CANCELED = -1
    };

    // Connect to sync service using sync connection string.
    SyncResult Connect(const SyncConnectionString& sync_connection_string);

    // Connect to CSWeb using URL and either username/password or a saved OAuth token.
    SyncResult ConnectCSWeb(const SyncConnectionString& sync_connection_string, std::unique_ptr<LoginCredentials> login_credentials = nullptr);

    // Connect to peer device for P2P sync
    // If the device address is known specify it, otherwise it will obtained from name. Specifying the address will be faster.
    SyncResult ConnectBluetooth(const SyncConnectionString& sync_connection_string);

    // Connect to Dropbox.
    // Based on the parameters in the sync connection string, this connection may be a local machine connection.
    SyncResult ConnectDropbox(cs::cref_optional<SyncConnectionString> sync_connection_string = std::nullopt);

    // Connect to FTP server using URL and either username/password or saved details in the credential store.
    SyncResult ConnectFtp(const SyncConnectionString& sync_connection_string, std::unique_ptr<LoginCredentials> login_credentials = nullptr);

    // Connect to local files.
    SyncResult ConnectLocalFiles(const SyncConnectionString& sync_connection_string);

    SyncResult Disconnect();

    bool IsConnected() const { return ( m_syncService != nullptr ); }

    ISyncService* GetSyncService() { return m_syncService.get(); }

    // Get device id of connected sync service.
    const DeviceId& GetServerDeviceId() const;

    // Sync data file using smart sync.
    SyncResult SyncData(SyncDirection direction, ISyncableDataRepository& repository, const std::string& universe);

    ///<summary>Sync non-data file. If to_path ends in a slash, it is assumed to be a directory and the filename will be appended to it.</summary>
    ///<param name="direction">direction of sync (PUT or GET)</param>
    ///<param name="from_path">path of source file (on sync service for get or on client for put)</param>
    ///<param name="to_path">path of destination file (on client for get or on sync service for put)</param>
    SyncResult SyncFile(SyncDirection direction, std::string from_path, std::string to_path);

    ///<summary>Download a list of dictionaries on the sync service that can be used with syncData</summary>
    ///<param name="dictionaries">List of dictionaries returned. Each dictionary is a pair of name and label.</param>
    SyncResult GetDictionaries(std::vector<SyncDictionaryInfo>& dictionaries);

    ///<summary>Download a dictionary from the sync service. Retrieves full text of the the dcf file for the dictionary.</summary>
    ///<param name="dictionary_name">Name of dictionary.</param>
    ///<param name="dictionary_text">Dictionary contents as text (dcf format) returned.</param>
    SyncResult DownloadDictionary(const std::string& dictionary_name, std::string& dictionary_text);

    ///<summary>Upload a dictionary to sync service to be used with syncData</summary>
    ///<param name="dictionary_file_path">Path of dictionary file to upload</param>
    SyncResult UploadDictionary(const std::string& dictionary_file_path);

    ///<summary>Delete a dictionary on the sync service</summary>
    ///<param name="dictionary_name">Name of dictionary to delete</param>
    SyncResult DeleteDictionary(const std::string& dictionary_name);

    /// <summary>List all application deployment packages available on the sync service</summary>
    /// <param name="packages">List of packages returned</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult ListApplicationPackages(std::vector<ApplicationPackage>& packages);

    /// <summary>Download an application deployment package from the sync service and install it</summary>
    /// <param name="application_package_manager">Instance of an application package manager</param>
    /// <param name="package_name">Name of package to download and install</param>
    /// <param name="force_full_install">Skip smart update and download entire package even if the installed package is up to date</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult DownloadApplicationPackage(const ApplicationPackageManager& application_package_manager, const std::string& package_name, bool force_full_install);

    /// <summary>Upload an application deployment package to the sync service</summary>
    /// <param name="local_package_zip_file_path">Source path to package file on local device</param>
    /// <param name="package_name">Name of package to upload</param>
    /// <param name="package_spec_json">Package spec file in JSON</param>
    /// <param name="directory_for_dictionary_upload_evaluation">The directory for evaluating dictionary paths; if empty, dictionaries are not uploaded</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name,
                                        const std::string& package_spec_json, const std::string& directory_for_dictionary_upload_evaluation);

    /// <summary>Download an deployment package for currently running application and update it</summary>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult UpdateApplication(const ApplicationPackageManager& application_package_manager, const std::string& application_file_path);

    ///<summary>Delete an application on the sync service</summary>
    ///<param name="package_name">Name of package to delete</param>
    SyncResult DeleteApplication(const std::string& package_name);

    std::optional<JsonNode> SendSyncMessage(const SyncMessage& sync_message);

    SyncResult SyncParadata(SyncDirection sync_direction);

    // Set callbacks for reporting errors and progress
    SyncListener* GetSyncListener()                                   { return m_syncListener.get(); }
    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) { m_syncListener = std::move(sync_listener); }

private:
    SyncResult ConnectWorker(const SyncConnectionString& sync_connection_string);
    SyncResult ConnectWithParadataSupport(const SyncConnectionString& sync_connection_string);

    SyncResult ConnectToSyncService(std::unique_ptr<ISyncService> sync_service, const std::string& sync_service_name);

    SyncResult DisconnectFromSyncService();

    void SyncDataGet(ISyncableDataRepository& repository, const std::string& universe);
    void SyncDataPut(ISyncableDataRepository& repository, const std::string& universe);

    void UploadDictionaryWorker(const std::string& dictionary_file_path);

    std::optional<SyncHistoryEntry> GetRevisionFromLastSync(SyncDirection direction, ISyncableDataRepository& repository, const std::string& universe) const;
    static std::vector<std::string> GetPutRevisionsSince(const DeviceId& device_id, ISyncableDataRepository& repository, const int start_serial_number);

    SyncResult GetFilesWithWildcard(const std::string& from_path, const std::string& to_path);
    SyncResult GetFile(const std::string& from_path, cs::cref_optional<std::string> to_path);
    void DownloadOneFile(const std::string& from_path, const std::string& to_path, const std::string& existing_file_md5);
    SyncResult PutFilesWithWildcard(const std::string& from_path, cs::cref_optional<std::string> to_path);
    SyncResult PutFile(const std::string& from_path, const std::string& to_path);
    void UploadOneFile(const std::string& from_path, cs::cref_optional<std::string> to_path);

    void DownloadAndInstallPackage(const ApplicationPackageManager& application_package_manager, const std::string& package_name,
                                   const ApplicationPackage* current_package, const std::string* current_package_signature);

private:
    DeviceId m_deviceId;
    std::unique_ptr<ISyncServiceFactory> m_syncServiceFactory;
    std::shared_ptr<SyncListener> m_syncListener;
    std::shared_ptr<ISyncService> m_syncService;
    std::shared_ptr<ConnectResponse> m_connectResponse;
    std::unique_ptr<SyncRunner::ParadataLogger> m_paradataLogger;
};
