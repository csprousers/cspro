#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/ObexConstants.h>
#include <zNetwork/HeaderList.h>
#include <zAppO/SyncTypes.h>

class BluetoothObexConnection;
struct IBluetoothAdapter;
struct IChooseBluetoothDeviceDialog;
struct IDataChunk;


// Server connection for peer to peer CSPro smart sync server over bluetooth.
class BluetoothSyncServerConnection : public ISyncServerConnection
{
public:
    BluetoothSyncServerConnection(IBluetoothAdapter* pAdapter, BluetoothObexConnection* pObexConnection,
        const BluetoothDeviceInfo& deviceInfo);

    BluetoothSyncServerConnection(IBluetoothAdapter* pAdapter, BluetoothObexConnection* pObexConnection,
        IChooseBluetoothDeviceDialog* pChooseDeviceDialog);

    virtual ~BluetoothSyncServerConnection();

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

    std::vector<ApplicationPackage> listApplicationPackages() override
    {
        return std::vector<ApplicationPackage>();
    }

    bool downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature) override;

    void uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson) override
    {}

    CString syncMessage(const CString& message_key, const CString& message_value) override;

    CString startParadataSync(const CString& log_uuid) override;
    void putParadata(const CString& filename) override;
    std::vector<std::shared_ptr<TemporaryFile>> getParadata() override;
    void stopParadataSync() override;

    virtual void setListener(ISyncListener* pListener) override;

    void setDownloadPffServerParams(PFF& ) override  {}

private:
    ObexResponseCode GetDataChunk(CString dictName, DeviceId deviceId, CString universe,
        CString serverRevision, CString lastGuid,
        const std::vector<CString>& excludedServerRevisions,
        HeaderList& responseHeaders, std::string& responseBody);

    ObexResponseCode getFileWorker(const CString& type, const CString& remotePath, const CString& localPath,
        std::optional<HeaderList> requestHeaders = std::nullopt);
    void putFileWorker(const CString& type, const CString& localPath, const CString& remotePath);

    BluetoothDeviceInfo m_deviceInfo;
    IChooseBluetoothDeviceDialog* m_pChooseDeviceDialog;
    BluetoothObexConnection* m_pObexConnection;
    IBluetoothAdapter* m_pAdapter;
    bool m_bWasBluetoothEnabled;
    ISyncListener* m_pListener;
    float m_server_cspro_version;
};

