#pragma once

#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/DefaultChunk.h>

template <typename T> class FileBasedSmartSyncer;
template <typename T> class FileBasedParadataSyncer;


// Local file connection for fast download of files from Dropbox or FTP server
class LocalFileSyncServerConnection : public ISyncServerConnection
{
public:
    LocalFileSyncServerConnection(const CString& root_directory, std::string local_file_sync_type = "file");

    ~LocalFileSyncServerConnection();

    // Connect to remote server
    // Throws SyncException.
    virtual ConnectResponse* connect() override;

    // Disconnect from remote server
    // Throws SyncException.
    virtual void disconnect() override;

    // Send request to download cases from server
    // Throws SyncException.
    virtual SyncGetResponse getData(const SyncRequest& request) override;

    // Send request to upload cases to server
    // Throws SyncException.
    virtual SyncPutResponse putData(const SyncRequest& request) override;

    // Get mutable chunk object.
    virtual IDataChunk& getChunk() override;

    // Get list of dictionaries.
    // Throws SyncException.
    virtual std::vector<DictionaryInfo> getDictionaries() override;

    // Download a dictionary
    // Throws SyncException.
    virtual CString getDictionary(CString dictionaryName) override;

    // Upload a dictionary
    // Throws SyncException.
    virtual void putDictionary(CString dictPath) override;

    // Delete a dictionary
    // Throws SyncException.
    virtual void deleteDictionary(CString dictName) override;

    // Download a file
    // Throws SyncException.
    virtual bool getFile(CString remoteFilePath, CString tempLocalFilePath, CString actualLocalFilePath, CString md5 = CString()) override;

    /// <summary>Download a file if it exists on the server</summary>
    // Throws SyncException.
    virtual std::unique_ptr<TemporaryFile> getFileIfExists(const CString& remoteFilePath) override;

    // Upload a file
    // Throws SyncException.
    virtual void putFile(CString localPath, CString remotePath) override;

    // Get remote directory listing
    // Throws SyncException.
    virtual std::vector<FileInfo>* getDirectoryListing(CString remotePath) override;

    // List application packages on server
    std::vector<ApplicationPackage> listApplicationPackages() override;

    // Download application package from server
    bool downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature) override;

    // Upload application package to server
    void uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson) override;

    CString syncMessage(const CString& message_key, const CString& message_value) override;

    CString startParadataSync(const CString& log_uuid) override;
    void putParadata(const CString& filename) override;
    std::vector<std::shared_ptr<TemporaryFile>> getParadata() override;
    void stopParadataSync() override;

    virtual void setListener(ISyncListener* pListener) override;
    ISyncListener* getListener() const { return m_pListener; }

    bool fileExists(CString remoteFilePath);
    bool directoryExists(CString remoteDirectoryPath);
    void getStream(CString remotePath, std::ostream& stream);
    void putStream(std::istream& stream, int64_t sizeBytes, CString remotePath);

    void setDownloadPffServerParams(PFF& pff) override;

    const CString& getRootDirectory() const { return m_rootDirectory; }

private:
    void UpdateProgress(uint64_t bytes_so_far);
    void CopyStreams(std::istream& src, std::ostream& dst, uint64_t total_bytes);

    const std::string m_localFileSyncType;
    CString m_rootDirectory;
    FileBasedSmartSyncer<LocalFileSyncServerConnection>* m_pSmartSyncer;
    std::unique_ptr<FileBasedParadataSyncer<LocalFileSyncServerConnection>> m_paradataSyncer;
    DefaultDataChunk m_dataChunk;
    ISyncListener* m_pListener;
};
