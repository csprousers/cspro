#include "stdafx.h"
#include "SyncObexHandler.h"
#include "ApplicationPackageManager.h"
#include "BluetoothChunk.h"
#include "ConnectResponse.h"
#include "IDataRepositoryRetriever.h"
#include "JsonConverter.h"
#include "SyncCustomHeaders.h"
#include "SyncException.h"
#include "SyncLogCaseConstructionReporter.h"
#include "SyncRequest.h"
#include <zToolsO/base64.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zUtilO/Versioning.h>
#include <zZipo/IZip.h>
#include <zZipo/ZipUtility.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryTransaction.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zParadataO/Logger.h>
#include <zParadataO/Syncer.h>
#include <easyloggingwrapper.h>
#include <fstream>
#include <sstream>
#include <utility>


namespace {

    JsonConverter jsonConverter;

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

    // Check if child path is contained inside parentPath or one of its
    // descendants. Assumes that both paths are relative and that
    // parentPath is canonical.
    bool isDescendantDirectory(CString parentPath, CString childPath)
    {
        CString canonicalChild;
        bool canonOk = PathCanonicalize(canonicalChild.GetBuffer(MAX_PATH), childPath) == TRUE;
        canonicalChild.ReleaseBuffer();
        if (!canonOk)
            return false;
        return canonicalChild.Find(parentPath) == 0;
    }

    bool createCSEntryPath(CString rootPath, CString& csentryPath)
    {
        // The rootPath must include the directory csentry in its path or the function will
        // return false. The csentryPath will contain the path up to and including csentry.
        // For example, the root path /foo/csentry/bar/ becomes /foo/csentry/.
        bool success = true;
        CString resultToken;
        int curPos = 0;

        while (curPos != -1 && resultToken != _T("csentry"))
        {
            resultToken = rootPath.Tokenize(_T("/"), curPos);

            if (resultToken == _T("csentry"))
            {
                csentryPath = rootPath.Left(curPos);
            }
            else if (curPos == -1)
            {
                // csentry folder doesn't exist
                success = false;
            }
        }

        return success;
    }

    std::streamsize getStreamSize(std::istream& iStream)
    {
        iStream.seekg(0, std::ios::end);
        const std::streamsize streamSize = iStream.tellg();
        iStream.seekg(0, std::ios::beg);

        return streamSize;
    }

    float GetClientCSProVersion(const HeaderList& headers)
    {
        CString userAgent = headers.value("User-Agent");
        const CString userAgentHeaderStart("CSPro sync client/");
        if (userAgent.Left(userAgentHeaderStart.GetLength()) == userAgentHeaderStart) {
            //version string will be a detailed version string of CSPro without the prefix like 7.7.3
            std::string versionString = UTF8Convert::WideToUTF8(userAgent.Mid(userAgentHeaderStart.GetLength()));
            std::regex  pattern{R"(\d+\.?\d+)"};
            std::smatch matches;
            //convert the major and minor version to decimal (do not include the patch release)
            if (std::regex_search(versionString, matches, pattern)) {
                return (float)atod(UTF8Convert::UTF8ToWide(matches[0].str()));
            }
        }
        return 0.0f;
    }

    bool UseCSPro74Compatability(const HeaderList& headers)
    {
        return GetClientCSProVersion(headers) <= 7.5;
    }

    /// <summary>Obex resource for a string constant</summary>
    class StringResource : public IObexResource {

    public:

        StringResource(const std::string& s, HeaderList response_headers = HeaderList())
            : m_size(s.size()),
              m_stream(s),
              m_responseHeaders(std::move(response_headers))
        {
        }

        ObexResponseCode openForWriting() override
        {
            return OBEX_FORBIDDEN;
        }

        ObexResponseCode openForReading() override
        {
            return OBEX_OK;
        }

        ObexResponseCode close() override
        {
            return OBEX_OK;
        }

        int64_t getTotalSize() override
        {
            return m_size;
        }

        std::istream* getIStream() override
        {
            return &m_stream;
        }

        std::ostream* getOStream() override
        {
            return nullptr;
        }

        const HeaderList& getHeaders() const override
        {
            return m_responseHeaders;
        }

    private:
        int64_t m_size;
        std::istringstream m_stream;
        HeaderList m_responseHeaders;
    };

    /// <summary>Obex resource for a getting a file</summary>
    class FileReadResource : public IObexResource {
    public:

        explicit FileReadResource(const CString& filename, const HeaderList& response_headers = HeaderList())
            : m_filename(filename),
              m_responseHeaders(response_headers)
        {
        }

        ObexResponseCode openForWriting() override
        {
            return OBEX_NOT_IMPLEMENTED;
        }

        ObexResponseCode openForReading() override
        {
            if (!m_fileStream.is_open()) {
                m_fileStream.open(m_filename, std::ifstream::in | std::ifstream::binary);
                CLOG(INFO, "sync") << "FileGetResource: open " << UTF8Convert::WideToUTF8(m_filename);
            }

            CLOG(INFO, "sync") << "FileGetResource: openForReading";

            if (!m_fileStream) {
                return OBEX_FORBIDDEN;
            }

            std::string chunkString;
            bool isLastFileChunk = readFileChunk(chunkString);
            CLOG(INFO, "sync") << "FileGetResource: sending " << chunkString.size();
            compress(chunkString);
            CLOG(INFO, "sync") << "FileGetResource: compressed " << chunkString.size();
            std::istringstream tempChunkStream(chunkString);
            m_chunkStream.swap(tempChunkStream);
            CLOG(INFO, "sync") << "isLastFileChunk " << isLastFileChunk;

            return isLastFileChunk ? OBEX_IS_LAST_FILE_CHUNK : OBEX_OK;
        }

        ObexResponseCode close() override
        {
            return OBEX_OK;
        }

        int64_t getTotalSize() override
        {
            return getStreamSize(m_chunkStream);
        }

        std::istream* getIStream() override
        {
            return &m_chunkStream;
        }

        std::ostream* getOStream() override
        {
            return nullptr;
        }

        const HeaderList& getHeaders() const override
        {
            return m_responseHeaders;
        }

    private:

        bool readFileChunk(std::string& chunkString)
        {
            bool isLastFileChunk = false;
            const std::vector<char>::size_type bufferSize = BluetoothFileChunk::size;
            std::streamsize chunkSize = bufferSize;
            std::vector<char> chunk(bufferSize, 0);

            m_fileStream.read(chunk.data(), bufferSize);

            if (m_chunkStream.bad()) {
                throw SyncError(100134);
            }
            else if (m_fileStream.eof()) {
                isLastFileChunk = true;
                chunkSize = m_fileStream.gcount();
            }

            chunkString.assign(chunk.data(), static_cast<size_t>(chunkSize));
            return isLastFileChunk;
        }

        void compress(std::string& data)
        {
            if (ZipUtility::compression(data)) {
                CLOG(ERROR, "sync") << "Compression failed";
                throw SyncError(100138);
            }
        }

        std::ifstream m_fileStream;
        std::istringstream m_chunkStream;
        HeaderList m_responseHeaders;
        int m_csproClientVersion;

    protected:
        CString m_filename;
    };

    /// <summary>Obex file resource for use with temp files. Deletes the file when done reading.</summary>
    class TemporaryFileReadResource : public FileReadResource {
    public:
        explicit TemporaryFileReadResource(const CString& filename)
        : FileReadResource(filename)
        {}

        virtual ~TemporaryFileReadResource()
        {
            PortableFunctions::FileDelete(m_filename);
        }
    };

    /// <summary>Obex resource for a putting a file</summary>
    class FileWriteResource : public IObexResource
    {
    public:

        FileWriteResource(const CString& filename)
            : m_filename(filename)
        {}

        ObexResponseCode openForWriting() override
        {
            if (m_tempFile != nullptr && PortableFunctions::FileExists(m_tempFile->GetPath())) {
                // The temporary file already exists
                return OBEX_OK;
            }
            else {
                m_tempFile = std::make_unique<TemporaryFile>(PortableFunctions::PathGetDirectory<CString>(m_filename));
                m_fileStream.open(m_tempFile->GetPath().c_str(), std::ofstream::out | std::ofstream::app | std::ofstream::binary);
                return !m_fileStream ? OBEX_FORBIDDEN : OBEX_OK;
            }
        }

        ObexResponseCode openForReading() override
        {
            // Allow opening for read but return zero bytes - this way
            // it acts just like the sync resource with an empty response
            return OBEX_OK;
        }

        ObexResponseCode close() override
        {
            m_fileStream.close();
            if (m_tempFile->Rename(CS2WS(m_filename)))
                return OBEX_OK;
            else
                return OBEX_FORBIDDEN;
        }

        int64_t getTotalSize() override
        {
            return 0;
        }

        std::istream* getIStream() override
        {
            return nullptr;
        }

        std::ostream* getOStream() override
        {
            return &m_fileStream;
        }

        const HeaderList& getHeaders() const override
        {
            return m_responseHeaders;
        }

    private:
        CString m_filename;
        std::unique_ptr<TemporaryFile> m_tempFile;
        std::ofstream m_fileStream;
        HeaderList m_responseHeaders;
    };

    /// <summary>Obex resource for a syncing (GET) a dictionary</summary>
    class DictionaryReadResource : public IObexResource {
    public:

        DictionaryReadResource(ISyncableDataRepository* pRepo, HeaderList requestHeaders) :
            m_pRepo(pRepo),
            m_requestHeaders(std::move(requestHeaders))
        {
        }

        ObexResponseCode openForWriting() override
        {
            return OBEX_NOT_IMPLEMENTED;
        }

        ObexResponseCode openForReading() override
        {
            ObexResponseCode ifMatchResult = checkIfMatchHeader();
            if (ifMatchResult != OBEX_OK)
                return ifMatchResult;
            return performSync();
        }

        ObexResponseCode close() override
        {
            return OBEX_OK;
        }

        int64_t getTotalSize() override
        {
            const std::streampos current = m_pResponseJsonStream.tellg();
            m_pResponseJsonStream.seekg(0, std::ios::end);
            const std::streampos size = m_pResponseJsonStream.tellg();
            m_pResponseJsonStream.seekg(current, std::ios::beg);

            return size;
        }

        std::istream* getIStream() override
        {
            return &m_pResponseJsonStream;
        }

        std::ostream* getOStream() override
        {
            return nullptr;
        }

        const HeaderList& getHeaders() const override
        {
            return m_responseHeaders;
        }

    private:

        ObexResponseCode checkIfMatchHeader()
        {
            m_lastSyncRev = -1;
            CString ifMatch = m_requestHeaders.value(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER);
            if (!ifMatch.IsEmpty()) {
                int serverRevNum = _ttoi((LPCTSTR) ifMatch);
                if (!m_pRepo->IsValidFileRevision(serverRevNum)) {
                    CLOG(INFO, "sync") << "Revision " << serverRevNum << " not found. Need to do a full sync";
                    return OBEX_PRECONDITION_FAILED;
                }
                m_lastSyncRev = serverRevNum;
            }

            return OBEX_OK;
        }

        ObexResponseCode performSync()
        {
            CString remoteDeviceId = m_requestHeaders.value(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (remoteDeviceId.IsEmpty())
                return OBEX_BAD_REQUEST;

            CString universe = m_requestHeaders.value(SyncCustomHeaders::UNIVERSE_HEADER);
            CString startAfter = m_requestHeaders.value(SyncCustomHeaders::START_AFTER_HEADER);
            CString rangeCount = m_requestHeaders.value(SyncCustomHeaders::RANGE_COUNT_HEADER);
            CString excludeRevisionsString = m_requestHeaders.value(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER);
            std::vector<std::wstring> excludeRevisions = SO::SplitString(excludeRevisionsString, ',');

            CLOG(INFO, "sync") << "Get: remote device " << UTF8Convert::WideToUTF8(remoteDeviceId)
                << ", universe \"" << UTF8Convert::WideToUTF8(universe) << "\""
                << ", startAfter \"" << UTF8Convert::WideToUTF8(startAfter) << "\""
                << ", count \"" << UTF8Convert::WideToUTF8(rangeCount) << "\"";

            if (m_lastSyncRev != -1) {
                CLOG(INFO, "sync") << "Last synced file with this device at revision " << m_lastSyncRev;
                SyncHistoryEntry lastSyncRev = m_pRepo->GetLastSyncForDevice(remoteDeviceId, SyncDirection::Put);
                bool clearBinarySyncHistory = true;
                if (lastSyncRev.valid()) {
                    clearBinarySyncHistory = false;
                    // only use the previous revision number if the universe and last case id are matching
                    if (universe != lastSyncRev.getUniverse() || startAfter != lastSyncRev.getLastCaseUuid())
                        clearBinarySyncHistory = true;
                }
                if (clearBinarySyncHistory) {
                    m_pRepo->ClearBinarySyncHistory(remoteDeviceId, m_lastSyncRev);
                    CLOG(INFO, "sync") << "Revision not found. Clearing binary sync history for device: " << UTF8Convert::WideToUTF8(remoteDeviceId);
                }
            } else {
                m_pRepo->ClearBinarySyncHistory(remoteDeviceId);
                CLOG(INFO, "sync") << "First time sync with this device";
            }

            // Get max cases to send from count header or send all if header not present
            int chunkSize = INT_MAX;
            if (!rangeCount.IsEmpty()) {
                chunkSize = _ttoi(rangeCount);
                if (chunkSize <= 0) {
                    CLOG(ERROR, "sync") << "Invalid range count header";
                    return OBEX_BAD_REQUEST;
                }
            }

            int nResponseCases, maxRevisionInChunk;
            std::shared_ptr<CaseIterator> case_iterator = m_pRepo->GetCasesModifiedSinceRevisionIterator(
                    m_lastSyncRev,
                    startAfter,
                    universe, chunkSize, &nResponseCases,
                    &maxRevisionInChunk,
                    CString(),
                    excludeRevisions);

            std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>> binary_case_items_in_chunk;
            uint64_t totalBinaryItemsByteSize = 0;
            std::set<std::string> excludeKeys;

            // read the cases in this chunk
            std::vector<std::shared_ptr<Case>> cases_in_chunk;
            cases_in_chunk.emplace_back(m_pRepo->GetCaseAccess()->CreateCase());

            while( case_iterator->NextCase(*cases_in_chunk.back()) ) {
                const auto& currentCase = cases_in_chunk.back().get();
                m_pRepo->GetBinaryCaseItemsModifiedSinceRevision(currentCase, binary_case_items_in_chunk, excludeKeys, totalBinaryItemsByteSize, remoteDeviceId);
                cases_in_chunk.emplace_back(m_pRepo->GetCaseAccess()->CreateCase());
                //process a subset of the chunk when the totalBinaryItemsByteSize exceeds a set limit
                if (totalBinaryItemsByteSize > BluetoothFileChunk::size) {
                    chunkSize = cases_in_chunk.size() - 1;
                    //get the max revision number at this chunksize
                    m_pRepo->GetCasesModifiedSinceRevisionIterator(
                        m_lastSyncRev,
                        startAfter,
                        universe, chunkSize, &nResponseCases,
                        &maxRevisionInChunk,
                        CString(),
                        excludeRevisions);
                    CLOG(INFO, "sync") << "Binary items in the chunk exceeded the set limit of chunk byte content size. Sending a subchunk of cases: " << chunkSize << " cases";
                    break;
                }
            }

            cases_in_chunk.pop_back();
            int casesThisChunk = std::min(chunkSize, nResponseCases);


            CLOG(INFO, "sync") << "Sending " << casesThisChunk << "/" << nResponseCases << " cases";

            CString etag;
            etag.Format(_T("ETag: %d"), maxRevisionInChunk);
            m_responseHeaders.push_back(etag);
            CLOG(INFO, "sync") <<  UTF8Convert::WideToUTF8(etag);

            if (!rangeCount.IsEmpty()) {
                CString rangeCountResponse;
                rangeCountResponse.Format(_T("%s: %d/%d"), (LPCTSTR) SyncCustomHeaders::RANGE_COUNT_HEADER, casesThisChunk, nResponseCases);
                m_responseHeaders.push_back(rangeCountResponse);
            }

            
            std::string responseJson = jsonConverter.toJson(cases_in_chunk, UseCSPro74Compatability(m_requestHeaders));

            if (binary_case_items_in_chunk.size() > 0) { //write the new binary json format only if cases have binary items
                std::ostringstream binary_cases_json_stream;
                JsonConverter converter;
                //cases have binary content send the questionnaire data using binary format
                converter.writeBinaryJson(binary_cases_json_stream, responseJson, binary_case_items_in_chunk);
                responseJson = binary_cases_json_stream.str();
            }
            CLOG(INFO, "sync") << "Binary items in the chunk: " << binary_case_items_in_chunk.size();

            if (ZipUtility::compression(responseJson)) {
                CLOG(ERROR, "sync") << "Compression failed";
                throw SyncError(100138);
            }
            //previous versions of sync did not have a record of the cases sent from the bluetooth server
            //the client kept track of what it received. However, with binary items sync
            //the server needs to keep track of the binary items sent from a get request from the client to avoid
            //sending duplicates. Mark cases sent to the server here and if the client does not received this
            //and resends a get request, when the entry that does not match the request for last case id it will archive
            //the binary items sent at this version in the history and resend the items
            //use the max revision of this chunk as the file_revision of the sync history entry
            //use the remote client id as the "server" device id
            //direction is put from the bluetooth server to the remote device
            //clientRevision stores the version that the server sent to the client. Leaving the server version as blank as it not used.
            const bool bUpdateOnConflict = false;
            m_pRepo->StartSync(remoteDeviceId, CString(), CString(), SyncDirection::Put, universe, bUpdateOnConflict);
            m_pRepo->MarkCasesSentToRemote(cases_in_chunk, binary_case_items_in_chunk, CString(), maxRevisionInChunk);
            m_pResponseJsonStream.str(responseJson);

            ObexResponseCode response = (casesThisChunk >= nResponseCases) ? OBEX_OK : OBEX_PARTIAL_CONTENT;
            //set the sync history to revision complete when all cases are sent 
            if(response == OBEX_OK)
                m_pRepo->EndSync();

            CLOG(INFO, "sync") << "Response: " << UTF8Convert::WideToUTF8(ObexResponseCodeToString(response));

            return response;
        }

        ISyncableDataRepository* m_pRepo;
        HeaderList m_requestHeaders;
        HeaderList m_responseHeaders;
        std::istringstream m_pResponseJsonStream;
        int m_lastSyncRev;
    };


    /// <summary>Obex resource for a syncing (PUT) a dictionary</summary>
    class DictionaryWriteResource : public IObexResource
    {
    public:

        DictionaryWriteResource(ISyncableDataRepository* pRepo, HeaderList requestHeaders) :
            m_pRepo(pRepo),
            m_requestHeaders(std::move(requestHeaders))
        {}

        ObexResponseCode openForWriting() override
        {
            return checkIfMatchHeader();
        }

        ObexResponseCode openForReading() override
        {
            return performSync();
        }

        ObexResponseCode close() override
        {
            return OBEX_OK;
        }

        int64_t getTotalSize() override
        {
            return -1;
        }

        std::istream* getIStream() override
        {
            return nullptr;
        }

        std::ostream* getOStream() override
        {
            return &m_requestJsonStream;
        }

        const HeaderList& getHeaders() const override
        {
            return m_responseHeaders;
        }

    private:

        ObexResponseCode checkIfMatchHeader()
        {
            m_lastSyncRev = -1;
            CString ifMatch = m_requestHeaders.value(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER);
            CString deviceId = m_requestHeaders.value(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (!ifMatch.IsEmpty()) {
                int serverRevNum = _ttoi((LPCTSTR) ifMatch);
                if (!m_pRepo->IsPreviousSync(serverRevNum, deviceId)) {
                    CLOG(INFO, "sync") << "Revision " << serverRevNum << " not found. Need to do a full sync";
                    return OBEX_PRECONDITION_FAILED;
                }
                m_lastSyncRev = serverRevNum;
            }

            return OBEX_OK;
        }


        ObexResponseCode performSync()
        {
            CString remoteDeviceId = m_requestHeaders.value(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (remoteDeviceId.IsEmpty())
                return OBEX_BAD_REQUEST;
            CString universe = m_requestHeaders.value(SyncCustomHeaders::UNIVERSE_HEADER);

            CLOG(INFO, "sync") << "Put: remote device " << UTF8Convert::WideToUTF8(remoteDeviceId)
                << ", universe \"" << UTF8Convert::WideToUTF8(universe) << "\"";
            std::string requestString = m_requestJsonStream.str();
            std::vector<std::shared_ptr<Case>> cases;

            //Start sync and ensure that the repo has readwrite permissions on  dictionaries
            const bool bUpdateOnConflict = true;
            m_pRepo->StartSync(remoteDeviceId, _T("Bluetooth"), CString(), SyncDirection::Get,
                universe, bUpdateOnConflict);

            try {
                //check if binary sync and read binary sync items
                std::istringstream iss;
                bool isBinaryFormat = false;
                if (jsonConverter.IsBinaryJson(requestString)) {
                    isBinaryFormat = true;
                    iss.str(requestString);
                    requestString = jsonConverter.readCasesJsonFromBinary(iss);
                }
                cases = jsonConverter.caseListFromJson(requestString, *m_pRepo->GetCaseAccess(), std::make_shared<SyncLogCaseConstructionReporter>());
                if (isBinaryFormat) {
                    jsonConverter.readBinaryCaseItems(iss, cases);
                }
            }
            catch (const InvalidJsonException& e) {
                CLOG(ERROR, "sync") << "Failed to parse request JSON " << requestString;
                CLOG(ERROR, "sync") << e.what();
                return OBEX_BAD_REQUEST;
            }

            DataRepositoryTransaction transaction(*m_pRepo);
            //TODO: Make cases observable
            int thisSyncRev = m_pRepo->SyncCasesFromRemote(cases, CString());
            m_pRepo->EndSync();
            ISyncableDataRepository::SyncStats stats = m_pRepo->GetLastSyncStats();

            CLOG(INFO, "sync") << "Received " << stats.numReceived << " cases. " << stats.numNewCasesNotInRepo
                << " new cases, " << stats.numCasesNewerOnRemote << " updated, "
                << stats.numCasesNewerInRepo << " ignored, " << stats.numConflicts
                << " conflicts.";

            // Add new rev for this sync
            CString etag;
            etag.Format(_T("ETag: %d"), thisSyncRev);
            m_responseHeaders.push_back(etag);

            CLOG(INFO, "sync") << UTF8Convert::WideToUTF8(etag);

            return OBEX_OK;
        }

        ISyncableDataRepository* m_pRepo;
        HeaderList m_requestHeaders;
        HeaderList m_responseHeaders;
        std::ostringstream m_requestJsonStream;
        int m_lastSyncRev;
    };

    /// <summary>Obex resource for ignoring reads/writes</summary>
    class NullResource : public IObexResource
    {
    public:
        ObexResponseCode openForWriting() override    { return OBEX_OK;           }
        ObexResponseCode openForReading() override    { return OBEX_OK;           }
        ObexResponseCode close() override             { return OBEX_OK;           }
        int64_t getTotalSize() override               { return 0;                 }
        std::istream* getIStream() override           { return nullptr;           }
        std::ostream* getOStream() override           { return &m_outputStream;   }
        const HeaderList& getHeaders() const override { return m_responseHeaders; }

    private:
        HeaderList m_responseHeaders;
        std::ostringstream m_outputStream;
    };
}

SyncObexHandler::SyncObexHandler(DeviceId deviceId, IDataRepositoryRetriever* pRepoRetriever,
    CString fileRoot, ISyncEngineFunctionCaller* sync_engine_function_caller)
    : m_deviceId(deviceId),
      m_pDataRepositoryRetriever(pRepoRetriever),
      m_rootPath(PortableFunctions::PathEnsureTrailingSlash(PortableFunctions::PathToNativeSlash(fileRoot))),
      m_syncEngineFunctionCaller(sync_engine_function_caller)
{
    CLOG(INFO, "sync") << "Creating Bluetooth Server handler";
    CLOG(INFO, "sync") << "Device id: " << UTF8Convert::WideToUTF8(m_deviceId);
    CLOG(INFO, "sync") << "Root folder: " << UTF8Convert::WideToUTF8(m_rootPath);
}

SyncObexHandler::~SyncObexHandler()
{
}

ObexResponseCode SyncObexHandler::onConnect(const char* target, int sizeBytes)
{
    if (sizeBytes > 0 && (strncmp(reinterpret_cast<const char*>(OBEX_FOLDER_BROWSING_UUID),
        target, std::min((size_t)sizeBytes, sizeof(OBEX_FOLDER_BROWSING_UUID))) == 0))
        return OBEX_OK;
    else
        return OBEX_SERVICE_UNAVAILABLE;
}

ObexResponseCode SyncObexHandler::onDisconnect()
{
    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::onGet(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    if (name.IsEmpty()) {
        // Empty name in Obex means get the default resource which for us
        // will be the server info sent in connect response.
        assert(resource.get() == nullptr);
        std::regex  pattern{ R"(\d+\.?\d+)" };
        std::smatch matches;
        float apiVersion = 0.0f;
        //convert the major and minor version to decimal (do not include the patch release)
        std::string versionString = UTF8Convert::WideToUTF8(Versioning::GetVersionString());
        if (std::regex_search(versionString, matches, pattern)) {
            apiVersion = (float)atod(UTF8Convert::UTF8ToWide(matches[0].str()));
        }
        ConnectResponse connectResponse(m_deviceId, CString("Bluetooth"), CString(), apiVersion);
        std::string responseJson = jsonConverter.toJson(connectResponse);
        resource = std::unique_ptr<IObexResource>(new StringResource(responseJson));

        CLOG(INFO, "sync") << "Received connection";

        return OBEX_OK;
    }

    if (type == OBEX_SYNC_DATA_MEDIA_TYPE) {
        return handleSyncGet(CS2WS(name), requestHeaders, resource);
    } else if (type == OBEX_DIRECTORY_LISTING_MEDIA_TYPE) {
        return handleDirectoryListing(name, resource);
    } else if (type == OBEX_SYNC_APP_MEDIA_TYPE) {
        return handleSyncApp(name, requestHeaders, resource);
    } else if (type == OBEX_SYNC_MESSAGE_MEDIA_TYPE) {
        return handleSyncMessage(requestHeaders, resource);
    } else if (type == OBEX_SYNC_PARADATA_SYNC_HANDSHAKE) {
        return handleSyncParadataStart(requestHeaders, resource);
    } else if (type == OBEX_SYNC_PARADATA_TYPE) {
        return handleSyncParadataGet(requestHeaders, resource);
    } else if (type == OBEX_BINARY_FILE_MEDIA_TYPE) {
        //  Regular file get
        return handleFileGet(name, requestHeaders, resource);
    }

    return OBEX_NOT_IMPLEMENTED;
}

ObexResponseCode SyncObexHandler::onPut(CString type, CString name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    if (type == OBEX_SYNC_DATA_MEDIA_TYPE) {
        return handleSyncPut(CS2WS(name), requestHeaders, resource);
    } else if (type == OBEX_SYNC_PARADATA_SYNC_HANDSHAKE) {
        return handleSyncParadataStop(requestHeaders, resource);
    } else if (type == OBEX_SYNC_PARADATA_TYPE) {
        return handleSyncParadataPut(requestHeaders, resource);
    } else if (type == OBEX_BINARY_FILE_MEDIA_TYPE) {
        // Regular file put
        return handleFilePut(name, resource);
    }

    return OBEX_NOT_IMPLEMENTED;
}

ObexResponseCode SyncObexHandler::handleSyncPut(const std::wstring& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    assert(resource.get() == nullptr);
    CLOG(INFO, "sync") << "Sync put " << UTF8Convert::WideToUTF8(dictionary_name);

    DataRepository* pRepo = m_pDataRepositoryRetriever->get(dictionary_name);
    if (pRepo == nullptr) {
        CLOG(ERROR, "sync") << "Error: dictionary " << UTF8Convert::WideToUTF8(dictionary_name) << " not found in current application";
        return OBEX_NOT_FOUND;
    }

    ISyncableDataRepository* pSyncableRepo = pRepo->GetSyncableDataRepository();

    if (pSyncableRepo == nullptr) {
        CLOG(ERROR, "sync") << "Error: dictionary " << UTF8Convert::WideToUTF8(dictionary_name) << " is not syncable";
        return OBEX_BAD_REQUEST;
    }

    resource = std::unique_ptr<IObexResource>(new DictionaryWriteResource(pSyncableRepo, requestHeaders));
    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleSyncGet(const std::wstring& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    assert(resource.get() == nullptr);
    CLOG(INFO, "sync") << "Sync get " << UTF8Convert::WideToUTF8(dictionary_name);

    DataRepository* pRepo = m_pDataRepositoryRetriever->get(dictionary_name);
    if (pRepo == nullptr) {
        CLOG(ERROR, "sync") << "Error: dictionary " << UTF8Convert::WideToUTF8(dictionary_name) << " not found in current application";
        return OBEX_NOT_FOUND;
    }

    ISyncableDataRepository* pSyncableRepo = pRepo->GetSyncableDataRepository();

    if (pSyncableRepo == nullptr) {
        CLOG(ERROR, "sync") << "Error: dictionary " << UTF8Convert::WideToUTF8(dictionary_name) << " is not syncable";
        return OBEX_BAD_REQUEST;
    }

    resource = std::unique_ptr<IObexResource>(new DictionaryReadResource(pSyncableRepo, requestHeaders));
    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleDirectoryListing(CString path, std::unique_ptr<IObexResource>& resource)
{
    assert(resource.get() == nullptr);
    CLOG(INFO, "sync") << "Listing directory " << UTF8Convert::WideToUTF8(path);

    CString fullDirectoryPath = PortableFunctions::PathAppendToPath(m_rootPath, PortableFunctions::PathToNativeSlash(path));
    CString canonicalDirectoryPath;
    bool canonOk = PathCanonicalize(canonicalDirectoryPath.GetBuffer(MAX_PATH), fullDirectoryPath) == TRUE;
    canonicalDirectoryPath.ReleaseBuffer();
    if (!canonOk) {
        CLOG(ERROR, "sync") << "Error: Invalid directory " << UTF8Convert::WideToUTF8(fullDirectoryPath);
        return OBEX_NOT_FOUND;
    }

    if (canonicalDirectoryPath[canonicalDirectoryPath.GetLength() - 1] != PATH_CHAR)
        canonicalDirectoryPath += PATH_CHAR;

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a GET that is outside both root and csentry paths
    if (!isDescendantDirectory(m_rootPath, canonicalDirectoryPath)
        && (!createCSEntryPath(m_rootPath, csentryPath) || !isDescendantDirectory(csentryPath, canonicalDirectoryPath))) {
        CLOG(ERROR, "sync") << "Error: Directory " << UTF8Convert::WideToUTF8(canonicalDirectoryPath) << " is outside of both the project root directory "
            << UTF8Convert::WideToUTF8(m_rootPath) << " and the csentry root directory " << UTF8Convert::WideToUTF8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    if (!PortableFunctions::FileIsDirectory(canonicalDirectoryPath)) {
        CLOG(ERROR, "sync") << "Error: " << UTF8Convert::WideToUTF8(canonicalDirectoryPath) << " is a file not a directory";
        return OBEX_NOT_FOUND;
    }

    std::vector<std::wstring> aFileNames = DirectoryLister().SetIncludeDirectories()
                                                            .GetPaths(canonicalDirectoryPath);

    // Store the relative path that when concatenated with the root path will construct the full path
    CString pathFromRoot = PortableFunctions::PathToForwardSlash(path);
    if (pathFromRoot.Left(2) != _T("./") && pathFromRoot.Left(3) != _T("../")) {
        pathFromRoot = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", pathFromRoot);
    }

    std::vector<FileInfo> listing;
    for (const std::wstring& fullPath : aFileNames) {
        CString filename = PortableFunctions::PathRemoveTrailingSlash<CString>(PortableFunctions::PathGetFilename(fullPath));
        FileInfo::FileType type = PortableFunctions::FileIsDirectory(fullPath) ? FileInfo::FileType::Directory : FileInfo::FileType::File;
        if (type == FileInfo::FileType::File) {
            int64_t size = PortableFunctions::FileSize(fullPath);
            listing.emplace_back(type, filename, pathFromRoot, size, time_t(0), PortableFunctions::FileMd5(fullPath));
        } else {
            listing.emplace_back(type, filename, pathFromRoot);
        }
    }

    resource = std::unique_ptr<IObexResource>(new StringResource(jsonConverter.toJson(listing)));
    CLOG(INFO, "sync") << "Found " << listing.size() << " files in directory " << UTF8Convert::WideToUTF8(canonicalDirectoryPath);

    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleFileGet(CString path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    if (resource.get() != nullptr) {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    CString fullPath = PortableFunctions::PathAppendToPath(m_rootPath, PortableFunctions::PathToNativeSlash(path));
    CLOG(INFO, "sync") << "Sending file " << UTF8Convert::WideToUTF8(fullPath);

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a GET that is outside both root and csentry paths
    if (!isDescendantDirectory(m_rootPath, fullPath)
        && (!createCSEntryPath(m_rootPath, csentryPath) || !isDescendantDirectory(csentryPath, fullPath))) {
        CLOG(ERROR, "sync") << "Error: File " << UTF8Convert::WideToUTF8(fullPath) << " is outside of both the project root directory "
            << UTF8Convert::WideToUTF8(m_rootPath) << " and the csentry root directory " << UTF8Convert::WideToUTF8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    if (!PortableFunctions::FileExists(fullPath)) {
        CLOG(ERROR, "sync") << "Error: File " << UTF8Convert::WideToUTF8(fullPath) << " does not exist";
        return OBEX_NOT_FOUND;
    }

    if (PortableFunctions::FileIsDirectory(fullPath)) {
        CLOG(ERROR, "sync") << "Error: File " << UTF8Convert::WideToUTF8(fullPath) << " is a directory, not a regular file";
        return OBEX_NOT_FOUND;
    }

    CString ifNoneMatch = requestHeaders.value(_T("If-None-Match"));
    if (!ifNoneMatch.IsEmpty() && PortableFunctions::FileExists(fullPath)) {
        std::wstring md5 = PortableFunctions::FileMd5(fullPath);
        if (SO::EqualsNoCase(md5, ifNoneMatch)) {
            CLOG(INFO, "sync") << "File not modified. Skipping.";
            return OBEX_NOT_MODIFIED;
        }
    }

    resource = std::unique_ptr<IObexResource>(new FileReadResource(fullPath));
    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleFilePut(CString path, std::unique_ptr<IObexResource>& resource)
{
    if (resource.get() != nullptr) {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    CString fullPath = PortableFunctions::PathAppendToPath(m_rootPath, PortableFunctions::PathToNativeSlash(path));
    CLOG(INFO, "sync") << "Receiving file " << UTF8Convert::WideToUTF8(fullPath);

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a PUT that is outside both root and csentry paths
    if (!isDescendantDirectory(m_rootPath, fullPath)
        && (!createCSEntryPath(m_rootPath, csentryPath) || !isDescendantDirectory(csentryPath, fullPath))) {
        CLOG(ERROR, "sync") << "Error: File " << UTF8Convert::WideToUTF8(fullPath) << " is outside of both the project root directory "
            << UTF8Convert::WideToUTF8(m_rootPath) << " and the csentry root directory " << UTF8Convert::WideToUTF8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    // Create path if it doesn't exist
    CString dir = PortableFunctions::PathGetDirectory<CString>(fullPath);
    if (!PortableFunctions::FileExists(dir)) {
        if (!PortableFunctions::PathMakeDirectories(dir)) {
            CLOG(ERROR, "sync") << "Error: Failed to create directory " << UTF8Convert::WideToUTF8(dir);
            return OBEX_FORBIDDEN;
        }
    }

    resource = std::unique_ptr<IObexResource>(new FileWriteResource(fullPath));
    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleSyncApp(const CString& app_name, const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
#ifdef ANDROID
    auto signed_package = ApplicationPackageManager::getInstalledApplicationPackageWithSignature(app_name);
    if (!signed_package) {
        CLOG(ERROR, "sync") << "Error: Application Package " << UTF8Convert::WideToUTF8(app_name) << " is not installed";
        return OBEX_NOT_FOUND;
    }

    auto request_signature = request_headers.value(_T("If-None-Match"));
    if (!request_signature.IsEmpty() && request_signature.CompareNoCase(signed_package->signature) == 0) {
        CLOG(INFO, "sync") << "Application signature matches installed version. Skipping.";
        return OBEX_NOT_MODIFIED;
    }

    auto request_build_time_header = request_headers.value(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER);
    if (!request_build_time_header.IsEmpty()) {
        auto request_build_time = PortableFunctions::ParseRFC3339DateTime(CS2WS(request_build_time_header));
        if (request_build_time > 0 && request_build_time >= signed_package->package.getBuildTime()) {
            CLOG(INFO, "sync") << "Application build time less than currently installed. Skipping.";
            return OBEX_NOT_MODIFIED;
        }
    }

    auto request_files_header = request_headers.value(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER);
    auto request_files_json = Base64::Decode<std::string>(UTF8Convert::WideToUTF8(request_files_header));
    ZipUtility::decompression(request_files_json);
    auto request_files = jsonConverter.fileSpecListFromJson(request_files_json);

    std::vector<CString> files_to_exclude;
    for (const auto& package_file : signed_package->package.getFiles()) {

        auto match = std::find_if(request_files.begin(), request_files.end(), [& package_file](const auto& f) {return package_file.Path.CompareNoCase(f.Path) == 0;});
        if (match != request_files.end() && (match->Signature.CompareNoCase(package_file.Signature) == 0 || package_file.OnlyOnFirstInstall)) {
            CString path_in_zip;
            PathCanonicalize(path_in_zip.GetBuffer(MAX_PATH), package_file.Path);
            path_in_zip.ReleaseBuffer();
            path_in_zip.Replace(_T("/"), _T("\\"));
            if (path_in_zip.Left(2) == L".\\")
                path_in_zip = path_in_zip.Right(path_in_zip.GetLength() - 2);
            files_to_exclude.emplace_back(path_in_zip);
        }
    }

    const auto& application_package_zip_path = ApplicationPackageManager::getPackageZipPath(app_name);
    if (files_to_exclude.empty()) {
        resource = std::unique_ptr<IObexResource>(new FileReadResource(application_package_zip_path));
    } else {
        CString temp_zip_path = PortableFunctions::FileTempName(GetTempDirectory());
        PortableFunctions::FileCopy(application_package_zip_path, temp_zip_path, false);
        std::unique_ptr<IZip> pZip = std::unique_ptr<IZip>(IZip::Create());
        size_t filesDeleted = pZip->RemoveFiles(temp_zip_path, files_to_exclude);
        if(filesDeleted != files_to_exclude.size() ){
            CLOG(INFO, "sync") << "Total files removed from zip does not match the number of files to be excluded. Application package zip may be a subset of the full package.";
        }
        resource = std::unique_ptr<IObexResource>(new TemporaryFileReadResource(temp_zip_path));
    }

    return OBEX_OK;

#else
    UNREFERENCED_PARAMETER(resource);
    UNREFERENCED_PARAMETER(request_headers);
    UNREFERENCED_PARAMETER(app_name);

    return OBEX_NOT_IMPLEMENTED;
#endif
}


ObexResponseCode SyncObexHandler::handleSyncMessage(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
    CString message_json = request_headers.value(SyncCustomHeaders::MESSAGE_HEADER);

    if( message_json.IsEmpty() )
        return OBEX_BAD_REQUEST;

    SyncMessage sync_message;

    try
    {
        sync_message = jsonConverter.syncMessageFromJson(UTF8Convert::WideToUTF8(message_json));
    }

    catch( const InvalidJsonException& exception )
    {
        CLOG(ERROR, "sync") << "Failed to parse request JSON " << message_json;
        CLOG(ERROR, "sync") << exception.what();
        return OBEX_BAD_REQUEST;
    }

    const auto& optional_response = m_syncEngineFunctionCaller->onSyncMessage(sync_message.key, sync_message.value);

    if( !optional_response.has_value() )
        return OBEX_METHOD_NOT_ALLOWED;

    sync_message.value = *optional_response;

    resource = std::make_unique<StringResource>(jsonConverter.toJson(sync_message));

    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleSyncParadataStart(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
    CString client_log_uuid = request_headers.value(SyncCustomHeaders::PARADATA_LOG_UUID);

    if( client_log_uuid.IsEmpty() )
        return OBEX_BAD_REQUEST;

    HeaderList response_headers;

    if( Paradata::Logger::IsOpen() )
    {
        m_paradataSyncer = Paradata::Logger::GetSyncer();

        m_paradataSyncer->SetPeerLogUuid(client_log_uuid);

        response_headers.push_back(SyncCustomHeaders::PARADATA_LOG_UUID + _T(": ") + m_paradataSyncer->GetLogUuid());
    }

    resource = std::make_unique<StringResource>("", response_headers);

    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleSyncParadataPut(const HeaderList& /*request_headers*/, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(m_paradataSyncer != nullptr);

    if( resource.get() != nullptr )
    {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    CLOG(INFO, "sync") << "Receiving paradata events from the log " << UTF8Convert::WideToUTF8(m_paradataSyncer->GetPeerLogUuid());

    resource = std::make_unique<FileWriteResource>(WS2CS(m_paradataSyncer->GetFilenameForReceivedSyncableDatabase()));

    return OBEX_OK;
}

ObexResponseCode SyncObexHandler::handleSyncParadataGet(const HeaderList& /*request_headers*/, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(m_paradataSyncer != nullptr);

    if( resource.get() != nullptr )
    {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    std::optional<std::wstring> extracted_syncable_database_filename = m_paradataSyncer->GetExtractedSyncableDatabase();

    if( extracted_syncable_database_filename.has_value() )
    {
        CLOG(INFO, "sync")
            << "Sending paradata events to be added to the log "
            << UTF8Convert::WideToUTF8(m_paradataSyncer->GetPeerLogUuid());

        resource = std::make_unique<FileReadResource>(WS2CS(*extracted_syncable_database_filename));

        return OBEX_OK;
    }

    else
    {
        CLOG(INFO, "sync")
            << "Skipping sending paradata events to the log "
            << UTF8Convert::WideToUTF8(m_paradataSyncer->GetPeerLogUuid())
            << " as all events are up-to-date";

        return OBEX_NOT_MODIFIED;
    }
}

ObexResponseCode SyncObexHandler::handleSyncParadataStop(const HeaderList& /*request_headers*/, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(m_paradataSyncer != nullptr);

    m_paradataSyncer->MergeReceivedSyncableDatabases();
    m_paradataSyncer->RunPostSuccessfulSyncTasks();

    m_paradataSyncer.reset();

    resource = std::make_unique<NullResource>();

    return OBEX_OK;
}
