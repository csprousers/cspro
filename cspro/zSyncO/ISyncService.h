#pragma once

#include <zSyncO/SyncGetResponse.h>
#include <zSyncO/SyncPutResponse.h>
#include <zToolsO/span.h>
#include <zAppO/SyncTypes.h>

class ApplicationPackage;
class Case;
class CaseAccess;
class CDataDict;
class ConnectResponse;
class FileBasedConnection;
class FileInfo;
class IDataChunk;
class ISyncableDataRepository;
class SyncBinaryDataUploadManager;
class SyncDictionaryInfo;
class SyncListener;
class SyncMessage;
class TemporaryFile;


// --------------------------------------------------------------------------
// ISyncService
//
// Interface to a synchronization service.
// Subclasses support different sync service types such as HTTP, Dropbox, FTP,
// Bluetooth peer-to-peer, etc.
// --------------------------------------------------------------------------

class ISyncService
{
public:
    virtual ~ISyncService() { }

    /// <summary>Connect to sync service.</summary>
    /// <exception cref="SyncException"></exception>
    virtual std::shared_ptr<ConnectResponse> Connect() = 0;

    /// <summary>Disconnect from sync service.</summary>
    /// <exception cref="SyncException"></exception>
    virtual void Disconnect() = 0;


    // --------------------------------------------------------------------------
    // data functionality
    // --------------------------------------------------------------------------

    // Download cases from the sync service and sync them to a repository.
    // Throws: SyncException.
    virtual SyncGetResponse GetCases(std::shared_ptr<const CaseAccess> case_access,
                                     const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                     const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions) = 0;


    // Upload cases from a repository to the sync service.
    // case_access, and the case objects in cases, will be non-null.
    // If sync_binary_data_manager is non-null, it means that the case can have binary data.
    // Throws: SyncException + DataRepositoryException::Error.
    virtual SyncPutResponse PutCases(std::shared_ptr<const CaseAccess> case_access,
                                     cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* sync_binary_data_upload_manager,
                                     const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision) = 0;

    // Get mutable chunk object.
    virtual IDataChunk& GetChunk() = 0;

    ///<summary>Download a list of dictionaries on the sync service that can be used with syncData</summary>
    ///<param name="dictionaries">List of dictionaries returned. Each dictionary is a pair of name and label.</param>
    virtual std::vector<SyncDictionaryInfo> GetDictionaries() = 0;

    /// <summary>Download a dictionary from the sync service</summary>
    /// <param name="dictionary_name">Name of dictionary to download</param>
    /// <returns>Dictionary as text (dcf) format.</returns>
    /// <exception cref="SyncException"></exception>
    virtual std::string GetDictionary(const std::string& dictionary_name) = 0;

    /// <summary>Upload a dictionary to the sync service</summary>
    /// <param name="dictionary">Dictionary</param>
    /// <exception cref="SyncException"></exception>
    virtual void PutDictionary(const CDataDict& dictionary) = 0;

    /// <summary>Delete a dictionary on the sync service</summary>
    /// <param name="dictionary_name">Name of dictionary to delete</param>
    /// <exception cref="SyncException"></exception>
    virtual void DeleteDictionary(const std::string& dictionary_name) = 0;


    // --------------------------------------------------------------------------
    // file functionality
    // --------------------------------------------------------------------------

    /// <summary>Download a file</summary>
    /// <param name="remote_file_path">Full path on sync service to download</param>
    /// <param name="local_file_path">Full path on local machine to write file contents to</param>
    /// <param name="existing_file_md5">Optional MD5 of existing file to check if remote has same version to avoid re-download</param>
    /// <returns>True if file was downloaded or false if no downloaded needed due to etag</returns>
    /// <exception cref="SyncException">On error or cancel</exception>
    virtual bool GetFile(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5) = 0;

    /// <summary>Download a file if it exists on the sync service</summary>
    /// <param name="remote_file_path">Full path on sync service to download</param>
    /// <returns>A temporary file if a file was downloaded or null if the file does not exist on the sync service</returns>
    /// <exception cref="SyncException">On error or cancel</exception>
    virtual std::unique_ptr<TemporaryFile> GetFileIfExists(const std::string& remote_file_path) = 0;

    /// <summary>Upload a file</summary>
    /// <param name="local_file_path">Full path to file on local device</param>
    /// <param name="remote_path">Destination path for upload</param>
    /// <exception cref="SyncException"></exception>
    virtual void PutFile(const std::string& local_file_path, const std::string& remote_path) = 0;

    /// <summary>Get remote directory listing</summary>
    /// <exception cref="SyncException"></exception>
    virtual std::vector<FileInfo> GetDirectoryListing(const std::string& remote_path, bool request_file_md5s) = 0;


    // --------------------------------------------------------------------------
    // application functionality
    // --------------------------------------------------------------------------

    /// <summary>List all application deployment packages available on the sync service</summary>
    /// <returns>List of packages from sync service</returns>
    /// <exception cref="SyncException"></exception>
    virtual std::vector<ApplicationPackage> ListApplicationPackages() = 0;

    /// <summary>Download an application deployment package from the sync service</summary>
    /// <param name="package_name">Name of package to download</param>
    /// <param name="local_file_path">Destination path to save package file to on local device</param>
    /// <param name="current_package">Current package spec or null if package not already installed</param>
    /// <exception cref="SyncException"></exception>
    /// <returns>True if file was downloaded or false if no downloaded needed because sync service version is same</returns>
    virtual bool DownloadApplicationPackage(const std::string& package_name, const std::string& local_file_path,
                                            const ApplicationPackage* current_package, const std::string* current_package_signature) = 0;

    /// <summary>Upload an application deployment package to the sync service</summary>
    /// <param name="local_package_zip_file_path">Source path to package file on local device</param>
    /// <param name="package_name">Name of package to upload</param>
    /// <param name="package_spec_json">Package spec file in JSON</param>
    virtual void UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name, const std::string& package_spec_json) = 0;

    ///<summary>Delete an application on the sync service</summary>
    ///<param name="package_name">Name of package to delete</param>
    virtual void DeleteApplication(const std::string& package_name) = 0;


    // --------------------------------------------------------------------------
    // message functionality
    // --------------------------------------------------------------------------

    /// <summary>Send a message to the sync service</summary>
    /// <exception cref="SyncException"></exception>
    virtual std::optional<JsonNode> SendSyncMessage(const DeviceId& device_id, const SyncMessage& sync_message) = 0;


    // --------------------------------------------------------------------------
    // paradata functionality
    // --------------------------------------------------------------------------

    // Sends the paradata log UUID to the sync service and gets the sync service's log UUID.
    virtual std::string StartParadataSync(const std::string& log_uuid) = 0;

    // Sends the paradata log to the sync service.
    virtual void PutParadata(const std::string& paradata_log_file_path) = 0;

    // Receives paradata logs from the sync service.
    virtual std::vector<TemporaryFile> GetParadata() = 0;

    // Informs the sync service that the paradata sync was successful.
    virtual void StopParadataSync() = 0;


    // --------------------------------------------------------------------------
    // other functionality
    // --------------------------------------------------------------------------

    // Gets or sets a sync listener.
    virtual std::shared_ptr<SyncListener> GetSharedSyncListener() = 0;
    virtual void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) = 0;

    // Gets the FileBasedConnection connected with this sync service (or null if not applicable).
    virtual std::shared_ptr<FileBasedConnection> GetFileBasedConnection() = 0;
};
