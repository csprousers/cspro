#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncHistoryEntry.h>

struct ISyncServerConnectionFactory;
struct ISyncListener;
struct ILoginDialog;
struct ICredentialStore;
struct IDropboxAuthDialog;
struct IChooseBluetoothDeviceDialog;

// Main client side interface to smart sync.
// Handles all communication with server as well as "smarts" about which
// cases to sync.
class SYNC_API SyncClient
{
public:

    ///<summary>Create a new sync client instance</summary>
    ///<param name="myDeviceId">device id of client</param>
    ///<param name="pServerFactory">factory for creating server connections</param>
    SyncClient(DeviceId myDeviceId,
        ISyncServerConnectionFactory *pServerFactory);

    ~SyncClient();

    enum class SyncResult {
        SYNC_OK = 1,
        SYNC_ERROR = 0,
        SYNC_CANCELED = -1
    };

    // Connect to sync server using url, will be prompted for credentials
    SyncResult connect(const CString& hostUrl, ILoginDialog* pLoginDlg, IDropboxAuthDialog* pAuthDlg,
        ICredentialStore* pCredentialStore, IChooseBluetoothDeviceDialog* pChooseDlg,
        std::optional<CString> username = {}, std::optional<CString> = {});

    // Connect to sync server using url and credentials
    SyncResult connectWeb(CString hostUrl, CString username, CString password);

    // Connect to sync server using url and saved oauth token
    SyncResult connectWeb(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    // Connect to peer device for p2p sync
    // If the device address is known specify it, otherwise it will
    // obtained from name. Specifying the address will be faster.
    SyncResult connectBluetooth(const BluetoothDeviceInfo& deviceInfo);
    SyncResult connectBluetooth(IChooseBluetoothDeviceDialog* pChooseDlg);

    // Connect to Dropbox server
    SyncResult connectDropbox(CString accessToken);
    SyncResult connectDropbox(IDropboxAuthDialog* pAuthDlg, ICredentialStore* pCredentialStore);

    // Connect to Dropbox on local machine
    SyncResult connectDropboxLocal();

    // Connect to sync server using url and credentials
    SyncResult connectFtp(CString hostUrl, CString username, CString password);

    // Connect to sync server using url and saved oauth token
    SyncResult connectFtp(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore);

    // Connect to local file system for sync
    SyncResult connectLocalFileSystem(CString root_directory);

    SyncResult disconnect();

    bool isConnected() const;

    // Get device id of connected server
    const DeviceId& getServerDeviceId() const;

    // Sync data file using smart sync
    SyncResult syncData(SyncDirection direction, ISyncableDataRepository& repository, CString universe);

    ///<summary>Sync non-data file</summary>
    ///<param name="direction">direction of sync (PUT or GET)</param>
    ///<param name="pathFrom">path of source file (on server for get or on client for put)</param>
    ///<param name="pathTo">path of dest file (on client for get or on server for put)</param>
    ///<param name="clientFileRoot">base directory for files on client specified with relative paths</param>
    SyncResult syncFile(SyncDirection direction, CString pathFrom, CString pathTo, CString clientFileRoot);

    ///<summary>Download a list of dictionaries on the server that can be used with syncData</summary>
    ///<param name="dictionaries">List of dictionaries returned. Each dictionary is a pair of name and label.</param>
    SyncResult getDictionaries(std::vector<DictionaryInfo>& dictionaries);

    ///<summary>Download a dictionary from the server. Retrieves full text of the the dcf file for the dictionary.</summary>
    ///<param name="dictionaryName">Name of dictionary.</param>
    ///<param name="dictionaryText">Dictionary contents as text (dcf format) returned.</param>
    SyncResult downloadDictionary(CString dictionaryName, CString& dictionaryText);

    ///<summary>Upload a dictionary to server to be used with syncData</summary>
    ///<param name="dictPath">path of dictionary file to upload</param>
    SyncResult uploadDictionary(CString dictPath);

    ///<summary>Delete a dictionary on the server</summary>
    ///<param name="dictName">Name of dictionary to delete</param>
    SyncResult deleteDictionary(CString dictName);

    /// <summary>List all application deployment packages available on the server</summary>
    /// <param name="packages">List of packages returned</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult listApplicationPackages(std::vector<ApplicationPackage>& packages);

    /// <summary>Download an application deployment package from the server and install it</summary>
    /// <param name="packageName">Name of package to download and install</param>
    /// <param name="forceFullInstall">Skip smart update and download entire package even if the installed package is up to date</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult downloadApplicationPackage(const CString& packageName, bool forceFullInstall);

    /// <summary>Upload an application deployment package to the server</summary>
    /// <param name="localPath">Source path to package file on local device</param>
    /// <param name="packageName">Name of package to upload</param>
    /// <param name="packageSpecJson">Package spec file in JSON</param>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson);

    /// <summary>Download an deployment package for currently running application and update it</summary>
    /// <returns>1 on success, 0 on failure</returns>
    SyncResult updateApplication(const CString& applicationPath);

    /// <summary>List all application deployment packages currently installed on the client</summary>
    /// <param name="packages">List of packages returned</param>
    /// <returns>1 on success, 0 on failure</returns>
    std::vector<ApplicationPackage> listInstalledApplicationPackages();

    CString syncMessage(const CString& message_key, const CString& message_value);

    SyncResult syncParadata(SyncDirection sync_directory);

    // Set callbacks for reporting errors and progress
    void setListener(ISyncListener *pListener);
    ISyncListener* getListener() const
    {
        return m_pListener;
    }

private:

    SyncResult connectToServer(ISyncServerConnection* pServer, CString serverName);
    SyncResult disconnectFromServer();
    SyncHistoryEntry getRevisionFromLastSync(SyncDirection direction, ISyncableDataRepository& repository, CString universe);
    void syncDataGet(ISyncableDataRepository& repository, CString universe);
    void syncDataPut(ISyncableDataRepository& repository, CString universe);
    SyncResult getFilesWithWildcard(CString pathFrom, CString pathTo);
    SyncResult getFile(CString pathFrom, CString pathTo);
    void downloadOneFile(CString pathFrom, CString pathTo, CString md5);
    SyncResult putFilesWithWildcard(CString pathFrom, CString pathTo, CString clientFileRoot);
    SyncResult putFile(CString pathFrom, CString pathTo, CString clientFileRoot);
    void uploadOneFile(CString pathFrom, CString pathTo);
    void downloadAndInstallPackage(const CString& packageName, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature);

    DeviceId m_deviceId;
    DeviceId m_serverDeviceId;
    CString m_serverDeviceName;
    CString m_userName;
    ISyncServerConnectionFactory *m_pServerFactory;
    ISyncServerConnection* m_pServer;
    ISyncListener* m_pListener;
};
