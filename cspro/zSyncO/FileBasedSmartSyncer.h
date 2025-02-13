#pragma once

#include <zSyncO/ApplicationPackage.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/ISyncListener.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/JsonConverter.h>
#include <zSyncO/SyncLogCaseConstructionReporter.h>
#include <zSyncO/SyncRequest.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zAppO/PFF.h>
#include <zCaseO/Case.h>
#include <zZipo/ZipUtility.h>
#include <regex>


///<summary>
/// Shared routines for doing smart data sync using only file operations encapsulated
/// in a template class. Template parameter T should be the appropriate sync server
/// connection class such DropbboxSyncServerConnection or FTPSyncServerConnection.
///</summary>
/// In addition the ISyncServerConnection interface class T must implement:
///         bool fileExists(CString path);
///         bool directoryExists(CString path);
///         void getStream(CString remotePath, std::ostream& stream);
///         void putStream(std::istream& stream, int64_t sizeBytes, CString remotePath);
/// File based smart sync stores JSON data in the same format used to sync to CSWeb in
/// files on the server. Each time cases are uploaded to the server from the client
/// all new/modified cases since the last sync are uploaded in a new file. Past sync files
/// that have already been uploaded to the server are never modified or overwritten.
/// So for every sync PUT from a client, a new file is created on the server. This means
/// that cases that are updated multiple times will be duplicated on the server but
/// when downloaded they will be merged correctly based on the uuid and vector clock.
/// Files are stored in the directory "CSProSync/<dictname>/data".
/// A copy of the dictionary is stored in "CSProSync/<dictname>/dict/dictionary.dcf"
/// Files are named <deviceid>$<random uuid>
/// The random uuid is added to avoid overwriting an existing file.
/// When doing a GET, the client first does a directory listing and then
/// only downloads files with modified dates more recent than that of
/// the last sync.
template <class T>
class FileBasedSmartSyncer
{
public:

    FileBasedSmartSyncer(T* pServerConnection);

    SyncGetResponse getData(const SyncRequest& syncRequest);

    SyncPutResponse putData(const SyncRequest& syncRequest);

    std::vector<DictionaryInfo> getDictionaries();

    CString getDictionary(CString dictionaryName);

    void putDictionary(CString dictPath);

    std::vector<ApplicationPackage> listApplicationPackages();

    bool downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage);

    void uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson);

private:

    void updateDownloadDataPff(CString dictName);
    void uploadDictionaryIfNeeded(const CDataDict& dictionary);
    CString getDataPath(CString dictName);
    CString getAppPath() const;

    CString getDataFilename(CString dataPath, CString deviceId, CString serverRevision);
    bool parseFilename(CString filename, CString& deviceId, CString& serverRevision);
    CString packageNameToFileName(CString packageName);

    T* m_pServerConnection;
    JsonConverter m_jsonConverter;
};

template<class T>
inline FileBasedSmartSyncer<T>::FileBasedSmartSyncer(T* pServerConnection) :
    m_pServerConnection(pServerConnection)
{}

template<class T>
inline SyncGetResponse FileBasedSmartSyncer<T>::getData(const SyncRequest& syncRequest)
{
    const CString dictionaryName = syncRequest.getDictionary().GetName();

    if (syncRequest.isFirstChunk())
        uploadDictionaryIfNeeded(syncRequest.getDictionary()); // only do this on first chunk

    time_t lastSyncTime = 0;
    std::vector<std::wstring> lastSyncMostRecentFilenames;
    if (!syncRequest.getLastServerRevision().IsEmpty()) {
        int dollarPos = syncRequest.getLastServerRevision().Find(_T('$'));
        if (dollarPos > 0) {
            char* endp = nullptr;
            lastSyncTime = strtoll(UTF8Convert::WideToUTF8(syncRequest.getLastServerRevision().Left(dollarPos)).c_str(), &endp, 10);
            lastSyncMostRecentFilenames = SO::SplitString(syncRequest.getLastServerRevision().Mid(dollarPos + 1), '|');
        }
        else {
            // Invalid revision
            return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
        }
    }

    // First check if data directory exists
    const CString dataPath = getDataPath(dictionaryName);

    if (!m_pServerConnection->directoryExists(dataPath)) {
        // No data directory yet so nothing to download
        CLOG(INFO, "sync") << "Server has no data directory.";
        return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, { }, CString());
    }

    // List data directory
    std::unique_ptr<std::vector<FileInfo>> pDataListing(m_pServerConnection->getDirectoryListing(dataPath));
    CLOG(INFO, "sync") << "Data directory files: " << pDataListing->size();

    // Check to see if last revision matches directory listing
    if (!syncRequest.getLastServerRevision().IsEmpty()) {
        for (const std::wstring& mrf : lastSyncMostRecentFilenames) {
            auto i = std::find_if(pDataListing->begin(), pDataListing->end(),
                [mrf](const FileInfo& fi) { return fi.getType() == FileInfo::FileType::File && SO::Equals(fi.getName(), mrf); });
            if (i == pDataListing->end())
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
        }
    }

    // Track most recent file modified date. That will be our last revision.
    time_t newestFileTime = 0;
    CString newestFileNames;

    // Ignore files that don't match the pattern deviceid$revision
    std::regex data_file_regex(R"(^[0-9a-fA-F\-]+\$[0-9a-fA-F\-]+$)");

    // Download files with more recent modified times

    std::vector<FileInfo> files_to_download;
    for (const FileInfo& fileInfo : *pDataListing.get()) {
        if (fileInfo.getType() == FileInfo::FileType::File) {

            // Skip files that don't match pattern such as temp files
            if (!std::regex_match(UTF8Convert::WideToUTF8(fileInfo.getName()), data_file_regex))
                continue;

            CString lastModDbg = PortableFunctions::TimeToRFC3339String(fileInfo.getLastModified());
            CLOG(INFO, "sync") << "File: " << UTF8Convert::WideToUTF8(fileInfo.getName()) << " Mod=" << fileInfo.getLastModified() << ", " << UTF8Convert::WideToUTF8(lastModDbg);

            if (fileInfo.getLastModified() > newestFileTime) {
                newestFileTime = fileInfo.getLastModified();
                newestFileNames = fileInfo.getName();
            }
            else if (fileInfo.getLastModified() == newestFileTime) {
                // store ties so we don't download them again

                if (newestFileNames.GetLength() < 4096) // Don't let it get too big, if we truncate we may download some files we don't need to but won't do any harm
                    newestFileNames += _T("|") + fileInfo.getName();
            }

            // Make sure this is a case file
            CString deviceId, serverRev;
            if (!parseFilename(fileInfo.getName(), deviceId, serverRev))
                continue;

            time_t fileLastModifiedTime = fileInfo.getLastModified();
            if (fileLastModifiedTime >= lastSyncTime) {

                // Only download cases that are not in the list of excluded revisions
                // (ones that we uploaded)
                if (deviceId == syncRequest.getDeviceId() &&
                    std::find(syncRequest.getExcludedServerRevisions().begin(), syncRequest.getExcludedServerRevisions().end(), serverRev) !=
                    syncRequest.getExcludedServerRevisions().end()) {
                    continue;
                }

                // Don't download the file(s) we used to mark the revision in the last sync (the newest file(s) at that time)
                // Since FTP file times are not precise, we could have had other files uploaded with the timestamp
                // from last revision after we synced so we have to download files with same timestamp as last revision
                // (fileLastModifiedTime >= lastSyncTime) instead of (fileLastModifiedTime > lastSyncTime).
                // This means that we will always download the file we used as the
                // revision on the next sync. To avoid this we also put the filename(s) that we found at that timestamp
                // in the revision so we can skip them.
                if (fileLastModifiedTime == lastSyncTime && std::find(lastSyncMostRecentFilenames.begin(), lastSyncMostRecentFilenames.end(), CS2WS(fileInfo.getName())) != lastSyncMostRecentFilenames.end()) {
                    continue;
                }

                files_to_download.emplace_back(fileInfo);
            }
        }
    }

    // Get cases from each of the files
    std::vector<std::shared_ptr<Case>> server_cases;

    int64_t total_size = 0;
    for (const FileInfo& fileInfo : files_to_download)
        total_size += fileInfo.getSize();

    if (m_pServerConnection->getListener()) {
        m_pServerConnection->getListener()->setProgressTotal(total_size);
    }

    for (const FileInfo& fileInfo : files_to_download) {

        CLOG(INFO, "sync") << "Download file: " << UTF8Convert::WideToUTF8(fileInfo.getName());

        std::ostringstream jsonStream;
        m_pServerConnection->getStream(dataPath + fileInfo.getName(), jsonStream);
        if (m_pServerConnection->getListener()) {
            m_pServerConnection->getListener()->addToProgressPreviousStepsTotal(fileInfo.getSize());
        }

        std::string casesJson = jsonStream.str();
        if (ZipUtility::isCompressed(casesJson)) {
            ZipUtility::decompression(casesJson);
        }

        std::istringstream iss;
        bool isBinaryFormat = false;
        if (m_jsonConverter.IsBinaryJson(casesJson)) {
            isBinaryFormat = true;
            iss.str(casesJson);
            casesJson = m_jsonConverter.readCasesJsonFromBinary(iss);
        }
        // Check universe
        try {
            auto all_server_cases = m_jsonConverter.caseListFromJson(casesJson, syncRequest.getCaseAccess(), std::make_shared<SyncLogCaseConstructionReporter>());
            if (isBinaryFormat) {
                m_jsonConverter.readBinaryCaseItems(iss, all_server_cases);
            }
            for (const auto& data_case : all_server_cases) {
                if (!syncRequest.getUniverse().IsEmpty()) {
                    if (data_case->GetKey().Find(syncRequest.getUniverse()) != 0)
                        continue;
                }
                server_cases.push_back(data_case);
            }

           
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << casesJson;
            throw SyncError(100121);
        }
    }



    // New server revision is the newest file time and name, next sync we will get anything newer
    CString serverRevision;
    if (newestFileTime > 0) {
        std::ostringstream revStream;
        revStream << newestFileTime;
        serverRevision = UTF8Convert::UTF8ToWide<CString>(revStream.str()) + _T('$') + newestFileNames;
    }

    auto cases_observable = std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(server_cases));
    return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, cases_observable, serverRevision);
}

template<class T>
inline SyncPutResponse FileBasedSmartSyncer<T>::putData(const SyncRequest& syncRequest)
{
    if (syncRequest.isFirstChunk())
        uploadDictionaryIfNeeded(syncRequest.getDictionary()); // Only on first chunks

    const CString dictionaryName = syncRequest.getDictionary().GetName();
    const CString dataPath = getDataPath(dictionaryName);

    if (!syncRequest.getLastServerRevision().IsEmpty()) {
        // Check to see if our last sync is on server
        CString lastSyncFilename = getDataFilename(dataPath, syncRequest.getDeviceId(), syncRequest.getLastServerRevision());

        if (!m_pServerConnection->fileExists(lastSyncFilename)) {
            return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
        }
    }

    // Server revision is just a random uuid that we add to filename
    // and that we save to database. When we put next time we can check
    // for the presence of the serverRevision file to see if the server
    // has our last sync.
    CString serverRevision = WS2CS(CreateUuid());

    CString filename = getDataFilename(dataPath, syncRequest.getDeviceId(), serverRevision);

    std::string clientCasesJson = m_jsonConverter.toJson(syncRequest.getClientCases());
    if (clientCasesJson.size() < 3) {
        CLOG(INFO, "sync") << "No cases to upload.";
        serverRevision = syncRequest.getLastServerRevision();
    } else {
        auto binaryCaseItems = syncRequest.getBinaryCaseItems();
        if (binaryCaseItems.size() > 0) { //write the new binary json format only if cases have binary items
            std::ostringstream oss;
            m_jsonConverter.writeBinaryJson(oss, clientCasesJson, syncRequest.getBinaryCaseItems());
            clientCasesJson = oss.str();
        }
        ZipUtility::compression(clientCasesJson);
        std::istringstream caseStream(clientCasesJson);
        m_pServerConnection->putStream(caseStream, clientCasesJson.size(), filename);
    }

    return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRevision);
}


template<class T>
inline std::vector<DictionaryInfo> FileBasedSmartSyncer<T>::getDictionaries()
{
    std::vector<DictionaryInfo> dictionaries;
    const CString dictRootPath = _T("/CSPro/DataSync/");
    if (m_pServerConnection->directoryExists(dictRootPath)) {
        std::unique_ptr<std::vector<FileInfo>> pListing(m_pServerConnection->getDirectoryListing(dictRootPath));
        for (const FileInfo& fi : *pListing) {
            if (fi.getType() == FileInfo::FileType::Directory) {
                const CString dictInfoPath = dictRootPath + fi.getName() + _T("/dict/info.json");
                try {
                    std::ostringstream infoStream;
                    m_pServerConnection->getStream(dictInfoPath, infoStream);
                    DictionaryInfo info = m_jsonConverter.dictionaryInfoFromJson(infoStream.str());
                    dictionaries.push_back(info);
                }
                catch (const SyncError&) {
                    // Just ignore the dictionary if files are missing or unreadable
                }
            }
        }
    }
    return dictionaries;
}

template<class T>
inline CString FileBasedSmartSyncer<T>::getDictionary(CString dictionaryName)
{
    const CString dictPath = _T("/CSPro/DataSync/") + dictionaryName + _T("/dict/") + dictionaryName + FileExtensions::WithDot::Dictionary;
    std::ostringstream dictStream;
    m_pServerConnection->getStream(dictPath, dictStream);
    return UTF8Convert::UTF8ToWide<CString>(dictStream.str());
}

template<class T>
inline void FileBasedSmartSyncer<T>::updateDownloadDataPff(CString dictName)
{
    // Upload pff file for data download
    const CString downloadPffPath = _T("/CSPro/DataSync/DownloadData-") + dictName + FileExtensions::WithDot::Pff;

    if (!m_pServerConnection->fileExists(downloadPffPath)) {
        TemporaryFile tmp;

        PFF pff;
        pff.SetPifFileName(WS2CS(tmp.GetPath()));
        pff.SetAppType(APPTYPE::SYNC_TYPE);
        pff.SetSyncDirection(SyncDirection::Get);
        pff.SetSilent(false);
        m_pServerConnection->setDownloadPffServerParams(pff);

        CString download_filename = PortableFunctions::PathGetDirectory<CString>(tmp.GetPath()) + dictName.MakeLower() + FileExtensions::Data::WithDot::CSProDB;
        pff.SetExternalDataConnectionString(dictName, download_filename);

        pff.Save();

        CLOG(INFO, "sync") << "Uploading download pff";
        m_pServerConnection->putFile(WS2CS(tmp.GetPath()), downloadPffPath);
    }
}

template<class T>
inline void FileBasedSmartSyncer<T>::uploadDictionaryIfNeeded(const CDataDict& dictionary)
{
    CString dictName = dictionary.GetName();
    const CString dictRootDir = _T("/CSPro/DataSync/") + dictName + _T("/dict/");
    const CString dictPath = dictRootDir + dictName + FileExtensions::WithDot::Dictionary;

    if (!m_pServerConnection->fileExists(dictPath)) {
        // Dictionary is missing, upload it
        CLOG(INFO, "sync") << "Uploading dictionary ";
        std::string dictionary_json = UTF8Convert::WideToUTF8(dictionary.GetJson());
        std::istringstream stream(dictionary_json);
        m_pServerConnection->putStream(stream, dictionary_json.size(), dictPath);
    }

    const CString dictInfoPath = dictRootDir + _T("info.json");
    if (!m_pServerConnection->fileExists(dictInfoPath)) {
        CLOG(INFO, "sync") << "Uploading dictionary info";
        DictionaryInfo info(dictionary.GetName(), dictionary.GetLabel(), -1);
        std::string json = m_jsonConverter.toJson(info);
        std::istringstream is(json);
        m_pServerConnection->putStream(is, json.size(), dictInfoPath);
    }

    updateDownloadDataPff(dictionary.GetName());
}

template<class T>
inline void FileBasedSmartSyncer<T>::putDictionary(CString localDictPath)
{
    CDataDict dict;

    try
    {
        dict.Open(localDictPath, true);
    }

    catch( const CSProException& exception )
    {
        throw SyncError(100143, WS2CS(exception.GetErrorMessage()));
    }

    CString dictName = dict.GetName();
    const CString dictRootDir = _T("/CSPro/DataSync/") + dictName + _T("/dict/");
    const CString remoteDictPath = dictRootDir + dictName + FileExtensions::WithDot::Dictionary;

    CLOG(INFO, "sync") << "Uploading dictionary ";
    m_pServerConnection->putFile(localDictPath, remoteDictPath);

    const CString dictInfoPath = dictRootDir + _T("info.json");
    CLOG(INFO, "sync") << "Uploading dictionary info";
    DictionaryInfo info(dict.GetName(), dict.GetLabel(), -1);
    std::string json = m_jsonConverter.toJson(info);
    std::istringstream is(json);
    m_pServerConnection->putStream(is, json.size(), dictInfoPath);

    updateDownloadDataPff(dict.GetName());
}

template<class T>
CString FileBasedSmartSyncer<T>::getDataPath(CString dictName)
{
    return _T("/CSPro/DataSync/") + dictName + _T("/data/");
}

template<class T>
CString FileBasedSmartSyncer<T>::getAppPath() const
{
    return _T("/CSPro/apps/");
}

template<class T>
CString FileBasedSmartSyncer<T>::getDataFilename(CString dataPath, CString deviceId, CString serverRevision)
{
    return dataPath + deviceId + _T('$') + serverRevision;
}

template<class T>
bool FileBasedSmartSyncer<T>::parseFilename(CString filename, CString& deviceId, CString& serverRevision)
{
    int dollarPos = filename.Find(L'$');
    if (dollarPos < 0 || filename.GetLength() - dollarPos != 37)
        return false; // uuid is length 36 so this is not a valid file
    deviceId = filename.Left(dollarPos);
    serverRevision = filename.Mid(dollarPos + 1);
    return true;
}

template<class T>
std::vector<ApplicationPackage> FileBasedSmartSyncer<T>::listApplicationPackages()
{
    std::vector<ApplicationPackage> packages;
    CString packagePath = getAppPath();

    if (!m_pServerConnection->directoryExists(packagePath)) {
        // No package directory yet so nothing to download
        CLOG(INFO, "sync") << "Server has no apps directory.";
        return packages;
    }

    // List packages directory
    std::unique_ptr<std::vector<FileInfo>> pDirListing(m_pServerConnection->getDirectoryListing(packagePath));
    CLOG(INFO, "sync") << "apps directory files: " << pDirListing->size();
    for (const FileInfo& fileInfo : *pDirListing.get()) {
        if (fileInfo.getType() == FileInfo::FileType::File) {
            CString packageFilename = fileInfo.getName();
            if (PortableFunctions::PathGetFileExtension(packageFilename) == _T("zip")) {
                CLOG(INFO, "sync") << "Found app: " + UTF8Convert::WideToUTF8(packageFilename);

                // Check for json file containing metadata
                CString jsonFilename = PortableFunctions::PathRemoveFileExtension<CString>(packageFilename) + ".json";
                if (std::find_if(pDirListing->begin(), pDirListing->end(), [jsonFilename](const FileInfo& fi) {return fi.getName() == jsonFilename; }) == pDirListing->end())
                    jsonFilename = PortableFunctions::PathRemoveFileExtension<CString>(packageFilename) + FileExtensions::WithDot::DeploySpec; //  old versions used csds instead of json

                if (std::find_if(pDirListing->begin(), pDirListing->end(), [jsonFilename](const FileInfo& fi) {return fi.getName() == jsonFilename; }) != pDirListing->end()) {

                    try {
                        std::ostringstream infoStream;
                        m_pServerConnection->getStream(packagePath + jsonFilename, infoStream);
                        ApplicationPackage package = m_jsonConverter.applicationPackageFromJson(infoStream.str());
                        packages.push_back(package);
                    }
                    catch (const SyncError& e) {
                        // Just ignore the package if files are missing or unreadable
                        CLOG(WARNING, "sync") << "Error reading package info file: " << e.what() << " : " << UTF8Convert::WideToUTF8(jsonFilename);
                    }
                } else {
                    CLOG(WARNING, "sync") << "Found package but missing info file: " <<  UTF8Convert::WideToUTF8(jsonFilename);
                }
            }
        }
    }

    return packages;
}

template<class T>
bool FileBasedSmartSyncer<T>::downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage) {

    // Check to see if we already have the current version
    if (currentPackage) {

        const auto server_packages = listApplicationPackages();
        auto server_package = std::find_if(server_packages.begin(), server_packages.end(),
                                           [&packageName](const ApplicationPackage &p) {
                                               return p.getName() == packageName;
                                           });
        if (server_package != server_packages.end()) {
            if (currentPackage->getBuildTime() >= server_package->getBuildTime())
                // Local package is same or newer than server package
                return false;
        }
    }

    CString packagePath = getAppPath();
    CString packageFilename = packageNameToFileName(packageName);
    m_pServerConnection->getFile(packagePath + packageFilename + _T(".zip"), localPath, CString());

    return true;
}

template<class T>
void FileBasedSmartSyncer<T>::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    CString packagePath = getAppPath();
    CString packageFilename = packageNameToFileName(packageName);
    m_pServerConnection->putFile(localPath, packagePath + packageFilename + _T(".zip"));
    std::string packageSpecJsonUtf8 = UTF8Convert::WideToUTF8(packageSpecJson);
    std::istringstream is(packageSpecJsonUtf8);
    m_pServerConnection->putStream(is, packageSpecJsonUtf8.size(), packagePath + packageFilename + _T(".json"));
}

template<class T>
CString FileBasedSmartSyncer<T>::packageNameToFileName(CString packageName)
{
    return ReplaceInvalidFileChars(packageName, _T('_'));
}
