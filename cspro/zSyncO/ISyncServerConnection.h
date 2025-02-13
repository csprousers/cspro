#pragma once

#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/DictionaryInfo.h>
#include <zSyncO/SyncGetResponse.h>
#include <zSyncO/SyncPutResponse.h>
#include <zNetwork/FileInfo.h>
#include <ostream>
#include <istream>

class ConnectResponse;
struct IDataChunk;
class ISyncableDataRepository;
struct ISyncListener;
class PFF;
class SyncRequest;
class TemporaryFile;


/// <summary>Interface to sync server</summary>
// Derived classes support different server types such as http, bluetooth
// p2p, ftp...
struct ISyncServerConnection
{
    /// <summary>Connect to remote server</summary>
    /// <exception cref="SyncException"></exception>
    virtual ConnectResponse* connect() = 0;

    /// <summary>Disconnect from remote server</summary>
    /// <exception cref="SyncException"></exception>
    virtual void disconnect() = 0;

    /// <summary>Download cases from server and sync them to repo</summary>
    /// <exception cref="SyncException"></exception>
    virtual SyncGetResponse getData(const SyncRequest& request) = 0;

    /// <summary>Upload cases from repo to server</summary>
    /// <exception cref="SyncException"></exception>
    virtual SyncPutResponse putData(const SyncRequest& request) = 0;

    /// <summary>Get mutable chunk object.</summary>
    virtual IDataChunk& getChunk() = 0;

    ///<summary>Download a list of dictionaries on the server that can be used with syncData</summary>
    ///<param name="dictionaries">List of dictionaries returned. Each dictionary is a pair of name and label.</param>
    virtual std::vector<DictionaryInfo> getDictionaries() = 0;

    /// <summary>Download a dictionary from server</summary>
    /// <param name="dictionaryName">Name of dictionary to download</param>
    /// <returns>Dictionary as text (dcf) format.</returns>
    /// <exception cref="SyncException"></exception>
    virtual CString getDictionary(CString dictionaryName) = 0;

    /// <summary>Upload a dictionary to server</summary>
    /// <param name="dictPath">Full path to dictionary on local device</param>
    /// <exception cref="SyncException"></exception>
    virtual void putDictionary(CString dictPath) = 0;

    /// <summary>Delete a dictionary on the server</summary>
    /// <param name="dictName">Name of dictionay to delete</param>
    /// <exception cref="SyncException"></exception>
    virtual void deleteDictionary(CString dictName) = 0;

    /// <summary>Download a file</summary>
    /// <param name="remoteFilePath">Full path on remote server to download</param>
    /// <param name="tempLocalFilePath">Temporary path on local machine to write file contents to</param>
    /// <param name="actualLocalFilePath">Full path on local machine that caller will rename temp file to</param>
    /// <param name="md5">Optional hash of existing file to check if remote has same version to avoid re-download</param>
    /// <returns>True if file was downloaded or false if no downloaded needed due to etag</returns>
    /// <exception cref="SyncException">On error or cancel</exception>
    virtual bool getFile(CString remoteFilePath, CString tempLocalFilePath, CString actualLocalFilePath, CString md5 = CString()) = 0;

    /// <summary>Download a file if it exists on the server</summary>
    /// <param name="remoteFilePath">Full path on remote server to download</param>
    /// <returns>A temporary file if a file was downloaded or null if the file does not exist on the server</returns>
    /// <exception cref="SyncException">On error or cancel</exception>
    virtual std::unique_ptr<TemporaryFile> getFileIfExists(const CString& remoteFilePath) = 0;

    /// <summary>Upload a file</summary>
    /// <param name="localPath">Full path to file on local device</param>
    /// <param name="remotePath">Destination path for upload</param>
    /// <exception cref="SyncException"></exception>
    virtual void putFile(CString localPath, CString remotePath) = 0;

    /// <summary>Get remote directory listing</summary>
    /// <exception cref="SyncException"></exception>
    virtual std::vector<FileInfo>* getDirectoryListing(CString remotePath) = 0;

    /// <summary>List all application deployment packages available on the server</summary>
    /// <returns>List of packages from server</returns>
    /// <exception cref="SyncException"></exception>
    virtual std::vector<ApplicationPackage> listApplicationPackages() = 0;

    /// <summary>Download an application deployment package from the server</summary>
    /// <param name="packageName">Name of package to download</param>
    /// <param name="localPath">Destination path to save package file to on local device</param>
    /// <param name="currentPackage">Current package spec or empty if package not already installed</param>
    /// <exception cref="SyncException"></exception>
    /// <returns>True if file was downloaded or false if no downloaded needed because server version is same</returns>
    virtual bool downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature) = 0;

    /// <summary>Upload an application deployment package to the server</summary>
    /// <param name="localPath">Source path to package file on local device</param>
    /// <param name="packageName">Name of package to upload</param>
    /// <param name="packageSpecJson">Metadata of package to upload</param>
    /// <exception cref="SyncException"></exception>
    virtual void uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson) = 0;

    /// <summary>Send a message to the server</summary>
    /// <param name="message_key">Tag to identify message type</param>
    /// <param name="message_value">Message content</param>
    /// <exception cref="SyncException"></exception>
    virtual CString syncMessage(const CString& message_key, const CString& message_value) = 0;


    /// <summary>Sends the paradata log UUID to the server and gets the server's log UUID.</summary>
    virtual CString startParadataSync(const CString& log_uuid) = 0;

    /// <summary>Sends the paradata log to the server.</summary>
    virtual void putParadata(const CString& filename) = 0;

    /// <summary>Receives paradata logs from the server.</summary>
    virtual std::vector<std::shared_ptr<TemporaryFile>> getParadata() = 0;

    /// <summary>Informs the server that the paradata sync was successful.</summary>
    virtual void stopParadataSync() = 0;


    /// <summary>Send request to synchronize</summary>
    virtual void setListener(ISyncListener* pListener) = 0;

    /// <summary>Set server parameters in pff to download from this server</summary>
    virtual void setDownloadPffServerParams(PFF& pPff) = 0;

    virtual ~ISyncServerConnection() {};
};
