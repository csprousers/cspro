#pragma once
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/DefaultChunk.h>

struct IFtpConnection;
struct ILoginDialog;
struct ICredentialStore;
struct IDataChunk;
class Case;
class CDataDict;
template <typename T> class FileBasedSmartSyncer;
template <typename T> class FileBasedParadataSyncer;

// FTP server connection for file sync.
class SYNC_API FtpSyncServerConnection : public ISyncServerConnection
{
public:

    FtpSyncServerConnection(std::shared_ptr<IFtpConnection> pConnection, CString hostUrl,
        CString username, CString password);
    FtpSyncServerConnection(std::shared_ptr<IFtpConnection> pConnection, CString hostUrl,
        ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    ~FtpSyncServerConnection();

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

    bool fileExists(CString path);
    bool directoryExists(CString path);
    void getStream(CString remotePath, std::ostream& stream);
    void putStream(std::istream& stream, int64_t sizeBytes, CString remotePath);

    void setDownloadPffServerParams(PFF& pff) override;

private:
    FileBasedSmartSyncer<FtpSyncServerConnection>* m_pSmartSyncer;
    std::unique_ptr<FileBasedParadataSyncer<FtpSyncServerConnection>> m_paradataSyncer;
    std::shared_ptr<IFtpConnection> m_pConnection;
    ILoginDialog* m_pLoginDialog;
    ICredentialStore* m_pSyncCredentialStore;
    CString m_hostUrl;
    CString m_username;
    CString m_password;
    CString m_basePath;
    DefaultDataChunk m_dataChunk; // ALW - TODO: This exists, so calls to IDataChunk compile in SyncClient.
                                  // ALW - Next step is to make use of it throughout FTPSyncServerConnection.
    ISyncListener* m_pListener;
};
