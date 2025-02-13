#pragma once
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/DropboxDb.h>
#include <zSyncO/DefaultChunk.h>

struct IHttpConnection;
class HeaderList;
struct IDropboxAuthDialog;
struct ICredentialStore;
struct IDataChunk;
class Case;
class CDataDict;
template <typename T> class FileBasedSmartSyncer;
template <typename T> class FileBasedParadataSyncer;

// Dropbox connection for CSPro http file sync server.
class DropboxSyncServerConnection : public ISyncServerConnection
{
public:

    DropboxSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString accessToken);

    DropboxSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection,
        IDropboxAuthDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    ~DropboxSyncServerConnection();

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

private:
    void init(std::shared_ptr<IHttpConnection> pConnection);
    int testConnection(std::string& responseBody);
    int sendTestConnectionRequest(CString url, std::string& responseBody);
    int sendRefreshTokenConnectionRequest(std::string& responseBody);
    int sendGetFileRequest(CString url, CString localFilePath, CString remoteFilePath, CString actualFilePath, CString checksum);
    void addEtag(CString actualFilePath, CString checksum, HeaderList& headers);
    void saveEtag(CString localFilePath, CString actualFilePath, HeaderList responseHeaders, std::ofstream& fileStream);
    int sendPutFileRequest(CString url, CString localFilePath, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody);
    int sendPutFileRequest(CString url, std::istream& uploadStream, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody);
    int sendChunkedPutFileRequest(CString localFilePath, CString remoteFilePath, int64_t fileSizeBytes, int64_t chunkSize, std::string& responseBody);
    int uploadSessionStart(CString localFilePath, CString remoteFilePath, int64_t chunkSize, std::ifstream& uploadStream, std::string& responseBody);
    int uploadSessionAppend(CString localFilePath, CString remoteFilePath, CString sessionId, CString closeSession, int64_t chunkSize, int64_t& offset, std::ifstream& uploadStream, std::string& responseBody);
    int uploadSessionFinish(CString sessionId, CString remoteFilePath, int64_t fileSizeBytes, std::string& responseBody);
    int sendGetDirectoryListingRequest(CString remotePath, std::vector<FileInfo>& directoryListing, std::string& responseBody);
    int listFolder(CString remotePath, std::string& responseBody);
    int listFolderContinue(CString cursor, std::string& responseBody);
    void buildDirectoryListing(const std::string& responseBody, std::vector<FileInfo>& directoryListing);
    void handleServerErrorResponse(int msgNum, int httpResponseCode, const std::string &responseBody);
    void handleServerErrorResponse(int msgNum, int httpResponseCode, CString remotePath, const std::string &responseBody);
    FileInfo getFileInfo(CString remoteFilePath);

    FileBasedSmartSyncer<DropboxSyncServerConnection>* m_pSmartSyncer;
    std::unique_ptr<FileBasedParadataSyncer<DropboxSyncServerConnection>> m_paradataSyncer;
    std::shared_ptr<IHttpConnection> m_pConnection;
    CString m_accessToken;
    CString m_refreshToken;
    CString m_authHeader;
    DropboxDb m_dropboxDb;
    bool m_isValidDb;
    IDropboxAuthDialog* m_pAuthDialog;
    ICredentialStore* m_pSyncCredentialStore;
    DefaultDataChunk m_dataChunk; // ALW - TODO: This exists, so calls to IDataChunk compile in SyncClient.
                                  // ALW - Next step is to make use of it throughout DropboxSyncServerConnection.
    ISyncListener* m_pListener;
};
