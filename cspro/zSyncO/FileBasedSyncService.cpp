#include "stdafx.h"
#include "FileBasedSyncService.h"
#include "CaseObservable.h"
#include "FileBasedParadataSyncer.h"
#include "JsonConverter.h"
#include "NetworkDataChunk.h"
#include "SyncDictionaryInfo.h"
#include "SyncMessage.h"
#include <zToolsO/MemoryStream.h>
#include <zUtilO/BinaryDataAccessor.h>
#include <zZip/ZipFile.h>
#include <zNetwork/FileBasedConnection.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zDataO/SyncBinaryDataUploadManager.h>


namespace
{
    constexpr size_t MaxFilesToProcessInGetCases = 100;

    constexpr std::string_view BinaryDataJsonFilename_sv = "binary-data.json";
}


FileBasedSyncService::FileBasedSyncService(std::string sync_service_description, std::shared_ptr<FileBasedConnection> file_based_connection,
                                           const bool is_connection_network_based)
    :   m_fileBasedConnection(std::move(file_based_connection)),
        m_syncServiceDescription(std::move(sync_service_description)),
        m_dataChunk(is_connection_network_based ? std::make_shared<NetworkDataChunk>() :
                                                  std::make_shared<NetworkDataChunkWithoutOptimizations>())
{
    ASSERT(m_fileBasedConnection != nullptr);
}


FileBasedSyncService::~FileBasedSyncService()
{
}


std::string FileBasedSyncService::FileReadText(const std::string& remote_file_path)
{
    std::ostringstream output_stream;
    m_fileBasedConnection->Download(remote_file_path, output_stream);
    return output_stream.str();
}


template<typename T>
void FileBasedSyncService::FileWriteText(const std::string& remote_file_path, T&& text)
{
    const size_t text_length = text.length();
    std::istringstream input_stream(std::forward<T>(text));
    m_fileBasedConnection->Upload(input_stream, text_length, remote_file_path);
}


SyncGetResponse FileBasedSyncService::GetCases(std::shared_ptr<const CaseAccess> case_access,
                                               const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                               const std::string& /*last_case_uuid*/, const std::vector<std::string>& excluded_revisions)
{
    ASSERT(case_access != nullptr);
    const std::string& dictionary_name = case_access->GetDataDict().GetSyncableName();

    SYNCLOG_INFO << "Start " << m_syncServiceDescription << " case download for dictionary: " << dictionary_name;
    const RAII::RunOnDestruction run_on_destruction([&]() { SYNCLOG_INFO << m_syncServiceDescription << " case download complete."; });

    // upload the dictionary if we have not already done so in this session
    UploadDictionaryIfNeeded(case_access->GetDataDict());

    int64_t last_sync_time = 0;
    std::vector<std::string> last_sync_most_recent_filenames;

    if( !last_server_revision.empty() )
    {
        const size_t dollar_pos = last_server_revision.find('$');

        // Invalid revision
        if( dollar_pos == std::string::npos )
            return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);

        char* endptr;
        last_sync_time = strtoll(last_server_revision.c_str(), &endptr, 10);
        ASSERT(dollar_pos == static_cast<size_t>(endptr - last_server_revision.c_str()));
        last_sync_most_recent_filenames = SO::SplitString(std::string_view(last_server_revision).substr(dollar_pos + 1), '|');
    }

    // First check if the data directory exists
    const std::string remote_data_directory = GetDataDirectory(dictionary_name);

    if( !m_fileBasedConnection->FileIsDirectory(remote_data_directory) )
    {
        // No data directory yet so nothing to download
        SYNCLOG_INFO << m_syncServiceDescription << " has no data directory.";
        return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, { }, std::string());
    }

    // List data directory
    const std::vector<FileInfo> directory_listing = m_fileBasedConnection->GetDirectoryListing(remote_data_directory, false);
    SYNCLOG_INFO << "Data directory files: " << directory_listing.size();

    // Check to see if last revision matches directory listing
    if( !last_server_revision.empty() )
    {
        for( const std::string& filename : last_sync_most_recent_filenames )
        {
            const auto& lookup = std::find_if(directory_listing.cbegin(), directory_listing.cend(),
                                              [&](const FileInfo& file_info) { return ( file_info.GetType() == FileInfo::FileType::File &&
                                                                                        file_info.GetName() == filename ); });

            if( lookup == directory_listing.cend() )
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
        }
    }

    // Ignore files that don't match the pattern deviceid$revision
    const std::regex data_file_regex(R"(^[0-9a-fA-F\-]+\$[0-9a-fA-F\-]+$)");

    // Download files with more recent modified times
    std::vector<const FileInfo*> files_to_download;

    for( const FileInfo& file_info : directory_listing )
    {
        if( file_info.GetType() == FileInfo::FileType::File )
        {
            // Skip files that don't match pattern such as temp files
            if( !std::regex_match(file_info.GetName(), data_file_regex) )
                continue;

            const int64_t file_last_modified_time = file_info.GetLastModified();

            SYNCLOG_INFO << "File: " << file_info.GetName()
                         << " Mod=" << file_last_modified_time
                         << ", " << DateTime::TimeToRFC3339(file_last_modified_time);

            // Make sure this is a case file
            const auto [file_device_id, file_server_revision] = ParseDataFilename(file_info.GetName());

            if( file_device_id.empty() )
                continue;

            if( file_last_modified_time >= last_sync_time )
            {
                // Only download cases that are not in the list of excluded revisions
                // (ones that we uploaded)
                if( file_device_id == device_id &&
                    std::find(excluded_revisions.cbegin(), excluded_revisions.cend(), file_server_revision) != excluded_revisions.cend() )
                {
                    continue;
                }

                // Don't download the file(s) we used to mark the revision in the last sync (the newest file(s) at that time)
                // Since FTP file times are not precise, we could have had other files uploaded with the timestamp
                // from last revision after we synced so we have to download files with same timestamp as last revision
                // (file_last_modified_time >= last_sync_time) instead of (file_last_modified_time > last_sync_time).
                // This means that we will always download the file we used as the
                // revision on the next sync. To avoid this we also put the filename(s) that we found at that timestamp
                // in the revision so we can skip them.
                if( file_last_modified_time == last_sync_time &&
                    std::find(last_sync_most_recent_filenames.cbegin(), last_sync_most_recent_filenames.cend(), file_info.GetName()) != last_sync_most_recent_filenames.cend() )
                {
                    continue;
                }

                files_to_download.emplace_back(&file_info);
            }
        }
    }

    std::vector<std::shared_ptr<Case>> server_cases;

    // return, with a blank server revision, if there are no cases to download
    if( files_to_download.empty() )
    {
        return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete,
                               std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(std::move(server_cases))),
                               std::string());
    }

    SyncGetResponse::SyncGetResult get_result = SyncGetResponse::SyncGetResult::Complete;

    // sort the files by modified time
    std::sort(files_to_download.begin(), files_to_download.end(),
              [&](const FileInfo* const fi1, const FileInfo* const fi2) { return ( fi1->GetLastModified() < fi2->GetLastModified() ); });

    // if there are are many files to download, download them in chunks
    if( files_to_download.size() > MaxFilesToProcessInGetCases )
    {
        size_t actual_files_to_download = MaxFilesToProcessInGetCases;

        // makes sure all files with the same file time are downloaded
        while( actual_files_to_download < files_to_download.size() &&
                files_to_download[actual_files_to_download - 1]->GetLastModified() == files_to_download[actual_files_to_download]->GetLastModified() )
        {
            ++actual_files_to_download;
        }

        if( actual_files_to_download != files_to_download.size() )
        {
            files_to_download.erase(files_to_download.begin() + actual_files_to_download,
                                    files_to_download.end());
            get_result = SyncGetResponse::SyncGetResult::MoreData;
        }
    }

    const int64_t newest_file_time = files_to_download.back()->GetLastModified();
    std::string newest_filenames = files_to_download.back()->GetName();

    // store ties (files with the same file time) so that we don't download them again
    for( size_t i = files_to_download.size() - 2; i < files_to_download.size() && files_to_download[i]->GetLastModified() == newest_file_time; --i )
    {
        newest_filenames.push_back('|');
        newest_filenames.append(files_to_download[i]->GetName());

        // Don't let it get too big, if we truncate we may download some files we don't need to but won't do any harm
        if( newest_filenames.length() >= 4096 )
            break;
    }

    // the server revision is the newest file time and name(s), next sync we will get anything newer
    std::string server_revision = SO::Concatenate(IntToString(static_cast<int64_t>(newest_file_time)), "$", newest_filenames);

    // Get cases from each of the files
    int64_t total_size = 0;

    for( const FileInfo* const file_info : files_to_download )
        total_size += file_info->GetSize();

    if( m_syncListener != nullptr )
        m_syncListener->SetProgressTotal(total_size);

    SyncCaseSerializer sync_case_serializer(case_access, SyncCaseSerializer::Version::V3);

    for( const FileInfo* const file_info : files_to_download )
    {
        SYNCLOG_INFO << "Download file: " << file_info->GetName();

        std::string case_data = FileReadText(remote_data_directory + file_info->GetName());

        if( m_syncListener != nullptr )
            m_syncListener->AddToProgressPreviousStepsTotal(file_info->GetSize());

        try
        {
            std::vector<std::shared_ptr<Case>> cases_in_chunk = ZipReader::IsZipHeader(case_data) ? GetCasesFromZipFile(dictionary_name, universe, sync_case_serializer, case_data, file_info->GetName()) :
                                                                                                    GetCasesFromV2CaseData(case_access, universe, case_data);

            server_cases.insert(server_cases.end(), std::make_move_iterator(cases_in_chunk.begin()),
                                                    std::make_move_iterator(cases_in_chunk.end()));
        }

        catch( const std::exception& exception )
        {
            SYNCLOG_ERROR << "Invalid server response: " << exception.what();
            SYNCLOG_ERROR << case_data;
            throw SyncError(100121);
        }
    }

    return SyncGetResponse(get_result,
                           std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(std::move(server_cases))),
                           std::move(server_revision));
}


std::vector<std::shared_ptr<Case>> FileBasedSyncService::GetCasesFromZipFile(const std::string& dictionary_name, const std::string& universe, SyncCaseSerializer& sync_case_serializer,
                                                                             const std::string& case_data, const std::string& zip_filename)
{
    ZipReader zip_reader(case_data.data(), case_data.length(), zip_filename.c_str());

    std::vector<std::shared_ptr<Case>> cases;
    std::optional<JsonNode> binary_data_json_node;

    // first parse each of the cases in the ZIP file
    zip_reader.ForeachFileContents(
        [&](const char* const path_in_zip, BinaryBlock binary_block)
        {
            JsonNode json_node = Json::Parse(binary_block.as<std::string_view>());

            if( path_in_zip == BinaryDataJsonFilename_sv )
            {
                ASSERT(!binary_data_json_node.has_value());
                binary_data_json_node = std::move(json_node);
            }

            else
            {
                cases.emplace_back(sync_case_serializer.ParseCaseJsonFromSyncableCaseData(json_node));
            }

            return true;
        });

    // filter by universe
    FilterCasesByUniverse(cases, universe);

    // then download any binary data that we do not currently have (that is part of this universe)
    if( binary_data_json_node.has_value() )
    {
        const std::string remote_binary_data_directory = GetDataBinaryDataDirectory(dictionary_name);

        const JsonNodeArray array_node = binary_data_json_node->GetArray();
        JsonNodeArrayIterator array_node_itr = array_node.begin();
        const JsonNodeArrayIterator array_node_end = array_node.end();

        sync_case_serializer.ParseSyncableCaseDataBinaryData(cases,
            [&]() -> std::optional<JsonNode>
            {
                while( array_node_itr != array_node_end )
                {
                    JsonNode metadata_json_node = (array_node_itr++)->Get<JsonNode>();

                    if( universe.empty() || SO::StartsWith(metadata_json_node.Get<std::string_view>(JK::key), universe) )
                        return metadata_json_node;
                }

                return std::nullopt;
            },
            [&](const std::string& signature)
            {
                // the file path is the signature (the MD5)
                const std::string remote_binary_data_file_path = remote_binary_data_directory + signature;

                const std::string binary_contents = FileReadText(remote_binary_data_file_path); // BINARY_BLOCK_TODO convert string to BinaryBlock once BinaryData supports it

                return SO::CreateByteVector(binary_contents);
            });
    }

    return cases;
}


std::vector<std::shared_ptr<Case>> FileBasedSyncService::GetCasesFromV2CaseData(std::shared_ptr<const CaseAccess> case_access, const std::string& universe, std::string& case_data)
{
    if( ZLib::IsDeflated(case_data) && !ZLib::Inflate(case_data) )
        throw SyncError(100139);

    // parse the cases using the V2 JSON
    SyncCaseSerializer sync_case_serializer_v2(std::move(case_access), SyncCaseSerializer::Version::V2);
    std::vector<std::shared_ptr<Case>> cases = sync_case_serializer_v2.ParseSyncableCaseData(case_data);

    // filter by universe
    FilterCasesByUniverse(cases, universe);

    return cases;
}


void FileBasedSyncService::FilterCasesByUniverse(std::vector<std::shared_ptr<Case>>& cases, const std::string& universe)
{
    if( universe.empty() )
        return;

    for( auto case_itr = cases.begin(); case_itr != cases.end(); )
    {
        if( SO::StartsWith((*case_itr)->GetKey(), universe) )
        {
            ++case_itr;
        }

        else
        {
            case_itr = cases.erase(case_itr);
        }
    }
}


SyncPutResponse FileBasedSyncService::PutCases(std::shared_ptr<const CaseAccess> case_access,
                                               const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* const sync_binary_data_upload_manager,
                                               const DeviceId& device_id, const std::string& /*universe*/, const std::string& last_server_revision)
{
    ASSERT(case_access != nullptr);
    const std::string& dictionary_name = case_access->GetDataDict().GetSyncableName();

    SYNCLOG_INFO << "Start " << m_syncServiceDescription << " case upload for dictionary: " << dictionary_name;
    const RAII::RunOnDestruction run_on_destruction([&]() { SYNCLOG_INFO << m_syncServiceDescription << " case upload complete."; });

    // upload the dictionary if we have not already done so in this session
    UploadDictionaryIfNeeded(case_access->GetDataDict());

    const std::string remote_data_directory = GetDataDirectory(dictionary_name);

    if( !last_server_revision.empty() )
    {
        // Check to see if our last sync is on server
        const std::string last_sync_file_path = GetDataFilePath(remote_data_directory, device_id, last_server_revision);

        if( !m_fileBasedConnection->FileIsRegular(last_sync_file_path) )
            return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
    }

    if( cases.empty() )
    {
        SYNCLOG_INFO << "No cases to upload.";
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, last_server_revision);
    }

    // server revision is just a random UUID that we add to the filename;
    // when we put next time we can check for the presence of the server revision file to see if the server has our last sync
    std::string server_revision = CreateUuid();
    const std::string sync_file_path = GetDataFilePath(remote_data_directory, device_id, server_revision);

    // create a ZIP file with each case's JSON (V3) as a file named after the case UUID
    TemporaryFile zip_temporary_file;
    ZipCreator zip_creator(zip_temporary_file.GetPath());

    SyncCaseSerializer sync_case_serializer(std::move(case_access), SyncCaseSerializer::Version::V3);
    size_t case_json_bytes_in_last_chunk = 0;

    for( const Case* const data_case : cases )
    {
        const std::string case_file_path = PortableFunctions::PathAppendFileExtension(data_case->GetUuid(), FileExtensions::Json);

        const std::string case_json = sync_case_serializer.GetSyncableJson(*data_case, false);
        case_json_bytes_in_last_chunk += case_json.length();

        zip_creator.AddContent(case_file_path, case_json);
    }

    m_dataChunk->ResetForNextChunk();

    // first upload all binary data
    if( sync_binary_data_upload_manager != nullptr && sync_binary_data_upload_manager->IsBinaryDataPartOfChunk() )
    {
        const std::string uploaded_binary_data_metadata = UploadCaseBinaryData(*sync_binary_data_upload_manager, dictionary_name);

        zip_creator.AddContent(std::string(BinaryDataJsonFilename_sv), uploaded_binary_data_metadata);
    }

    // then upload the ZIP file with the case data
    zip_creator.Close();

    m_fileBasedConnection->Upload(zip_temporary_file.GetPath(), sync_file_path);

    m_dataChunk->Optimize(cases.size(), case_json_bytes_in_last_chunk);

    return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, std::move(server_revision));
}


std::string FileBasedSyncService::UploadCaseBinaryData(const SyncBinaryDataUploadManager& sync_binary_data_upload_manager, const std::string& dictionary_name)
{
    ASSERT(sync_binary_data_upload_manager.IsBinaryDataPartOfChunk());

    const std::string remote_binary_data_directory = GetDataBinaryDataDirectory(dictionary_name);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
    json_writer->BeginArray();

    sync_binary_data_upload_manager.ForeachBinaryCaseItemInChunk(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
            const std::vector<std::byte>& content = binary_data_accessor.GetBinaryData().GetContent();

            // the file path is the signature (the MD5)
            const std::string remote_binary_data_file_path = remote_binary_data_directory + binary_data_accessor.GetSignature();

            MemoryStream content_memory_stream(content.data(), content.size());
            m_fileBasedConnection->Upload(content_memory_stream, content.size(), remote_binary_data_file_path);

            json_writer->BeginObject()
                        .Write(JK::uuid, index.GetCase().GetUuid())
                        .Write(JK::key, index.GetCase().GetKey())
                        .Write(JK::signature, binary_data_accessor.GetSignature())
                        .Write(JK::size, content.size())
                        .EndObject();
        });

    json_writer->EndArray();

    return json_writer->ReleaseString();
}


IDataChunk& FileBasedSyncService::GetChunk()
{
    return *m_dataChunk;
}


std::vector<SyncDictionaryInfo> FileBasedSyncService::GetDictionaries()
{
    std::vector<SyncDictionaryInfo> dictionaries;

    const std::string remote_dictionary_root_path = "/CSPro/DataSync/";

    if( m_fileBasedConnection->FileIsDirectory(remote_dictionary_root_path) )
    {
        for( const FileInfo& file_info : GetDirectoryListing(remote_dictionary_root_path, false) )
        {
            if( file_info.GetType() == FileInfo::FileType::Directory )
            {
                const std::string remote_dictionary_info_file_path = remote_dictionary_root_path + file_info.GetName() + "/dict/info.json";

                try
                {
                    const std::string info_json = FileReadText(remote_dictionary_info_file_path);
                    dictionaries.emplace_back(Json::FromJson<SyncDictionaryInfo>(info_json));
                }

                catch(...)
                {
                    // Just ignore the dictionary if files are missing or unreadable
                    ASSERT(false);
                }
            }
        }
    }

    return dictionaries;
}


std::string FileBasedSyncService::GetDictionary(const std::string& dictionary_name)
{
    const std::string remote_dictionary_file_path = GetDictionaryFilePath(dictionary_name);

    return FileReadText(remote_dictionary_file_path);
}


void FileBasedSyncService::UploadDictionaryWorker(const CDataDict& dictionary, const bool upload_even_if_exists)
{
    // return if we know that we have uploaded the dictionary in this session
    if( m_dictionariesUploaded.find(dictionary.GetSyncableName()) != m_dictionariesUploaded.cend() )
        return;

    const std::string remote_dictionary_file_path = GetDictionaryFilePath(dictionary.GetSyncableName());

    // upload the dictionary
    if( upload_even_if_exists || !m_fileBasedConnection->FileIsRegular(remote_dictionary_file_path) )
    {
        SYNCLOG_INFO << "Uploading dictionary";
        FileWriteText(remote_dictionary_file_path,
                      dictionary.GetJson(true));
    }

    // upload the dictionary info
    const std::string remote_dictionary_info_file_path = PortableFunctions::PathReplaceFilename(remote_dictionary_file_path, "info.json");

    if( upload_even_if_exists || !m_fileBasedConnection->FileIsRegular(remote_dictionary_info_file_path) )
    {
        SYNCLOG_INFO << "Uploading dictionary info";
        FileWriteText(remote_dictionary_info_file_path, Json::ToJson(SyncDictionaryInfo(dictionary)));
    }

    // upload a PFF to download the data
    const std::string remote_download_pff_file_path = "/CSPro/DataSync/DownloadData-" + PortableFunctions::PathAppendFileExtension(dictionary.GetSyncableName(), FileExtensions::Pff);

    if( upload_even_if_exists || !m_fileBasedConnection->FileIsRegular(remote_download_pff_file_path) )
    {
        TemporaryFile temporary_file;

        PFF pff;
        pff.SetPifFileName(UTF8_TODO::GetCString(temporary_file.GetPath()));
        pff.SetAppType(APPTYPE::Sync);
        pff.SetSyncDirection(SyncDirection::Get);
        pff.SetSilent(false);
        SetDownloadPffSyncServiceParams(pff);

        const std::string data_file_path = PortableFunctions::CreateFilePath(PortableFunctions::PathGetDirectory(temporary_file.GetPath()),
                                                                             SO::ToLower(dictionary.GetSyncableName()),
                                                                             FileExtensions::Data::CSProDB);
        pff.SetExternalDataConnectionString(UTF8_TODO::GetCString(dictionary.GetSyncableName()), UTF8_TODO::GetCString(data_file_path));

        pff.Save();

        SYNCLOG_INFO << "Uploading download PFF";
        PutFile(temporary_file.GetPath(), remote_download_pff_file_path);
    }

    m_dictionariesUploaded.insert(dictionary.GetSyncableName());
}


void FileBasedSyncService::UploadDictionaryIfNeeded(const CDataDict& dictionary)
{
    UploadDictionaryWorker(dictionary, false);
}


void FileBasedSyncService::PutDictionary(const CDataDict& dictionary)
{
    UploadDictionaryWorker(dictionary, true);
}


void FileBasedSyncService::DeleteDictionary(const std::string& dictionary_name)
{
    const std::string remote_dictionary_directory = GetDictionaryDirectory(dictionary_name);
    m_fileBasedConnection->DirectoryDelete(remote_dictionary_directory);
}


std::string FileBasedSyncService::GetDictionaryDirectory(const std::string& dictionary_name)
{
    return SO::Concatenate("/CSPro/DataSync/",
                           dictionary_name,
                           "/");
}


std::string FileBasedSyncService::GetDictionaryFilePath(const std::string& dictionary_name)
{
    return SO::Concatenate(GetDictionaryDirectory(dictionary_name),
                           "dict/",
                           PortableFunctions::PathAppendFileExtension(dictionary_name, FileExtensions::Dictionary));
}


std::string FileBasedSyncService::GetDataDirectory(const std::string& dictionary_name)
{
    return GetDictionaryDirectory(dictionary_name) + "data/";
}


std::string FileBasedSyncService::GetDataBinaryDataDirectory(const std::string& dictionary_name)
{
    return GetDictionaryDirectory(dictionary_name) + "data/binary-data/";
}


std::string FileBasedSyncService::GetDataFilePath(const std::string& data_path, const DeviceId& device_id, const std::string& server_revision)
{
    return SO::Concatenate(data_path, device_id, "$", server_revision);
}


const char* FileBasedSyncService::GetAppsDirectory()
{
    return "/CSPro/apps/";
}


std::tuple<DeviceId, std::string> FileBasedSyncService::ParseDataFilename(const std::string& filename)
{
    constexpr size_t UuidLength = 36;
    const auto [device_id_sv, uuid_sv] = SO::GetTextOnEitherSideOfCharacter(filename, '$');

    // uuid is length 36 so this is not a valid file
    if( uuid_sv.length() != UuidLength )
        return std::make_tuple(DeviceId(), std::string());

    return std::make_tuple(DeviceId(device_id_sv), std::string(uuid_sv));
}


bool FileBasedSyncService::GetFile(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5)
{
    return m_fileBasedConnection->Download(remote_file_path, local_file_path, existing_file_md5);
}


std::unique_ptr<TemporaryFile> FileBasedSyncService::GetFileIfExists(const std::string& remote_file_path)
{
    if( m_fileBasedConnection->FileIsRegular(remote_file_path) )
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        if( GetFile(remote_file_path, temporary_file->GetPath(), std::string()) )
            return temporary_file;
    }

    return nullptr;
}


void FileBasedSyncService::PutFile(const std::string& local_file_path, const std::string& remote_path)
{
    m_fileBasedConnection->Upload(local_file_path, remote_path);
}


std::vector<FileInfo> FileBasedSyncService::GetDirectoryListing(const std::string& remote_path, const bool request_file_md5s)
{
    return m_fileBasedConnection->GetDirectoryListing(remote_path, request_file_md5s);
}


std::vector<ApplicationPackage> FileBasedSyncService::ListApplicationPackages()
{
    SYNCLOG_INFO << m_syncServiceDescription << " list application packages.";
    const RAII::RunOnDestruction run_on_destruction([&]() { SYNCLOG_INFO << m_syncServiceDescription << " list application packages complete."; });

    std::vector<ApplicationPackage> packages;
    const std::string apps_directory = GetAppsDirectory();

    if( !m_fileBasedConnection->FileIsDirectory(apps_directory) )
    {
        // No package directory yet so nothing to download
        SYNCLOG_INFO << "Sync service has no apps directory.";
        return packages;
    }

    // List packages directory
    const std::vector<FileInfo> directory_listing = GetDirectoryListing(apps_directory, false);
    SYNCLOG_INFO << "apps directory files: " << directory_listing.size();

    for( const FileInfo& file_info : directory_listing )
    {
        if( file_info.GetType() == FileInfo::FileType::File )
        {
            const std::string& package_filename = file_info.GetName();

            if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(package_filename), FileExtensions::Zip) )
                continue;

            SYNCLOG_INFO << "Found app: " + package_filename;

            // Check for JSON file containing metadata
            std::string package_spec_filename = PortableFunctions::PathReplaceFileExtension(package_filename, FileExtensions::Json);

            auto find_file = [&]()
            {
                return ( std::find_if(directory_listing.cbegin(), directory_listing.cend(),
                                      [&](const FileInfo& fi) { return ( fi.GetName() == package_spec_filename ); }) != directory_listing.cend() );
            };

            if( !find_file() )
            {
                //  old versions used .csds instead of .json
                package_spec_filename = PortableFunctions::PathReplaceFileExtension(package_filename, FileExtensions::DeploySpec);

                if( !find_file() )
                {
                    SYNCLOG_WARNING << "Found package but missing info file: " << package_spec_filename;
                    continue;
                }
            }

            try
            {
                const std::string package_info_json = FileReadText(Path::Combine(apps_directory, package_spec_filename));
                packages.emplace_back(JsonConverter::CreateApplicationPackageFromJson(Json::Parse(package_info_json)));
            }

            catch( const SyncError& exception )
            {
                // Just ignore the package if files are missing or unreadable
                SYNCLOG_WARNING << "Error reading package info file: " << exception.what() << " : " << package_spec_filename;
            }
        }
    }

    return packages;
}


bool FileBasedSyncService::DownloadApplicationPackage(const std::string& package_name, const std::string& local_file_path,
                                                               const ApplicationPackage* const current_package, const std::string* const current_package_signature)
{
    SYNCLOG_INFO << m_syncServiceDescription << " download application package " << package_name << " to " << local_file_path;

    // Check to see if we already have the current version
    if( current_package != nullptr )
    {
        const std::vector<ApplicationPackage> sync_service_packages = ListApplicationPackages();
        const auto& lookup = std::find_if(sync_service_packages.cbegin(), sync_service_packages.cend(),
                                          [&](const ApplicationPackage& p) { return ( p.GetName() == package_name ); });

        if( lookup != sync_service_packages.cend() )
        {
            // Local package is same or newer than sync service package
            if( current_package->GetBuildTime() >= lookup->GetBuildTime() )
            {
                SYNCLOG_INFO << m_syncServiceDescription << " application package has an earlier build time than the current package. Skipping download.";
                return false;
            }
        }
    }

    const std::string package_file_path = GetFilePathFromPackageName(package_name, nullptr);
    GetFile(package_file_path, local_file_path, ( current_package_signature != nullptr ) ? *current_package_signature : std::string());

    SYNCLOG_INFO << m_syncServiceDescription << " download application package complete.";

    return true;
}


void FileBasedSyncService::UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name, const std::string& package_spec_json)
{
    SYNCLOG_INFO << m_syncServiceDescription << " upload application package " << package_name << " from " << local_package_zip_file_path;

    std::string package_spec_file_path;
    const std::string package_file_path = GetFilePathFromPackageName(package_name, &package_spec_file_path);

    PutFile(local_package_zip_file_path, package_file_path);
    FileWriteText(package_spec_file_path, package_spec_json);

    SYNCLOG_INFO << m_syncServiceDescription << " upload application package complete.";
}


void FileBasedSyncService::DeleteApplication(const std::string& package_name)
{
    std::string package_spec_file_path;
    const std::string package_file_path = GetFilePathFromPackageName(package_name, &package_spec_file_path);

    m_fileBasedConnection->FileDelete(package_spec_file_path);
    m_fileBasedConnection->FileDelete(package_file_path);
}


std::string FileBasedSyncService::GetFilePathFromPackageName(const std::string& package_name, std::string* const package_spec_file_path)
{
    std::string package_file_path = PortableFunctions::CreateFilePath(GetAppsDirectory(),
                                                                      ReplaceInvalidFileChars(package_name, '_'),
                                                                      FileExtensions::Zip);

    if( package_spec_file_path != nullptr )
        *package_spec_file_path = PortableFunctions::PathReplaceFileExtension(package_file_path, FileExtensions::Json);

    return package_file_path;
}


std::optional<JsonNode> FileBasedSyncService::SendSyncMessage(const DeviceId& device_id, const SyncMessage& sync_message)
{
    // the message filename will appear as: [device_id]$[timestamp]$[counter].json
    // the counter will ensure that messages sent at the same time result in unique names
    if( m_syncMessageCounter == nullptr )
    {
        m_syncMessageCounter = std::make_unique<std::tuple<int64_t, size_t>>(sync_message.GetTimestamp(), 1);
    }

    else if( std::get<0>(*m_syncMessageCounter) == sync_message.GetTimestamp() )
    {
        ++std::get<1>(*m_syncMessageCounter);
    }

    else
    {
        std::get<0>(*m_syncMessageCounter) = sync_message.GetTimestamp();
        std::get<1>(*m_syncMessageCounter) = 1;
    }

    const std::string sync_message_file_path = FormatText("/CSPro/messages/%s$%s$%02d.json", device_id.c_str(),
                                                                                             IntToString(sync_message.GetTimestamp()).c_str(),
                                                                                             static_cast<int>(std::get<1>(*m_syncMessageCounter)));

    FileWriteText(sync_message_file_path, Json::ToJson(sync_message));

    return std::nullopt;
}


std::string FileBasedSyncService::StartParadataSync(const std::string& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer>(*this);

    return m_paradataSyncer->StartParadataSync(log_uuid);
}


void FileBasedSyncService::PutParadata(const std::string& paradata_log_file_path)
{
    return m_paradataSyncer->PutParadata(paradata_log_file_path);
}


std::vector<TemporaryFile> FileBasedSyncService::GetParadata()
{
    return m_paradataSyncer->GetParadata();
}


void FileBasedSyncService::StopParadataSync()
{
    m_paradataSyncer->StopParadataSync();

    m_paradataSyncer.reset();
}


std::shared_ptr<SyncListener> FileBasedSyncService::GetSharedSyncListener()
{
    return m_syncListener;
}


void FileBasedSyncService::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    m_syncListener = sync_listener;
    m_fileBasedConnection->SetSyncListener(std::move(sync_listener));
}


std::shared_ptr<FileBasedConnection> FileBasedSyncService::GetFileBasedConnection()
{
    return m_fileBasedConnection;
}
