#include "stdafx.h"
#include "SyncObexHandler.h"
#include "ApplicationPackageManager.h"
#include "BluetoothChunk.h"
#include "JsonConverter.h"
#include "SyncMessage.h"
#include <zToolsO/base64.h>
#include <zToolsO/ObjectTransporter.h>
#include <zToolsO/SpanHelpers.h>
#include <zUtilO/Interapp.h>
#include <zNetwork/SyncCustomHeaders.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryTransaction.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncBinaryDataUploadManager.h>
#include <zDataO/SyncHistoryEntry.h>
#include <zParadataO/Logger.h>
#include <zParadataO/Syncer.h>
#include <engine/EngineDictionaryModifier.h>
#include <engine/InterpreterAccessor.h>


namespace
{
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

    /// <summary>Obex resource for a string constant</summary>
    class StringResource : public IObexResource
    {
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
    class FileReadResource : public IObexResource
    {
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
                SYNCLOG_INFO << "FileGetResource: open " << UTF8_TODO::GetUtf8(m_filename);
            }

            SYNCLOG_INFO << "FileGetResource: openForReading";

            if (!m_fileStream) {
                return OBEX_FORBIDDEN;
            }

            std::string chunkString;
            bool isLastFileChunk = readFileChunk(chunkString);
            SYNCLOG_INFO << "FileGetResource: sending " << chunkString.size();
            compress(chunkString);
            SYNCLOG_INFO << "FileGetResource: compressed " << chunkString.size();
            std::istringstream tempChunkStream(chunkString);
            m_chunkStream.swap(tempChunkStream);
            SYNCLOG_INFO << "isLastFileChunk " << isLastFileChunk;

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
            const size_t bufferSize = BluetoothDataChunk::FileChunkSize;
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
            if (!ZLib::Deflate(data)) {
                SYNCLOG_ERROR << "Compression failed";
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
    class TemporaryFileReadResource : public FileReadResource
    {
    public:
        explicit TemporaryFileReadResource(TemporaryFile temporary_file)
            :   FileReadResource(UTF8_TODO::GetCString(temporary_file.GetPath())),
                m_temporaryFile(std::move(temporary_file))
        {
        }

    private:
        TemporaryFile m_temporaryFile;
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
                m_tempFile = std::make_unique<TemporaryFile>(PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(m_filename)));
                m_fileStream.open(UTF8_TODO::GetWide(m_tempFile->GetPath()).c_str(), std::ofstream::out | std::ofstream::app | std::ofstream::binary);
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
            if (m_tempFile->Rename_noexcept(UTF8_TODO::GetUtf8(m_filename)))
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
    class DictionaryReadResource : public IObexResource
    {
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
            CString ifMatch = UTF8_TODO::GetCString(m_requestHeaders.GetValue(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER));
            if (!ifMatch.IsEmpty()) {
                int serverRevNum = _ttoi(ifMatch.GetString());
                if (!m_pRepo->IsValidClientRevision(serverRevNum)) {
                    SYNCLOG_INFO << "Revision " << serverRevNum << " not found. Need to do a full sync";
                    return OBEX_PRECONDITION_FAILED;
                }
                m_lastSyncRev = serverRevNum;
            }

            return OBEX_OK;
        }

        ObexResponseCode performSync()
        {
            DeviceId remoteDeviceId = m_requestHeaders.GetValue(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (remoteDeviceId.empty())
                return OBEX_BAD_REQUEST;

            const std::string universe = m_requestHeaders.GetValue(SyncCustomHeaders::UNIVERSE_HEADER);
            const std::string startAfter = m_requestHeaders.GetValue(SyncCustomHeaders::START_AFTER_HEADER);
            const std::string rangeCount = m_requestHeaders.GetValue(SyncCustomHeaders::RANGE_COUNT_HEADER);
            const std::vector<std::string> revisions_to_exclude = SO::SplitString(m_requestHeaders.GetValue(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER), ',');

            SYNCLOG_INFO << "Get: remote device " << remoteDeviceId
                         << ", universe \"" << universe << "\""
                         << ", startAfter \"" << startAfter << "\""
                         << ", count \"" << rangeCount << "\"";

            if (m_lastSyncRev != -1) {
                SYNCLOG_INFO << "Last synced file with this device at revision " << m_lastSyncRev;
                const std::optional<SyncHistoryEntry> last_sync_revision = m_pRepo->GetLastSyncForDevice(remoteDeviceId, SyncDirection::Put);
                bool clearBinarySyncHistory = true;
                if (last_sync_revision.has_value()) {
                    clearBinarySyncHistory = false;
                    // only use the previous revision number if the universe and last case id are matching
                    if (universe != last_sync_revision->GetUniverse() || startAfter != last_sync_revision->GetLastCaseUuid())
                        clearBinarySyncHistory = true;
                }
                if (clearBinarySyncHistory) {
                    m_pRepo->ClearBinarySyncHistory(remoteDeviceId, m_lastSyncRev);
                    SYNCLOG_INFO << "Revision not found. Clearing binary sync history for device: " << remoteDeviceId;
                }
            } else {
                SYNCLOG_INFO << "First time sync with this device (or no cases were previously sent)";
            }

            // Get max cases to send from count header or send all if header not present
            size_t chunk_size = SIZE_MAX;
            if (!rangeCount.empty()) {
                const int new_size = atoi(rangeCount.c_str());
                if (new_size <= 0) {
                    SYNCLOG_ERROR << "Invalid range count header";
                    return OBEX_BAD_REQUEST;
                }
                chunk_size = new_size;
            }

            const std::unique_ptr<SyncBinaryDataUploadManager> sync_binary_data_upload_manager =
                ( m_pRepo->GetCaseAccess().GetCaseMetadata().UsesBinaryData() ) ? std::make_unique<SyncableDataRepositorySyncBinaryDataUploadManager>(*m_pRepo, remoteDeviceId) :
                                                                                   nullptr;
            size_t nResponseCases;
            int maxRevisionInChunk;
            std::unique_ptr<CaseIterator> case_iterator = m_pRepo->GetCasesModifiedSinceRevisionIterator(m_lastSyncRev, startAfter, universe, chunk_size,
                                                                                                         &nResponseCases, &maxRevisionInChunk, std::nullopt, revisions_to_exclude);

            // read the cases in this chunk
            std::vector<std::shared_ptr<Case>> cases_in_chunk;

            while( true )
            {
                std::shared_ptr<Case> data_case = m_pRepo->GetCaseAccess().CreateCase();

                if( !case_iterator->NextCase(*data_case) )
                    break;

                cases_in_chunk.emplace_back(data_case);

                if( sync_binary_data_upload_manager != nullptr )
                {
                    sync_binary_data_upload_manager->AnalyzeCaseBinaryData(*data_case);

                    // process a subset of the chunk when the total binary bytes exceeds a set limit
                    if( sync_binary_data_upload_manager->GetBinaryDataSizeOfChunk() > BluetoothDataChunk::FileChunkSize )
                    {
                        chunk_size = cases_in_chunk.size() - 1;

                        // get the max revision number at this chunksize
                        m_pRepo->GetCasesModifiedSinceRevisionIterator(m_lastSyncRev, startAfter, universe, chunk_size, &nResponseCases,
                                                                       &maxRevisionInChunk, std::nullopt, revisions_to_exclude);
                        SYNCLOG_INFO << "Binary items in the chunk exceeded the set limit of chunk byte content size. Sending a subchunk of cases: " << chunk_size << " cases";
                        break;
                    }
                }
            }

            const size_t casesThisChunk = std::min(chunk_size, nResponseCases);


            SYNCLOG_INFO << "Sending " << casesThisChunk << "/" << nResponseCases << " cases";

            std::string etag = FormatText("ETag: %d", maxRevisionInChunk);
            SYNCLOG_INFO << etag;
            m_responseHeaders.Add(std::move(etag));

            if( !rangeCount.empty() )
                m_responseHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, FormatText("%d/%d", casesThisChunk, nResponseCases));

            SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(m_pRepo->GetSharedCaseAccess(), m_requestHeaders);

            const std::vector<Case*> cases_in_chunk_for_span = SpanHelpers::CreatePointersSpan(cases_in_chunk);

            std::string case_data = sync_case_serializer.GetSyncableCaseData(cases_in_chunk_for_span, sync_binary_data_upload_manager.get());

            SYNCLOG_INFO << "Binary bytes in the chunk: " << ( ( sync_binary_data_upload_manager == nullptr ) ? 0 : sync_binary_data_upload_manager->GetBinaryDataSizeOfChunk() );

            if( !ZLib::Deflate(case_data) )
            {
                SYNCLOG_ERROR << "Compression failed";
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
            constexpr bool UseRemoteCaseOnConflict = false; // this flag's value will not actually be used because we are only putting cases
            m_pRepo->StartSync(remoteDeviceId, std::string(), std::string(), SyncDirection::Put, universe, UseRemoteCaseOnConflict);
            m_pRepo->MarkCasesSentToRemote(cases_in_chunk_for_span, sync_binary_data_upload_manager.get(), SO::Empty_string, maxRevisionInChunk);
            m_pResponseJsonStream.str(case_data);

            const ObexResponseCode response = ( casesThisChunk >= nResponseCases ) ? OBEX_OK : OBEX_PARTIAL_CONTENT;
            //set the sync history to revision complete when all cases are sent
            if( response == OBEX_OK )
                m_pRepo->EndSync();

            SYNCLOG_INFO << "Response: " << ObexResponseCodeToString(response);

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
        {
        }

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
            const std::string ifMatch = m_requestHeaders.GetValue(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER);
            const DeviceId device_id = m_requestHeaders.GetValue(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (!ifMatch.empty()) {
                const int serverRevNum = atoi(ifMatch.c_str());
                if (!m_pRepo->IsPreviousSync(serverRevNum, device_id)) {
                    SYNCLOG_INFO << "Revision " << serverRevNum << " not found. Need to do a full sync";
                    return OBEX_PRECONDITION_FAILED;
                }
                m_lastSyncRev = serverRevNum;
            }

            return OBEX_OK;
        }

        ObexResponseCode performSync()
        {
            DeviceId remoteDeviceId = m_requestHeaders.GetValue(SyncCustomHeaders::DEVICE_ID_HEADER);
            if (remoteDeviceId.empty())
                return OBEX_BAD_REQUEST;

            const std::string universe = m_requestHeaders.GetValue(SyncCustomHeaders::UNIVERSE_HEADER);

            SYNCLOG_INFO << "Put: remote device " << remoteDeviceId
                         << ", universe \"" << universe << "\"";

            const std::string requestString = m_requestJsonStream.str();

            //Start sync and ensure that the repo has readwrite permissions on dictionaries
            constexpr bool UseRemoteCaseOnConflict = true;
            m_pRepo->StartSync(remoteDeviceId, "Bluetooth", std::string(), SyncDirection::Get, universe, UseRemoteCaseOnConflict);

            std::vector<std::shared_ptr<Case>> cases;

            try
            {
                SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(m_pRepo->GetSharedCaseAccess(), m_requestHeaders);

                cases = sync_case_serializer.ParseSyncableCaseData(requestString);
            }

            catch( const std::exception& exception )
            {
                SYNCLOG_ERROR << "Failed to parse request cases data: " << requestString;
                SYNCLOG_ERROR << exception.what();
                return OBEX_BAD_REQUEST;
            }

            // because the cases may change during the sync, this object will ensure
            // that any cases currently loaded are properly updated post-sync
            std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier;

            try
            {
                engine_dictionary_modifier = ObjectTransporter::GetInterpreterAccessor()->CreateEngineDictionaryModifier(
                    m_pRepo->GetCaseAccess().GetDataDict().GetName()
                );

                engine_dictionary_modifier->PrepareForModifications();
            }
            catch(...) { ASSERT(false); }

            // sync the cases
            DataRepositoryTransaction transaction(*m_pRepo);

            //TODO: Make cases observable
            const int thisSyncRev = m_pRepo->SyncCasesFromRemote(cases, std::string());
            m_pRepo->EndSync();

            if( engine_dictionary_modifier != nullptr )
            {
                try
                {
                    engine_dictionary_modifier->FinishedWithModifications();
                }
                catch(...) { ASSERT(false); }
            }

            const ISyncableDataRepository::SyncStats stats = m_pRepo->GetLastSyncStats();

            SYNCLOG_INFO << "Received " << stats.cases_received << " cases. "
                         << stats.cases_not_in_repository << " new cases, "
                         << stats.cases_newer_on_remote << " updated, "
                         << stats.cases_newer_in_repository << " ignored, "
                         << stats.cases_with_conflicts << " conflicts.";

            // Add new rev for this sync
            std::string etag = FormatText("ETag: %d", thisSyncRev);
            SYNCLOG_INFO << etag;
            m_responseHeaders.Add(std::move(etag));

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


SyncObexHandler::SyncObexHandler(DeviceId device_id, std::string root_directory, std::unique_ptr<ISyncObexEngineAccessor> sync_obex_engine_accessor)
    :   m_deviceId(std::move(device_id)),
        m_rootDirectory(PortableFunctions::PathEnsureTrailingSlash(PortableFunctions::PathToNativeSlash(std::move(root_directory)))),
        m_syncObexEngineAccessor(std::move(sync_obex_engine_accessor))
{
    SYNCLOG_INFO << "Creating Bluetooth Server handler";
    SYNCLOG_INFO << "Device id: " << m_deviceId;
    SYNCLOG_INFO << "Root folder: " << m_rootDirectory;
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
        ASSERT(resource.get() == nullptr);
        std::regex  pattern{ R"(\d+\.?\d+)" };
        std::smatch matches;
        float apiVersion = 0.0f;
        //convert the major and minor version to decimal (do not include the patch release)
        const std::string version_string = Versioning::GetVersionString();
        if (std::regex_search(version_string, matches, pattern)) {
            apiVersion = (float)atod(matches[0].str());
        }

        const ConnectResponse connectResponse(m_deviceId, "Bluetooth", std::string(), apiVersion);
        std::string responseJson = Json::ToJson(connectResponse);
        resource = std::make_unique<StringResource>(responseJson);

        SYNCLOG_INFO << "Received connection";

        return OBEX_OK;
    }

    if (type == OBEX_SYNC_DATA_MEDIA_TYPE) {
        return handleSyncGet(UTF8_TODO::GetUtf8(name), requestHeaders, resource);
    } else if (type == OBEX_DIRECTORY_LISTING_MEDIA_TYPE) {
        return handleDirectoryListing(UTF8_TODO::GetUtf8(name), requestHeaders, resource);
    } else if (type == OBEX_SYNC_APP_MEDIA_TYPE) {
        return handleSyncApp(UTF8_TODO::GetUtf8(name), requestHeaders, resource);
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
        return handleSyncPut(UTF8_TODO::GetUtf8(name), requestHeaders, resource);
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


DataRepository* SyncObexHandler::FindDataRepository(const std::string& syncable_dictionary_name, const HeaderList& request_headers) const
{
    if( m_syncObexEngineAccessor == nullptr )
        return nullptr;

    std::string dictionary_name = request_headers.GetValue(SyncCustomHeaders::DICTIONARY_NAME);

    // the dictionary name header does not exist when syncing with versions earlier than CSPro 8.1
    return m_syncObexEngineAccessor->GetDataRepository(syncable_dictionary_name,
                                                       dictionary_name.empty() ? syncable_dictionary_name : dictionary_name);
}


ObexResponseCode SyncObexHandler::handleSyncPut(const std::string& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(resource.get() == nullptr);
    SYNCLOG_INFO << "Sync put " << dictionary_name;

    DataRepository* pRepo = FindDataRepository(dictionary_name, requestHeaders);
    if (pRepo == nullptr) {
        SYNCLOG_ERROR << "Error: dictionary " << dictionary_name << " not found in current application";
        return OBEX_NOT_FOUND;
    }

    ISyncableDataRepository* pSyncableRepo = pRepo->GetSyncableDataRepository();

    if (pSyncableRepo == nullptr) {
        SYNCLOG_ERROR << "Error: dictionary " << dictionary_name << " is not syncable";
        return OBEX_BAD_REQUEST;
    }

    resource = std::make_unique<DictionaryWriteResource>(pSyncableRepo, requestHeaders);
    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleSyncGet(const std::string& dictionary_name, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(resource.get() == nullptr);
    SYNCLOG_INFO << "Sync get " << dictionary_name;

    DataRepository* pRepo = FindDataRepository(dictionary_name, requestHeaders);
    if (pRepo == nullptr) {
        SYNCLOG_ERROR << "Error: dictionary " << dictionary_name << " not found in current application";
        return OBEX_NOT_FOUND;
    }

    ISyncableDataRepository* pSyncableRepo = pRepo->GetSyncableDataRepository();

    if (pSyncableRepo == nullptr) {
        SYNCLOG_ERROR << "Error: dictionary " << dictionary_name << " is not syncable";
        return OBEX_BAD_REQUEST;
    }

    resource = std::make_unique<DictionaryReadResource>(pSyncableRepo, requestHeaders);
    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleDirectoryListing(const std::string& path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    ASSERT(resource.get() == nullptr);
    SYNCLOG_INFO << "Listing directory " << path;

    // this header was not used until CSPro 8.1 so default to true
    const bool request_file_md5s = ( requestHeaders.GetValue(SyncCustomHeaders::GET_FILE_MD5_HEADER) != Json::Text::Bool(false) );

    CString fullDirectoryPath = UTF8_TODO::GetCString(Path::Combine(m_rootDirectory, PortableFunctions::PathToNativeSlash(path)));
    CString canonicalDirectoryPath;
    bool canonOk = PathCanonicalize(canonicalDirectoryPath.GetBuffer(MAX_PATH), fullDirectoryPath) == TRUE;
    canonicalDirectoryPath.ReleaseBuffer();
    if (!canonOk) {
        SYNCLOG_ERROR << "Error: Invalid directory " << UTF8_TODO::GetUtf8(fullDirectoryPath);
        return OBEX_NOT_FOUND;
    }

    if (canonicalDirectoryPath[canonicalDirectoryPath.GetLength() - 1] != PATH_CHAR)
        canonicalDirectoryPath += PATH_CHAR;

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a GET that is outside both root and csentry paths
    if (!isDescendantDirectory(UTF8_TODO::GetCString(m_rootDirectory), canonicalDirectoryPath)
        && (!createCSEntryPath(UTF8_TODO::GetCString(m_rootDirectory), csentryPath) || !isDescendantDirectory(csentryPath, canonicalDirectoryPath))) {
        SYNCLOG_ERROR << "Error: Directory " << UTF8_TODO::GetUtf8(canonicalDirectoryPath) << " is outside of both the project root directory "
                      << m_rootDirectory << " and the csentry root directory " << UTF8_TODO::GetUtf8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    if (!PortableFunctions::FileIsDirectory(canonicalDirectoryPath)) {
        SYNCLOG_ERROR << "Error: " << UTF8_TODO::GetUtf8(canonicalDirectoryPath) << " is a file not a directory";
        return OBEX_NOT_FOUND;
    }

    std::vector<std::wstring> aFileNames = DirectoryLister().SetIncludeDirectories()
                                                            .GetPaths(canonicalDirectoryPath);

    // Store the relative path that when concatenated with the root path will construct the full path
    CString pathFromRoot = UTF8_TODO::GetCString(PortableFunctions::PathToForwardSlash(path));
    if (pathFromRoot.Left(2) != _T("./") && pathFromRoot.Left(3) != _T("../")) {
        pathFromRoot = PortableFunctions::PathAppendForwardSlashToPath<CString>(L"/", pathFromRoot);
    }

    std::vector<FileInfo> directory_listing;

    for( const std::wstring& fullPath : aFileNames )
    {
        std::string filename = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetFilename(UTF8_TODO::GetUtf8(fullPath)));
        const FileInfo::FileType type = PortableFunctions::FileIsDirectory(fullPath) ? FileInfo::FileType::Directory :
                                                                                       FileInfo::FileType::File;

        if( type == FileInfo::FileType::File )
        {
            directory_listing.emplace_back(type, std::move(filename), UTF8_TODO::GetUtf8(pathFromRoot),
                                           PortableFunctions::FileSize(fullPath),
                                           int64_t(0),
                                           request_file_md5s ? PortableFunctions::FileMd5(fullPath) : std::string());
        }

        else
        {
            directory_listing.emplace_back(type, std::move(filename), UTF8_TODO::GetUtf8(pathFromRoot));
        }
    }

    resource = std::make_unique<StringResource>(Json::ToJson(directory_listing));
    SYNCLOG_INFO << "Found " << directory_listing.size() << " files in directory " << UTF8_TODO::GetUtf8(canonicalDirectoryPath);

    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleFileGet(CString path, const HeaderList& requestHeaders, std::unique_ptr<IObexResource>& resource)
{
    if (resource.get() != nullptr) {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    CString fullPath = PortableFunctions::PathAppendToPath(UTF8_TODO::GetCString(m_rootDirectory), PortableFunctions::PathToNativeSlash(path));
    SYNCLOG_INFO << "Sending file " << UTF8_TODO::GetUtf8(fullPath);

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a GET that is outside both root and csentry paths
    if (!isDescendantDirectory(UTF8_TODO::GetCString(m_rootDirectory), fullPath)
        && (!createCSEntryPath(UTF8_TODO::GetCString(m_rootDirectory), csentryPath) || !isDescendantDirectory(csentryPath, fullPath))) {
        SYNCLOG_ERROR << "Error: File " << UTF8_TODO::GetUtf8(fullPath) << " is outside of both the project root directory "
                      << m_rootDirectory << " and the csentry root directory " << UTF8_TODO::GetUtf8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    if (!PortableFunctions::FileExists(fullPath)) {
        SYNCLOG_ERROR << "Error: File " << UTF8_TODO::GetUtf8(fullPath) << " does not exist";
        return OBEX_NOT_FOUND;
    }

    if (PortableFunctions::FileIsDirectory(fullPath)) {
        SYNCLOG_ERROR << "Error: File " << UTF8_TODO::GetUtf8(fullPath) << " is a directory, not a regular file";
        return OBEX_NOT_FOUND;
    }

    CString ifNoneMatch = UTF8_TODO::GetCString(requestHeaders.GetValue("If-None-Match"));
    if (!ifNoneMatch.IsEmpty() && PortableFunctions::FileExists(fullPath)) {
        std::wstring md5 = UTF8_TODO::GetWide(PortableFunctions::FileMd5(fullPath));
        if (SO::EqualsNoCase(md5, ifNoneMatch)) {
            SYNCLOG_INFO << "File not modified. Skipping.";
            return OBEX_NOT_MODIFIED;
        }
    }

    resource = std::make_unique<FileReadResource>(fullPath);
    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleFilePut(CString path, std::unique_ptr<IObexResource>& resource)
{
    if (resource.get() != nullptr) {
        // Not the initial file chunk. Already intialized.
        return OBEX_OK;
    }

    CString fullPath = PortableFunctions::PathAppendToPath(UTF8_TODO::GetCString(m_rootDirectory), PortableFunctions::PathToNativeSlash(path));
    SYNCLOG_INFO << "Receiving file " << UTF8_TODO::GetUtf8(fullPath);

#ifndef WIN_DESKTOP
    CString csentryPath;
    // Don't allow a PUT that is outside both root and csentry paths
    if (!isDescendantDirectory(UTF8_TODO::GetCString(m_rootDirectory), fullPath)
        && (!createCSEntryPath(UTF8_TODO::GetCString(m_rootDirectory), csentryPath) || !isDescendantDirectory(csentryPath, fullPath))) {
        SYNCLOG_ERROR << "Error: File " << UTF8_TODO::GetUtf8(fullPath) << " is outside of both the project root directory "
                      << m_rootDirectory << " and the csentry root directory " << UTF8_TODO::GetUtf8(csentryPath);
        return OBEX_FORBIDDEN;
    }
#endif

    // Create path if it doesn't exist
    CString dir = PortableFunctions::PathGetDirectory<CString>(fullPath);
    if (!PortableFunctions::FileExists(dir)) {
        if (!PortableFunctions::PathMakeDirectories(dir)) {
            SYNCLOG_ERROR << "Error: Failed to create directory " << UTF8_TODO::GetUtf8(dir);
            return OBEX_FORBIDDEN;
        }
    }

    resource = std::make_unique<FileWriteResource>(fullPath);
    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleSyncApp(const std::string& package_name, const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
    const std::unique_ptr<const ApplicationPackageManager> application_package_manager = ( m_syncObexEngineAccessor != nullptr ) ? m_syncObexEngineAccessor->CreateApplicationPackageManager() : nullptr;

    if( application_package_manager == nullptr ) {
        SYNCLOG_ERROR << "Error: Application Package Manager cannot be created because no root directory exists";
        return OBEX_NOT_FOUND;
    }

    const std::unique_ptr<const ApplicationPackageManager::ApplicationWithSignature> signed_package = application_package_manager->GetInstalledApplicationPackageWithSignature(package_name);
    if (signed_package == nullptr) {
        SYNCLOG_ERROR << "Error: Application Package " << package_name << " is not installed";
        return OBEX_NOT_FOUND;
    }

    const std::string request_signature = request_headers.GetValue("If-None-Match");
    if (!request_signature.empty() && SO::EqualsNoCase(request_signature, signed_package->signature)) {
        SYNCLOG_INFO << "Application signature matches installed version. Skipping.";
        return OBEX_NOT_MODIFIED;
    }

    const std::string request_build_time_header = request_headers.GetValue(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER);
    if (!request_build_time_header.empty()) {
        const int64_t request_build_time = PortableFunctions::ParseRFC3339DateTime(request_build_time_header);
        if (request_build_time > 0 && request_build_time >= signed_package->package.GetBuildTime()) {
            SYNCLOG_INFO << "Application build time less than currently installed. Skipping.";
            return OBEX_NOT_MODIFIED;
        }
    }

    const std::string request_files_header = request_headers.GetValue(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER);

    std::string request_files_json = Base64::DecodeToString(request_files_header);

    if( !ZLib::Inflate(request_files_json) )
        throw SyncError(100139);

    const std::vector<ApplicationPackage::File> request_files = JsonConverter::CreateFileSpecListFromJson(Json::Parse(request_files_json));
    std::vector<std::string> files_to_include;

    for( const ApplicationPackage::File& package_file : signed_package->package.GetFiles() )
    {
        const auto& match = std::find_if(request_files.cbegin(), request_files.cend(),
                                         [&package_file](const auto& f) { return SO::EqualsNoCase(package_file.path, f.path); });

        if( match == request_files.cend() || ( !package_file.only_on_first_install && !SO::EqualsNoCase(match->signature, package_file.signature) ) )
        {
            std::string& path_in_zip = PortableFunctions::MakePathToForwardSlash(files_to_include.emplace_back(package_file.path));

            if( SO::StartsWith(path_in_zip, "./") )
                path_in_zip = path_in_zip.substr(2);
        }
    }

    const std::string application_package_zip_path = application_package_manager->GetPackageZipFilePath(package_name);

    if( files_to_include.size() == signed_package->package.GetFiles().size() )
    {
        resource = std::make_unique<FileReadResource>(UTF8_TODO::GetCString(application_package_zip_path));
    }

    else
    {
        // create a temporary ZIP file with only the files to send
        TemporaryFile temporary_zip_file;
        ZipCreator zip_creator(temporary_zip_file.GetPath());

        ZipReader zip_reader(application_package_zip_path);

        auto add_package_json = [&](std::string filename)
        {
            if( zip_reader.FindPathInZip(filename).has_value() )
            {
                files_to_include.emplace_back(filename);
                return true;
            }

            return false;
        };

        if( !add_package_json("package.json") )
        {
            if( !add_package_json("package.csds") )
                ASSERT(false);
        }

        zip_creator.AddFiles(zip_reader, files_to_include);

        zip_creator.Close();

        resource = std::make_unique<TemporaryFileReadResource>(std::move(temporary_zip_file));
    }

    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleSyncMessage(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
    const std::string sync_message_json = request_headers.GetValue(SyncCustomHeaders::MESSAGE_HEADER);

    if( sync_message_json.empty() )
        return OBEX_BAD_REQUEST;

    std::optional<SyncMessage> sync_message;

    try
    {
        const JsonNode json_node = Json::Parse(sync_message_json);
        sync_message.emplace(SyncMessage::CreateFromJsonFromBluetooth(json_node));
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_ERROR << "Failed to parse request JSON " << sync_message_json;
        SYNCLOG_ERROR << exception.what();
        return OBEX_BAD_REQUEST;
    }

    std::optional<SharableString> optional_response = ( m_syncObexEngineAccessor != nullptr ) ? m_syncObexEngineAccessor->OnSyncMessage(*sync_message) :
                                                                                                std::nullopt;

    if( !optional_response.has_value() )
        return OBEX_METHOD_NOT_ALLOWED;

    const SyncMessage sync_message_response(sync_message->GetName(), std::move(*optional_response));

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
    sync_message_response.WriteJsonForBluetooth(*json_writer);

    resource = std::make_unique<StringResource>(json_writer->GetString());

    return OBEX_OK;
}


ObexResponseCode SyncObexHandler::handleSyncParadataStart(const HeaderList& request_headers, std::unique_ptr<IObexResource>& resource)
{
    std::string client_log_uuid = request_headers.GetValue(SyncCustomHeaders::PARADATA_LOG_UUID);

    if( client_log_uuid.empty() )
        return OBEX_BAD_REQUEST;

    HeaderList response_headers;

    if( Paradata::Logger::IsOpen() )
    {
        m_paradataSyncer = Paradata::Logger::GetSyncer();

        m_paradataSyncer->SetPeerLogUuid(std::move(client_log_uuid));

        response_headers.Add(SyncCustomHeaders::PARADATA_LOG_UUID, m_paradataSyncer->GetLogUuid());
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

    SYNCLOG_INFO << "Receiving paradata events from the log " << m_paradataSyncer->GetPeerLogUuid();

    resource = std::make_unique<FileWriteResource>(UTF8_TODO::GetCString(m_paradataSyncer->GetFilePathForReceivedSyncableDatabase()));

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

    const std::optional<std::string> extracted_syncable_database_file_path = m_paradataSyncer->GetExtractedSyncableDatabaseFilePath();

    if( extracted_syncable_database_file_path.has_value() )
    {
        SYNCLOG_INFO
            << "Sending paradata events to be added to the log "
            << m_paradataSyncer->GetPeerLogUuid();

        resource = std::make_unique<FileReadResource>(UTF8_TODO::GetCString(*extracted_syncable_database_file_path));

        return OBEX_OK;
    }

    else
    {
        SYNCLOG_INFO
            << "Skipping sending paradata events to the log "
            << m_paradataSyncer->GetPeerLogUuid()
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
