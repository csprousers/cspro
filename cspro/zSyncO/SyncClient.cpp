#include "stdafx.h"
#include "SyncClient.h"
#include "ApplicationPackageManager.h"
#include "BluetoothSyncServerConnection.h"
#include "CaseObservable.h"
#include "ConnectResponse.h"
#include "IDataChunk.h"
#include "ISyncListener.h"
#include "ISyncServerConnection.h"
#include "ISyncServerConnectionFactory.h"
#include "JsonConverter.h"
#include "SyncException.h"
#include "SyncRequest.h"
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zCaseO/VectorClock.h>
#include <zDataO/DataRepositoryTransaction.h>
#include <zParadataO/Logger.h>
#include <zParadataO/Syncer.h>
#include <easyloggingwrapper.h>
#include <ctime>
#include <fstream>
#include <regex>


#if defined(ANDROID) && defined(_DEBUG)
#define LOG_SYNC_TO_ANDROID_LOG 1
#endif

#ifdef LOG_SYNC_TO_ANDROID_LOG
#include <android/log.h>
#endif

// This needs to be done in one cpp file to
// declare easylogging globals
INITIALIZE_EASYLOGGINGPP

namespace {

    std::string getLogFilename()
    {
#ifdef WIN_DESKTOP
        // the log file will go in the AppData folder on Windows
        std::wstring log_directory = GetAppDataPath();
#else
        std::wstring log_directory = PlatformInterface::GetInstance()->GetCSEntryDirectory();
#endif

        std::wstring log_filename = PortableFunctions::PathAppendToPath(log_directory, _T("sync.log"));

        return UTF8Convert::WideToUTF8(log_filename);
    }

#ifdef LOG_SYNC_TO_ANDROID_LOG
    class AndroidLogDispatcher : public el::LogDispatchCallback
    {
        void handle(const el::LogDispatchData* data) noexcept override {
            android_LogPriority priority;
            switch (data->logMessage()->level()) {
                case el::Level::Debug:
                case el::Level::Trace:
                    priority = ANDROID_LOG_DEBUG;
                    break;
                case el::Level::Verbose:
                    priority = ANDROID_LOG_VERBOSE;
                    break;
                case el::Level::Error:
                    priority = ANDROID_LOG_ERROR;
                    break;
                case el::Level::Warning:
                    priority = ANDROID_LOG_WARN;
                    break;
                case el::Level::Fatal:
                    priority = ANDROID_LOG_FATAL;
                    break;
                case el::Level::Info:
                    priority = ANDROID_LOG_INFO;
                    break;
                default:
                    priority = ANDROID_LOG_INFO;
                    break;
            }
            __android_log_print(priority, "Sync", "%s",  data->logMessage()->message().data());
        }
    };
#endif

    // Initial setup of sync log
    void configureLogging()
    {
        static bool loggingConfigured = false;
        if (!loggingConfigured) {
            loggingConfigured = true;
            el::Configurations c;
            //c.setToDefault();
            c.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
            c.set(el::Level::Global, el::ConfigurationType::Format, "%datetime %level: %msg");
            c.set(el::Level::Global, el::ConfigurationType::ToFile, "true");
            c.set(el::Level::Global, el::ConfigurationType::Filename, getLogFilename());
            c.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
            c.set(el::Level::Global, el::ConfigurationType::LogFlushThreshold, "0");
            el::Loggers::reconfigureLogger("sync", c);

#ifdef LOG_SYNC_TO_ANDROID_LOG
            el::Helpers::installLogDispatchCallback<AndroidLogDispatcher>("AndroidLogDispatcher");
            auto dispatcher = el::Helpers::logDispatchCallback<AndroidLogDispatcher>("AndroidLogDispatcher");
            dispatcher->setEnabled(true);
#endif
        }
    }

    const bool bUpdateOnConflict = false;

    CString PathCanonical(CString path)
    {
        CString canonicalDirectoryPath;
        bool canonOk = PathCanonicalize(canonicalDirectoryPath.GetBuffer(MAX_PATH), path) == TRUE;
        canonicalDirectoryPath.ReleaseBuffer();
        if (!canonOk) {
            CLOG(ERROR, "sync") << "Error: Invalid directory " << UTF8Convert::WideToUTF8(path);
            throw SyncError(100113, path);
        }
        return canonicalDirectoryPath;
    }

    bool containsWildcard(CString path)
    {
        return path.FindOneOf(_T("*?#")) != -1;
    }

    // If path is relative then add root directory
    // to make it absolute.
    CString makeClientPathAbsolute(CString clientPath, CString clientFileRoot)
    {
        if (PathIsRelative(clientPath)) {
            return clientFileRoot + clientPath;
        }
        return clientPath;
    }

    CString addFilenameToPathTo(CString pathFrom, CString pathTo, wchar_t pathSeparator)
    {
        CString fullPathTo = pathTo;

        // Add the filename if it is not included in pathTo
        if (fullPathTo.IsEmpty() ||
            fullPathTo.GetAt(fullPathTo.GetLength() - 1) == _T('/') ||
            fullPathTo.GetAt(fullPathTo.GetLength() - 1) == _T('\\') ||
            PortableFunctions::FileIsDirectory(fullPathTo)) {

            CString fromFilename = PortableFunctions::PathGetFilename(pathFrom);

            if (fullPathTo.IsEmpty() ||
                (fullPathTo.GetAt(fullPathTo.GetLength() - 1) != _T('/') &&
                fullPathTo.GetAt(fullPathTo.GetLength() - 1) != _T('\\'))) {
                fullPathTo += pathSeparator;
            }
            fullPathTo += fromFilename;
        }

        return fullPathTo;
    }

    const char* directionToString(SyncDirection d)
    {
        switch (d) {
        case SyncDirection::Put:
            return "PUT";
        case SyncDirection::Get:
            return "GET";
        case SyncDirection::Both:
            return "BOTH";
        }
        return "INVALID";
    }

    std::vector<CString> getPutRevisionsSince(CString deviceId, ISyncableDataRepository& repository, const SyncHistoryEntry& syncEntry)
    {
        std::vector<SyncHistoryEntry> syncsSince = repository.GetSyncHistory(deviceId, SyncDirection::Put, syncEntry.getSerialNumber());
        std::vector<CString> revs;
        revs.reserve(syncsSince.size());
        for (const SyncHistoryEntry& s : syncsSince) {
            if (!s.getServerFileRevision().IsEmpty())
                revs.push_back(s.getServerFileRevision());
        }
        return revs;
    }

    JsonConverter jsonConverter;
}

SyncClient::SyncClient(DeviceId myDeviceId,
    ISyncServerConnectionFactory *pServerFactory)
    : m_deviceId(myDeviceId),
      m_pServerFactory(pServerFactory),
      m_pServer(NULL),
      m_pListener(NULL)
{
    configureLogging();
}

SyncClient::~SyncClient()
{
    if (m_pServer)
        disconnectFromServer();
}

SyncClient::SyncResult SyncClient::connect(const CString& hostUrl, ILoginDialog* pLoginDlg, IDropboxAuthDialog* pAuthDlg,
    ICredentialStore* pCredentialStore, IChooseBluetoothDeviceDialog* pChooseDlg, std::optional<CString> username,
    std::optional<CString> password)
{
    if (hostUrl == _T("Dropbox")) {
        return connectDropbox(pAuthDlg, pCredentialStore);
    }
    else if (hostUrl == _T("Bluetooth")) {
        return connectBluetooth(pChooseDlg);
    }
    else if (hostUrl.Left(4).CompareNoCase(_T("http")) == 0) {
        if (username && password)
            return connectWeb(hostUrl, *username, *password);
        else
            return connectWeb(hostUrl, pLoginDlg, pCredentialStore);
    }
    else if (hostUrl.Left(3).CompareNoCase(_T("ftp")) == 0) {
        if (username && password)
            return connectFtp(hostUrl, *username, *password);
        else
            return connectFtp(hostUrl, pLoginDlg, pCredentialStore);
    }
    else if (hostUrl == _T("DropboxLocal")) {
        return connectDropboxLocal();
    }
    else if (hostUrl.Left(4).CompareNoCase(_T("file")) == 0) {
        return connectLocalFileSystem(hostUrl);
    }

    return SyncResult::SYNC_ERROR;
}

SyncClient::SyncResult SyncClient::connectWeb(CString hostUrl, CString username, CString password)
{
    ISyncServerConnection* pServer = m_pServerFactory->createCSWebConnection(hostUrl, username, password);
    if (!pServer) {
        if (m_pListener) {
            m_pListener->onError(100120, (LPCTSTR) hostUrl);
        }
        CLOG(ERROR, "sync") << "Error failed to connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " using username " << UTF8Convert::WideToUTF8(username) << " and password";
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " as user " << UTF8Convert::WideToUTF8(username);
    return connectToServer(pServer, hostUrl);
}

SyncClient::SyncResult SyncClient::connectWeb(CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore)
{
    ISyncServerConnection* pServer = m_pServerFactory->createCSWebConnection(hostUrl, pLoginDlg, pCredentialStore);
    if (!pServer) {
        if (m_pListener) {
            m_pListener->onError(100120, (LPCTSTR) hostUrl);
        }
        CLOG(ERROR, "sync") << "Error failed to connect to server: " << UTF8Convert::WideToUTF8(hostUrl) << " using saved token";
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " using saved credentials";
    return connectToServer(pServer, hostUrl);
}

SyncClient::SyncResult SyncClient::connectBluetooth(const BluetoothDeviceInfo& deviceInfo)
{
    CLOG(INFO, "sync") << "Connect to Bluetooth device " << UTF8Convert::WideToUTF8(deviceInfo.csName);
    ISyncServerConnection* pServer = m_pServerFactory->createBluetoothConnection(deviceInfo);
    if (!pServer)
        return SyncResult::SYNC_ERROR;
    return connectToServer(pServer, deviceInfo.csName);
}

SyncClient::SyncResult SyncClient::connectBluetooth(IChooseBluetoothDeviceDialog* pChooseDlg)
{
    CLOG(INFO, "sync") << "Connect to Bluetooth device, name not specified";
    ISyncServerConnection* pServer = m_pServerFactory->createBluetoothConnection(pChooseDlg);
    if (!pServer)
        return SyncResult::SYNC_ERROR;
    return connectToServer(pServer, _T("Bluetooth"));
}

SyncClient::SyncResult SyncClient::connectDropbox(CString accessToken)
{
    CLOG(INFO, "sync") << "Connect to Dropbox ";
    ISyncServerConnection* pServer = m_pServerFactory->createDropboxConnection(accessToken);
    if (!pServer)
        return SyncResult::SYNC_ERROR;
    return connectToServer(pServer, _T("Dropbox"));
}

SyncClient::SyncResult SyncClient::connectDropbox(IDropboxAuthDialog* pAuthDlg, ICredentialStore* pCredentialStore)
{
    CLOG(INFO, "sync") << "Connect to Dropbox ";
    ISyncServerConnection* pServer = m_pServerFactory->createDropboxConnection(pAuthDlg, pCredentialStore);
    if (!pServer)
        return SyncResult::SYNC_ERROR;
    return connectToServer(pServer, _T("Dropbox"));
}

SyncClient::SyncResult SyncClient::connectDropboxLocal()
{
    ISyncServerConnection* pServer = m_pServerFactory->createDropboxLocalConnection();
    if (!pServer) {
        CLOG(ERROR, "sync") << "Error failed to connect to local Dropbox: ";
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to local Dropbox ";
    return connectToServer(pServer, _T("LocalDropbox"));
}

SyncClient::SyncResult SyncClient::connectFtp(CString hostUrl, CString username, CString password)
{
    ISyncServerConnection* pServer = m_pServerFactory->createFtpConnection(hostUrl, username, password);
    if (!pServer) {
        if (m_pListener) {
            m_pListener->onError(100120, (LPCTSTR) hostUrl);
        }
        CLOG(ERROR, "sync") << "Error failed to connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " using username " << UTF8Convert::WideToUTF8(username) << " and password";
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " as user " << UTF8Convert::WideToUTF8(username);
    return connectToServer(pServer, hostUrl);
}

SyncClient::SyncResult SyncClient::connectFtp(CString hostUrl, ILoginDialog * pLoginDlg, ICredentialStore * pCredentialStore)
{
    ISyncServerConnection* pServer = m_pServerFactory->createFtpConnection(hostUrl, pLoginDlg, pCredentialStore);
    if (!pServer) {
        if (m_pListener) {
            m_pListener->onError(100120, (LPCTSTR) hostUrl);
        }
        CLOG(ERROR, "sync") << "Error failed to connect to server: " << UTF8Convert::WideToUTF8(hostUrl) << " using saved token";
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to server " << UTF8Convert::WideToUTF8(hostUrl) << " using saved credentials";
    return connectToServer(pServer, hostUrl);
}

SyncClient::SyncResult SyncClient::connectLocalFileSystem(CString root_directory)
{
    ISyncServerConnection* pServer = m_pServerFactory->createLocalFileConnection(root_directory);
    if (!pServer) {
        CLOG(ERROR, "sync") << "Error failed to connect to local filesystem: " << UTF8Convert::WideToUTF8(root_directory);
        return SyncResult::SYNC_ERROR;
    }

    CLOG(INFO, "sync") << "Connect to local filesystem " << UTF8Convert::WideToUTF8(root_directory);
    return connectToServer(pServer, root_directory);
}

SyncClient::SyncResult SyncClient::connectToServer(ISyncServerConnection* pServer, CString serverName)
{
    CLOG(INFO, "sync") << "Connect to server " << UTF8Convert::WideToUTF8(serverName);

    try {
        SyncListenerCloser listenerCloser(m_pListener);

        // Disconnect old connection before connecting to a new server
        if (m_pServer) {
            CLOG(INFO, "sync") << "Disconnect from existing server";
            disconnectFromServer();
        }

        pServer->setListener(m_pListener);

        // Connecting...
        if (m_pListener) {
            m_pListener->onStart(100102, (LPCTSTR)serverName);
        }

        m_pServer = pServer;
        std::unique_ptr<ConnectResponse> pResponse(pServer->connect());
        m_serverDeviceId = pResponse->getServerDeviceId();
        m_serverDeviceName = pResponse->getServerName();
        m_userName = pResponse->getUserName();

        CLOG(INFO, "sync") << "Connection successful. Server id: " << UTF8Convert::WideToUTF8(m_serverDeviceId);

        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        CLOG(ERROR, "sync") << "Error connecting to server: " << e.what();
        if (m_pListener) {
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        }
        m_pServerFactory->destroy(m_pServer);
        m_pServer = NULL;
        m_serverDeviceId = DeviceId();
        return SyncResult::SYNC_ERROR;
    } catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Canceled connection";
        m_pServerFactory->destroy(m_pServer);
        m_pServer = NULL;
        m_serverDeviceId = DeviceId();
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::disconnect()
{
    if (m_pServer == nullptr)
        return SyncResult::SYNC_ERROR;

    SyncListenerCloser listenerCloser(m_pListener);
    m_pServer->setListener(m_pListener);

    // disconnecting...
    if (m_pListener)
        m_pListener->onStart(100103);

    CLOG(INFO, "sync") << "Disconnecting from server ";

    return disconnectFromServer();
}

SyncClient::SyncResult SyncClient::disconnectFromServer()
{
    try {

        if (!m_pServer)
            return SyncResult::SYNC_ERROR;

        m_pServer->disconnect();
        m_pServerFactory->destroy(m_pServer);
        m_pServer = NULL;

        return SyncResult::SYNC_OK;
    }
    catch (const SyncException&) {
        // Ignore disconnect errors since at this point the sync is done
        // and 90% of the time there was already an error during a call
        // to sync_data or sync_file prior to the disconnect.
    }

    m_pServerFactory->destroy(m_pServer);
    m_pServer = NULL;
    return SyncResult::SYNC_ERROR;
}

bool SyncClient::isConnected() const
{
    return m_pServer != NULL;
}

const DeviceId& SyncClient::getServerDeviceId() const
{
    return m_serverDeviceId;
}

SyncClient::SyncResult SyncClient::syncData(SyncDirection direction, ISyncableDataRepository& repository, CString universe)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        const CString dataFileName = repository.GetName(DataRepositoryNameType::Concise);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100104, (LPCTSTR) dataFileName);

        CLOG(INFO, "sync") << "Syncing data: " << UTF8Convert::WideToUTF8(dataFileName) << " direction " <<
            directionToString(direction) << " universe \"" << UTF8Convert::WideToUTF8(universe) << "\"";

        if (!m_pServer)
            return SyncResult::SYNC_ERROR;

        if (direction == SyncDirection::Get || direction == SyncDirection::Both)
            syncDataGet(repository, universe);

        if (direction == SyncDirection::Put || direction == SyncDirection::Both) {
            syncDataPut(repository, universe);
        }

        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error syncing data: " << e.m_errorCode << " " << e.what();
        return SyncResult::SYNC_ERROR;
    } catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Canceled sync data";
        return SyncResult::SYNC_CANCELED;
    }

}

void SyncClient::syncDataGet(ISyncableDataRepository& repository, CString universe)
{
    try {

        SyncHistoryEntry lastSyncRev = getRevisionFromLastSync(SyncDirection::Get, repository, universe);

        if (lastSyncRev.valid()) {
            CLOG(INFO, "sync") << "Last GET with this server " << PortableFunctions::TimeToString(lastSyncRev.getDateTime()) <<
                " local rev " << lastSyncRev.getFileRevision() <<
                " server rev \"" << UTF8Convert::WideToUTF8(lastSyncRev.getServerFileRevision()) << "\"";
            if (lastSyncRev.isPartialGet()) {
                CLOG(INFO, "sync") << "Partial get, resume from case " << UTF8Convert::WideToUTF8(lastSyncRev.getLastCaseUuid());
            }
        }
        else {
            CLOG(INFO, "sync") << "First time GET with this server";
        }

        if (lastSyncRev.isPartialGet() && lastSyncRev.getUniverse() != universe) {
            // Invalid to try to resume a sync with different params
            CLOG(INFO, "sync") << "Universe doesn't match universe from partial - doing full sync";
            lastSyncRev = SyncHistoryEntry();
        }

        CString serverRevision = lastSyncRev.getServerFileRevision();
        CString startCaseUuid = lastSyncRev.isPartialGet() ? lastSyncRev.getLastCaseUuid() : CString();
        std::vector<CString> excludedRevisions = getPutRevisionsSince(m_serverDeviceId, repository, lastSyncRev);

        m_pServer->getChunk().enableOptimization();
        repository.StartSync(m_serverDeviceId, m_serverDeviceName, m_userName, SyncDirection::Get, universe, bUpdateOnConflict);
        DataRepositoryTransaction transaction(repository);

        int total_cases = 0;
        int cases_so_far = 0;

        while (true) {

            // Send request to server
            bool first_chunk = cases_so_far == 0;
            if (m_pListener && !first_chunk)
                m_pListener->showProgressUpdates(false); // we will update progress below based on cases so disable default handling

            SyncRequest request(*repository.GetCaseAccess(), m_deviceId, universe, serverRevision, startCaseUuid, excludedRevisions, first_chunk);
            SyncGetResponse response = m_pServer->getData(request);

            if (response.getResult() == SyncGetResponse::SyncGetResult::RevisionNotFound) {

                CLOG(INFO, "sync") << "Previous revision not found, doing full sync";
                // Server revision history is out of sync with local revision history. Fallback to doing a full sync instead
                // of changes since last sync.
                lastSyncRev = SyncHistoryEntry();
                serverRevision = CString();
                startCaseUuid = CString();
                excludedRevisions.clear();

            }
            else {
                // OK or more data
                auto server_revision = response.getServerRevision();
                if (total_cases == 0 && response.getTotalCases()) {
                    total_cases = *response.getTotalCases();
                    if (m_pListener)
                        m_pListener->setProgressTotal(total_cases);
                }

                CString lastUuid;
                if (response.getCases()) {

                    response.getCases()->subscribe(
                        [this, &repository, server_revision, &lastUuid, &cases_so_far](std::shared_ptr<Case> data_case)
                        {
                            repository.SyncCasesFromRemote({ data_case }, server_revision);
                            lastUuid = data_case->GetUuid();
                            if (m_pListener) {
                                m_pListener->showProgressUpdates(true);
                                m_pListener->onProgress(++cases_so_far);
                                m_pListener->showProgressUpdates(false);
                            }
                        },
                        [](std::exception_ptr eptr)
                        {
                            std::rethrow_exception(eptr);
                        }
                    );
                }

                serverRevision = response.getServerRevision();

                if (response.getResult() == SyncGetResponse::SyncGetResult::Complete)
                    break;

                // Start next chunk after last uuid received
                startCaseUuid = lastUuid;
            }
        }

        repository.EndSync();
        m_pServer->getChunk().resetOptimization();

        ISyncableDataRepository::SyncStats stats = repository.GetLastSyncStats();


        CLOG(INFO, "sync") << "New server revision = " << UTF8Convert::WideToUTF8(serverRevision);
        CLOG(INFO, "sync") << "Sync GET completed. ";
        CLOG(INFO, "sync") << "Downloaded " << stats.numReceived << " cases";
        CLOG(INFO, "sync") << stats.numNewCasesNotInRepo
            << " new cases, " << stats.numCasesNewerOnRemote << " updated, "
            << stats.numCasesNewerInRepo << " ignored, " << stats.numConflicts
            << " conflicts";
    }
    catch( const DataRepositoryException::Error& exception ) {
        CLOG(ERROR, "sync") << "Database error during sync GET: " << UTF8Convert::WideToUTF8(exception.GetErrorMessage());
        throw SyncError(100133, WS2CS(exception.GetErrorMessage()));
    }
}

void SyncClient::syncDataPut(ISyncableDataRepository& repository, CString universe)
{
    try {
        CString excludeGetsFromDevice = m_serverDeviceId;
        SyncHistoryEntry lastSyncRev = getRevisionFromLastSync(SyncDirection::Put, repository, universe);

        if (lastSyncRev.valid()) {
            CLOG(INFO, "sync") << "Last PUT with this server " << PortableFunctions::TimeToString(lastSyncRev.getDateTime()) <<
                " local rev " << lastSyncRev.getFileRevision() <<
                " server rev \"" << UTF8Convert::WideToUTF8(lastSyncRev.getServerFileRevision()) << "\"";
            if (lastSyncRev.isPartialPut())
                CLOG(INFO, "sync") << "Partial put, resume from case " << UTF8Convert::WideToUTF8(lastSyncRev.getLastCaseUuid());
        }
        else {
            CLOG(INFO, "sync") << "First time PUT with this server";
        }

        if (lastSyncRev.isPartialPut() && lastSyncRev.getUniverse() != universe) {
            // Invalid to try to resume a sync with different params
            CLOG(INFO, "sync") << "Universe doesn't match universe from partial - doing full sync";
            lastSyncRev = SyncHistoryEntry();
            excludeGetsFromDevice = CString();
            repository.ClearBinarySyncHistory(m_serverDeviceId);
        }

        int nCaseCount = 0;
        int nMaxClientRev = 0;
        int nCasesSent = 0;

        int clientRevision = lastSyncRev.getFileRevision();

        // When uploading multiple chunks we store revs as comma separate list
        // so we need to grab the last one to get the most recent sync put rev
        CString serverRevision = lastSyncRev.getServerFileRevision().IsEmpty() ?
            CString() :
            CString(SO::SplitString<wstring_view>(lastSyncRev.getServerFileRevision(), ',').back());

        CString lastUuid = lastSyncRev.isPartialPut() ? lastSyncRev.getLastCaseUuid() : CString();
        CString allReturnedRevisions;

        std::vector<std::shared_ptr<Case>> cases_pool;

        std::shared_ptr<CaseIterator> case_iterator = repository.GetCasesModifiedSinceRevisionIterator(clientRevision,
            lastUuid, universe, m_pServer->getChunk().getSize(), &nCaseCount, &nMaxClientRev, excludeGetsFromDevice);

        CLOG(INFO, "sync") << "Total new/modified cases since last sync: " << nCaseCount;
        if (m_pListener) {
            m_pListener->setProgressTotal(nCaseCount);
        }
        m_pServer->getChunk().enableOptimization();
        repository.StartSync(m_serverDeviceId, m_serverDeviceName, m_userName, SyncDirection::Put, universe, bUpdateOnConflict);

        bool bDoneProcessing = false;
        while (true) {
            // read the cases in this chunk
            std::vector<std::shared_ptr<Case>> cases_in_chunk;
            //loop through each case and keep a running count of the binary items length. If it it exceeds a preset value
            //send the cases accumulated so far before processing the remaining cases in the chunk
            while( true ) {
                size_t cases_in_chunk_index = cases_in_chunk.size();

                if( cases_in_chunk_index >= cases_pool.size() )
                    cases_pool.emplace_back(repository.GetCaseAccess()->CreateCase());

                if( case_iterator->NextCase(*cases_pool[cases_in_chunk_index]) )
                    cases_in_chunk.emplace_back(cases_pool[cases_in_chunk_index]);

                else
                    break;
            }

            if (m_pListener)
                m_pListener->showProgressUpdates(false); // we will update progress based on cases, so disable default progress

            bool first_chunk = nCasesSent == 0;
            int  caseIndexInChunk = 0;
            if (bDoneProcessing)
                break;
            while (caseIndexInChunk < cases_in_chunk.size() || nCasesSent == 0) {
                std::vector<std::shared_ptr<Case>> cases_in_sub_chunk;
                std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>> binary_case_items_in_chunk;
                std::set<std::string> excludeKeys;
                uint64_t totalBinaryItemsByteSize = 0;
                //process a subset of the chunk when the totalBinaryItemsByteSize exceeds a set limit
                while(caseIndexInChunk < cases_in_chunk.size()) {
                    const auto& currentCase = cases_in_chunk[caseIndexInChunk];
                    cases_in_sub_chunk.push_back(currentCase);
                    //always send the server device as we are using this to exclude binary items for puts.  The exclude gets is used for cases to avoid sending
                    //cases that we get from the server. When a revision on the server is not found this is set to blank to resend items that were previously received from this server.
                    //however, binary items do this differently as the binary content can be common between cases. In this case clearBinarySyncHistory removes the information
                    //about binary items for gets and puts from/to the server and they are resent again.
                    repository.GetBinaryCaseItemsModifiedSinceRevision(currentCase.get(), binary_case_items_in_chunk, excludeKeys, totalBinaryItemsByteSize, m_serverDeviceId);
                    caseIndexInChunk++;
                    if (totalBinaryItemsByteSize > m_pServer->getChunk().getBinaryContentSize()) {
                        CLOG(INFO, "sync") << "Binary items in the chunk exceeded the set limit of chunk size. Sending a subchunk of cases: " << cases_in_sub_chunk.size() << " cases";
                        break;
                    }
                }
                SyncRequest request(*repository.GetCaseAccess(), m_deviceId, universe, cases_in_sub_chunk, binary_case_items_in_chunk, serverRevision, first_chunk);
                SyncPutResponse response = m_pServer->putData(request);
                if (response.getResult() == SyncPutResponse::SyncPutResult::RevisionNotFound) {

                    CLOG(INFO, "sync") << "Previous revision not found, doing full sync";

                    // Server revision history is out of sync with local revision history. Fallback to doing a full sync instead
                    // of changes since last sync.
                    clientRevision = 0;
                    serverRevision = CString();
                    excludeGetsFromDevice = CString();
                    lastUuid = CString();

                    //clear the binary sync to this device
                    repository.ClearBinarySyncHistory(m_serverDeviceId);
                    // Get next chunk of cases
                    case_iterator = repository.GetCasesModifiedSinceRevisionIterator(clientRevision,
                        lastUuid, universe, m_pServer->getChunk().getSize(), &nCaseCount, &nMaxClientRev, excludeGetsFromDevice);

                    if (m_pListener) {
                        m_pListener->setProgressTotal(nCaseCount);
                    }
                    break; //process the cases in the case_iterator for a full sync 
                }
                else {
                    nCasesSent += cases_in_sub_chunk.size();
                    CLOG(INFO, "sync") << "Uploaded chunk of " << cases_in_sub_chunk.size() << " cases";
                    if (m_pListener) {
                        m_pListener->showProgressUpdates(true);
                        m_pListener->onProgress(nCasesSent);
                    }

                    // Since a single sync on client can correspond to multiple server revisions we store
                    // a comma separated list of server revisions in database
                    serverRevision = response.getServerRevision();
                    allReturnedRevisions += (allReturnedRevisions.IsEmpty() ? L"" : L",") + serverRevision;

                    //get the max revision at this subchunk size
                    int nSubchunkMaxClientRev = nMaxClientRev;
                    if (caseIndexInChunk < cases_in_chunk.size()) {
                        repository.GetCasesModifiedSinceRevisionIterator(clientRevision,
                            lastUuid, universe, caseIndexInChunk, NULL, &nSubchunkMaxClientRev, excludeGetsFromDevice);
                    }
                    repository.MarkCasesSentToRemote(cases_in_sub_chunk, binary_case_items_in_chunk, allReturnedRevisions, nSubchunkMaxClientRev);
                    if (nCasesSent >= nCaseCount) {
                        // No more cases, all done
                        bDoneProcessing = true;
                        break;
                    }
                    if (caseIndexInChunk < cases_in_chunk.size()) {
                        CLOG(INFO, "sync") << "Total subchunk of cases sent so far: " << caseIndexInChunk << " of " << cases_in_chunk.size() << " cases";
                        continue;
                    }

                    ASSERT(cases_in_sub_chunk.size() > 0);
                    lastUuid = cases_in_sub_chunk.back()->GetUuid();
                    clientRevision = nMaxClientRev;

                    // Get next chunk of cases
                    case_iterator = repository.GetCasesModifiedSinceRevisionIterator(clientRevision,
                        lastUuid, universe, m_pServer->getChunk().getSize(), NULL, &nMaxClientRev, excludeGetsFromDevice);
                }
            }
        }

        repository.EndSync();
        m_pServer->getChunk().resetOptimization();

        ISyncableDataRepository::SyncStats stats = repository.GetLastSyncStats();
        CLOG(INFO, "sync") << "New server revision = " << UTF8Convert::WideToUTF8(serverRevision);
        CLOG(INFO, "sync") << "Sync PUT completed. ";
        CLOG(INFO, "sync") << "Uploaded " << stats.numSent << " cases";
    }
    catch( const DataRepositoryException::Error& exception ) {
        CLOG(ERROR, "sync") << "Database error during sync PUT: " << UTF8Convert::WideToUTF8(exception.GetErrorMessage());
        throw SyncError(100133, WS2CS(exception.GetErrorMessage()));
    }
}

SyncClient::SyncResult SyncClient::syncFile(SyncDirection direction, CString pathFrom, CString pathTo, CString clientFileRoot)
{
    CLOG(INFO, "sync") << "Sync file: " << directionToString(direction) << " from: " <<
        UTF8Convert::WideToUTF8(pathFrom) << " to: " << UTF8Convert::WideToUTF8(pathTo) <<
        " root: " << UTF8Convert::WideToUTF8(clientFileRoot);

    if (direction == SyncDirection::Get) {
        pathFrom = PortableFunctions::PathToForwardSlash(pathFrom);
        pathTo = PortableFunctions::PathToNativeSlash(pathTo);
        clientFileRoot = PortableFunctions::PathToNativeSlash(clientFileRoot);
        pathTo = makeClientPathAbsolute(pathTo, clientFileRoot);
        pathTo = PathCanonical(pathTo);

        if (containsWildcard(pathFrom)) {
            return getFilesWithWildcard(pathFrom, pathTo);
        }
        else {
            return getFile(pathFrom, pathTo);
        }
    }
    else if (direction == SyncDirection::Put) {
        pathFrom = PortableFunctions::PathToNativeSlash(pathFrom);
        clientFileRoot = PortableFunctions::PathToNativeSlash(clientFileRoot);
        pathFrom = makeClientPathAbsolute(pathFrom, clientFileRoot);
        pathTo = PortableFunctions::PathToForwardSlash(pathTo);
        if (containsWildcard(pathFrom)) {
            return putFilesWithWildcard(pathFrom, pathTo, clientFileRoot);
        } else {
            return putFile(pathFrom, pathTo, clientFileRoot);
        }
    }
    else {
        return SyncResult::SYNC_ERROR;
    }
}

SyncClient::SyncResult SyncClient::getDictionaries(std::vector<DictionaryInfo>& dictionaries)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);

        if (m_pServer == nullptr)
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100128);

        CLOG(INFO, "sync") << "Downloading dictionary list";
        dictionaries = m_pServer->getDictionaries();

        CLOG(INFO, "sync") << "Downloading dictionary list complete";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading dictionaries: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Dictionary list download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::downloadDictionary(CString dictionaryName, CString& dictionaryText)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100107, (LPCTSTR) dictionaryName);

        CLOG(INFO, "sync") << "Downloading dictionary file: " << UTF8Convert::WideToUTF8(dictionaryName);
        dictionaryText = m_pServer->getDictionary(dictionaryName);

        CLOG(INFO, "sync") << "Downloading dictionary file complete";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading dictionary: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Dictionary download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::uploadDictionary(CString dictPath)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100108, (LPCTSTR) PortableFunctions::PathGetFilename(dictPath));

        CLOG(INFO, "sync") << "Uploading dictionary file: " << UTF8Convert::WideToUTF8(dictPath);
        m_pServer->putDictionary(dictPath);

        CLOG(INFO, "sync") << "Upload dictionary completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError & e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error uploading dictionary: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Dictionary upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncHistoryEntry SyncClient::getRevisionFromLastSync(SyncDirection direction, ISyncableDataRepository& repository, CString universe)
{
    SyncHistoryEntry lastSyncRev = repository.GetLastSyncForDevice(m_serverDeviceId, direction);

    if (lastSyncRev.valid()) {
        // only use the previous revision number if the universe stayed the same or became more restrictive
        if (universe.Find(lastSyncRev.getUniverse()) != 0)
            return SyncHistoryEntry();
    }
    return lastSyncRev;
}

SyncClient::SyncResult SyncClient::getFilesWithWildcard(CString pathFrom, CString pathTo)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100107, (LPCTSTR)pathFrom);

        CLOG(INFO, "sync") << "Get files from: " << UTF8Convert::WideToUTF8(pathFrom) << " to: " << UTF8Convert::WideToUTF8(pathTo);

        // Get the directory listing
        CString fromDirectory = PortableFunctions::PathGetDirectory<CString>(pathFrom);

        // If directory is empty send root directory since sending an empty string
        // gives us the server info.
        if (fromDirectory.IsEmpty())
            fromDirectory = _T("/");

        std::unique_ptr<std::vector<FileInfo> > files(m_pServer->getDirectoryListing(fromDirectory));

        // Download each file in the directory that matches the pattern
        CString fileSpec = PortableFunctions::PathGetFilename(pathFrom);
        FileSpecRegex regEx;
        try {
            regEx = CreateRegexFromFileSpec(fileSpec);
        }
        catch (const std::regex_error&) {
            CLOG(INFO, "sync") << "Syncfile has an invalid wildcard specification";
            throw SyncError(100113, fileSpec);
        }

        for (std::vector<FileInfo>::const_iterator i = files->begin(); i != files->end(); ++i) {
            bool process = ( i->getType() == FileInfo::FileType::File ) &&
#ifdef WIN32
                           std::regex_match((LPCTSTR)i->getName(), regEx);
#else
                           std::regex_match(UTF8Convert::WideToUTF8(i->getName()), regEx);
#endif
            if (process) {
                CString pathToFile = addFilenameToPathTo(i->getDirectory() + i->getName(), pathTo, PATH_CHAR);

                std::wstring md5 = PortableFunctions::FileMd5(pathToFile);
                if (md5.empty() || !SO::EqualsNoCase(md5, i->getMd5())) {
                    CLOG(INFO, "sync") << "Downloading file " << UTF8Convert::WideToUTF8(i->getName());
                    downloadOneFile(i->getDirectory() + i->getName(), pathToFile, WS2CS(md5));
                } else {
                    CLOG(INFO, "sync") << "Skipping file " << UTF8Convert::WideToUTF8(i->getName()) << " version on server matches local version";
                }
            }
        }

        CLOG(INFO, "sync") << "Sync file completed";

        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading files: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "File download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::getFile(CString pathFrom, CString pathTo)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100107, (LPCTSTR) pathFrom);

        CString pathToFile = addFilenameToPathTo(pathFrom, pathTo, PATH_CHAR);
        std::wstring md5 = PortableFunctions::FileMd5(pathToFile);
        CLOG(INFO, "sync") << "Downloading file: " << UTF8Convert::WideToUTF8(pathFrom) << " to: " << UTF8Convert::WideToUTF8(pathToFile);
        downloadOneFile(pathFrom, pathToFile, WS2CS(md5));

        CLOG(INFO, "sync") << "Sync file completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading file: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "File download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

void SyncClient::downloadOneFile(CString pathFrom, CString pathTo, CString md5)
{
    if (m_pListener)
        m_pListener->onProgress(0, 100107, (LPCTSTR) PortableFunctions::PathGetFilename(pathTo));

    // Store the downloaded file in a temporary file first so we don't
    // corrupt the original if the download fails. Put the temp file
    // in same directory as destination file so that we can rename
    // it after. On Android it is not possible to rename files
    // on different mount points and there is gaurantee that system
    // temp is on same mount point as dest file.

    CString toDir = PortableFunctions::PathGetDirectory<CString>(pathTo);

    // Make sure that the directory we are saving the file to exists
    PortableFunctions::PathMakeDirectories(toDir);

    TemporaryFile temp(toDir);

    if (m_pServer->getFile(pathFrom, WS2CS(temp.GetPath()), pathTo, md5)) {
        // Delete the existing file first, otherwise rename will fail
        PortableFunctions::FileDelete(pathTo);

        // Move the temp file to correct location
        if (!temp.Rename(CS2WS(pathTo)))
            throw SyncError(100109, pathTo);
    }
}

SyncClient::SyncResult SyncClient::putFilesWithWildcard(CString pathFrom, CString pathTo, CString clientFileRoot)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100108, (LPCTSTR)pathFrom);


        CLOG(INFO, "sync") << "Put files from: " << UTF8Convert::WideToUTF8(pathFrom) << " to: " << UTF8Convert::WideToUTF8(pathTo);

        if (!PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(pathFrom)))
            throw SyncError(100113, pathFrom);

        // Make sure that destination path ends in "/" so that it gets treated as a directory and not a file
        pathTo = PortableFunctions::PathEnsureTrailingForwardSlash(pathTo);

        for (const std::wstring& filename : DirectoryLister::GetFilenamesWithPossibleWildcard(pathFrom, true)) {
            uploadOneFile(WS2CS(filename), pathTo);
        }
        CLOG(INFO, "sync") << "File upload completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error uploading files: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "File upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::putFile(CString pathFrom, CString pathTo, CString clientFileRoot)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100108, (LPCTSTR)pathFrom);

        uploadOneFile(pathFrom, pathTo);

        CLOG(INFO, "sync") << "File upload completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error uploading file: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "File upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

void SyncClient::uploadOneFile(CString pathFrom, CString pathTo)
{
    if (m_pListener)
        m_pListener->onProgress(0, 100108, (LPCTSTR) PortableFunctions::PathGetFilename(pathFrom));

    if (!PortableFunctions::FileExists(pathFrom))
        throw SyncError(100118, pathFrom);

    pathTo = addFilenameToPathTo(pathFrom, pathTo, '/');

    CLOG(INFO, "sync") << "Uploading file: " << UTF8Convert::WideToUTF8(pathFrom) << " to: " << UTF8Convert::WideToUTF8(pathTo);

    // Send request to server
    m_pServer->putFile(pathFrom, pathTo);
}

void SyncClient::downloadAndInstallPackage(const CString& packageName, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature)
{
#ifndef WIN_DESKTOP
    TemporaryFile tmp_zip;

    if (!m_pServer->downloadApplicationPackage(packageName, WS2CS(tmp_zip.GetPath()), currentPackage, currentPackageSignature)) {
        CLOG(INFO, "sync") << "Application package " << UTF8Convert::WideToUTF8(packageName) << "already up to date ";
        return;
    }

    ApplicationPackageManager::installApplication(packageName, WS2CS(tmp_zip.GetPath()), currentPackage);

#else
    UNREFERENCED_PARAMETER(packageName);
    UNREFERENCED_PARAMETER(currentPackage);
    UNREFERENCED_PARAMETER(currentPackageSignature);
#endif
}

SyncClient::SyncResult SyncClient::deleteDictionary(CString dictName)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100123, (LPCTSTR) dictName);

        CLOG(INFO, "sync") << "Deleting dictionary  " << UTF8Convert::WideToUTF8(dictName);
        m_pServer->deleteDictionary(dictName);

        CLOG(INFO, "sync") << "Delete dictionary completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error deleting dictionary: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Dictionary delete canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::listApplicationPackages(std::vector<ApplicationPackage>& packages)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);

        if (m_pServer == nullptr)
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        if (m_pListener)
            m_pListener->onStart(100136);

        CLOG(INFO, "sync") << "Downloading application deployment package list";
        packages = m_pServer->listApplicationPackages();
        CLOG(INFO, "sync") << "Found " << packages.size() << " application packages";

        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading package list: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Package listing canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::downloadApplicationPackage(const CString& packageName, bool forceFullInstall)
{
#ifndef WIN_DESKTOP

    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        CLOG(INFO, "sync") << "Downloading package " << UTF8Convert::WideToUTF8(packageName);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100107, (LPCTSTR)packageName);

        if (forceFullInstall) {
            downloadAndInstallPackage(packageName, {}, CString());
        } else {
            auto current_package = ApplicationPackageManager::getInstalledApplicationPackageWithSignature(packageName);
            if (current_package)
                downloadAndInstallPackage(packageName, current_package->package, current_package->signature);
            else
                downloadAndInstallPackage(packageName, {}, CString());
        }

        CLOG(INFO, "sync") << "Package download completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading package: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Package download canceled";
        return SyncResult::SYNC_CANCELED;
    }

#else
    UNREFERENCED_PARAMETER(packageName);
    UNREFERENCED_PARAMETER(forceFullInstall);

    return SyncResult::SYNC_ERROR;
#endif
}

SyncClient::SyncResult SyncClient::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);
        if (m_pServer == nullptr)
            throw SyncError(100132);
        m_pServer->setListener(m_pListener);

        CLOG(INFO, "sync") << "Uploading package " << UTF8Convert::WideToUTF8(packageName);

        // Syncing ...
        if (m_pListener)
            m_pListener->onStart(100108, (LPCTSTR)packageName);

        m_pServer->uploadApplicationPackage(localPath, packageName, packageSpecJson);

        CLOG(INFO, "sync") << "Package upload completed";
        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error uploading package: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Package upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}

SyncClient::SyncResult SyncClient::updateApplication(const CString& applicationPath)
{
#ifndef WIN_DESKTOP
    try {
        auto current_package = ApplicationPackageManager::getApplicationPackageWithSignatureFromAppDirectory(applicationPath);
        if (!current_package) {
            CLOG(ERROR, "sync")
                    << "No installed package found when trying to update application package from "
                    << UTF8Convert::WideToUTF8(applicationPath);
            throw SyncError(100151);
        }

        const CString packageName = current_package->package.getName();

        SyncListenerCloser listenerCloser(m_pListener);

        if (m_pServer == nullptr)
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        if (m_pListener)
            m_pListener->onStart(100107, (LPCTSTR)packageName);

        CLOG(INFO, "sync") << "Checking for updates for package " << UTF8Convert::WideToUTF8(packageName);

        // Download and install the new package
        downloadAndInstallPackage(packageName, current_package->package, current_package->signature);

        CLOG(INFO, "sync") << "Done checking for updates for package " << UTF8Convert::WideToUTF8(packageName);

        return SyncResult::SYNC_OK;
    }
    catch (const SyncError& e) {
        if (m_pListener)
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        CLOG(ERROR, "sync") << "Error downloading package list: " << e.what();
        return SyncResult::SYNC_ERROR;
    }
    catch (const SyncCancelException&) {
        CLOG(INFO, "sync") << "Package listing canceled";
        return SyncResult::SYNC_CANCELED;
    }
#else
    UNREFERENCED_PARAMETER(applicationPath);

    return SyncResult::SYNC_ERROR;
#endif
}

std::vector<ApplicationPackage> SyncClient::listInstalledApplicationPackages()
{
#ifndef WIN_DESKTOP
    return ApplicationPackageManager::getInstalledApplications();
#else
    return {};
#endif
}


CString SyncClient::syncMessage(const CString& message_key, const CString& message_value)
{
    try
    {
        SyncListenerCloser listenerCloser(m_pListener);

        if( m_pServer == nullptr )
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        if( m_pListener != nullptr )
            m_pListener->onStart(100155);

        CLOG(INFO, "sync") << "Syncing the message " << UTF8Convert::WideToUTF8(message_key);

        return m_pServer->syncMessage(message_key, message_value);
    }

    catch( const SyncError& exception )
    {
        if( m_pListener != nullptr )
            m_pListener->onError(exception.m_errorCode, exception.GetErrorMessage().c_str());

        return CString();
    }
}


SyncClient::SyncResult SyncClient::syncParadata(SyncDirection sync_directory)
{
    ASSERT(Paradata::Logger::IsOpen());

    try
    {
        SyncListenerCloser listenerCloser(m_pListener);

        if( m_pServer == nullptr )
            throw SyncError(100132);

        m_pServer->setListener(m_pListener);

        if( m_pListener != nullptr )
            m_pListener->onStart(100171);

        auto paradata_syncer = Paradata::Logger::GetSyncer();

        // start the sync, sending the client's log UUID and getting the server's log UUID
        paradata_syncer->SetPeerLogUuid(m_pServer->startParadataSync(paradata_syncer->GetLogUuid()));
        ASSERT(!paradata_syncer->GetPeerLogUuid().IsEmpty());

        // receive paradata from the server
        if( sync_directory != SyncDirection::Put )
        {
            CLOG(INFO, "sync")
                << "Requesting paradata events to be added to the log "
                << UTF8Convert::WideToUTF8(paradata_syncer->GetLogUuid());

            auto received_database_temporary_files = m_pServer->getParadata();

            if( !received_database_temporary_files.empty() )
            {
                CLOG(INFO, "sync")
                    << "Received paradata events from the log "
                    << UTF8Convert::WideToUTF8(paradata_syncer->GetPeerLogUuid());

                paradata_syncer->SetReceivedSyncableDatabases(received_database_temporary_files);
            }

            else
            {
                CLOG(INFO, "sync")
                    << "No paradata events from the log "
                    << UTF8Convert::WideToUTF8(paradata_syncer->GetPeerLogUuid())
                    << " received as all events are up-to-date";
            }
        }

        // send paradata to the server
        if( sync_directory != SyncDirection::Get )
        {
            std::optional<std::wstring> extracted_syncable_database_filename = paradata_syncer->GetExtractedSyncableDatabase();

            if( extracted_syncable_database_filename.has_value() )
            {
                CLOG(INFO, "sync")
                    << "Sending paradata events to be added to the log "
                    << UTF8Convert::WideToUTF8(paradata_syncer->GetPeerLogUuid());

                m_pServer->putParadata(WS2CS(*extracted_syncable_database_filename));
            }

            else
            {
                CLOG(INFO, "sync")
                    << "Skipping sending paradata events to the log "
                    << UTF8Convert::WideToUTF8(paradata_syncer->GetPeerLogUuid())
                    << " as all events are up-to-date";
            }
        }

        // after merging any received data, inform the server that all was transfered well
        paradata_syncer->MergeReceivedSyncableDatabases();

        m_pServer->stopParadataSync();

        paradata_syncer->RunPostSuccessfulSyncTasks();
    }

    catch( const SyncError& exception )
    {
        if( m_pListener != nullptr )
            m_pListener->onError(exception.m_errorCode, exception.GetErrorMessage().c_str());

        return SyncResult::SYNC_ERROR;
    }

    return SyncResult::SYNC_OK;
}


void SyncClient::setListener(ISyncListener *pListener)
{
    m_pListener = pListener;
}
