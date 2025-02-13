#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/CSWebDataChunk.h>
#include <zSyncO/DictionaryInfo.h>
#include <zNetwork/IHttpConnection.h>

struct ILoginDialog;
struct ICredentialStore;
struct IDataChunk;
class HeaderList;
template <typename T> class FileBasedParadataSyncer;

// Server connection for CSPro http smart sync server.
class SYNC_API CSWebSyncServerConnection : public ISyncServerConnection
{
public:
    CSWebSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString hostUrl, CString username, CString password);

    CSWebSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    ~CSWebSyncServerConnection();

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
    virtual CSWebDataChunk& getChunk() override;

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

    void setDownloadPffServerParams(PFF& pff) override;

    virtual void setListener(ISyncListener* pListener) override;

private:

    void retrieveOauthToken(CString username, CString password, CString& authToken, CString& refreshToken);
    bool refreshOauthToken();
    ConnectResponse* retrieveServerInfo();
    int sendRetrieveServerInfoRequest(CString url, std::string& result);
    SyncGetResponse downloadServerCases(CString url, const SyncRequest& request);

    SyncPutResponse uploadClientCases(CString url, CString deviceId, CString universe,
        CString lastServerRevision, const std::string& clientCasesJson, int numberOfCases);

    int sendUploadClientCasesRequest(CString url, const std::string& requestBody, CString deviceId, CString universe, CString lastServerRevision, HeaderList& responseHeaders, std::string& responseBody, float& requestTimeSecs);
    HttpResponse sendDownloadServerCasesRequest(CString url, CString deviceId, CString universe, CString lastServerRevision, CString lastGuid,
        const std::vector<CString>& excludeRevisions);
    int sendGetFileRequest(CString url, CString localFilePath, CString etag, const HeaderList& additionalRequestHeaders, HeaderList& responseHeaders);
    int sendPutFileRequest(CString url, CString localFilePath, int64_t fileSizeBytes, CString md5, std::string& responseBody);
    int sendGetJsonRequest(CString url, std::string& responseBody);
    void handleServerErrorResponse(int msgNum, int httpResponseCode, const std::string &responseBody);

    std::shared_ptr<IHttpConnection> m_pConnection;
    ILoginDialog* m_pLoginDialog;
    ICredentialStore* m_pSyncCredentialStore;
    CString m_hostUrl;
    CString m_username;
    CString m_password;
    CString m_authHeader;
    CString m_refreshToken;
    CSWebDataChunk m_dataChunk;
    float m_server_api_version;
    std::unique_ptr<FileBasedParadataSyncer<CSWebSyncServerConnection>> m_paradataSyncer;
};

