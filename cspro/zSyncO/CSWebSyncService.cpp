#include "stdafx.h"
#include "CSWebSyncService.h"
#include "CaseObservable.h"
#include "ExponentialBackoff.h"
#include "FileBasedParadataSyncer.h"
#include "JsonConverter.h"
#include "NetworkDataChunk.h"
#include "SyncDictionaryInfo.h"
#include "SyncExceptionRethrower.h"
#include "SyncMessage.h"
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/HttpConnection.h>
#include <zToolsO/base64.h>
#include <zToolsO/Encoders.h>
#include <zNetwork/SyncCustomHeaders.h>
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncCaseIncrementalParser.h>


namespace
{
    constexpr bool IsRetryableHttpError(const int status)
    {
        // Retry all server errors (500 and above) and
        // too many requests (429) in case of throttling
        return ( status >= HttpResponse::Status_500_InternalServerError ||
                 status == HttpResponse::Status_429_TooManyRequests );
    }

    // Maximum number of times to retry failed requests
    constexpr int MAX_RETRIES = 3;
}


CSWebSyncService::CSWebSyncService(std::unique_ptr<CSWebConnection> csweb_connection)
    :   m_cswebConnection(std::move(csweb_connection)),
        m_dataChunk(std::make_shared<NetworkDataChunk>())
{
    ASSERT(m_cswebConnection != nullptr);
}


CSWebSyncService::CSWebSyncService(std::unique_ptr<HttpConnection> http_connection,
                                   SyncConnectionString sync_connection_string, LoginCredentials login_credentials)
    :   CSWebSyncService(std::make_unique<CSWebConnection>(std::move(http_connection), std::move(sync_connection_string), std::move(login_credentials)))
{
}


CSWebSyncService::~CSWebSyncService()
{
}


std::shared_ptr<ConnectResponse> CSWebSyncService::Connect()
{
    return m_cswebConnection->Connect(CSWebVersion::V1);
}


void CSWebSyncService::Disconnect()
{
}


SyncGetResponse CSWebSyncService::GetCases(std::shared_ptr<const CaseAccess> case_access,
                                           const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                           const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions)
{
    ASSERT(case_access != nullptr);
    const std::string url = m_cswebConnection->GetHostUrl() + "dictionaries/" + case_access->GetDataDict().GetSyncableName() + "/cases";

    return DownloadServerCases(url, std::move(case_access), device_id, universe, last_server_revision, last_case_uuid, excluded_revisions);
}


SyncPutResponse CSWebSyncService::PutCases(std::shared_ptr<const CaseAccess> case_access,
                                           const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* const sync_binary_data_upload_manager,
                                           const DeviceId& device_id, const std::string& /*universe*/, const std::string& last_server_revision)
{
    ASSERT(case_access != nullptr);
    const std::string url = m_cswebConnection->GetHostUrl() + "dictionaries/" + case_access->GetDataDict().GetSyncableName() + "/cases";

    SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSWebApiVersion(std::move(case_access), m_cswebConnection->GetApiVersion());

    std::string case_data = sync_case_serializer.GetSyncableCaseData(cases, sync_binary_data_upload_manager);

    if( m_cswebConnection->ApiSupportsCompressingCaseUpload() )
        ZLib::Deflate(case_data);

    return UploadClientCases(url, device_id, last_server_revision, case_data, cases.size());
}


IDataChunk& CSWebSyncService::GetChunk()
{
    return *m_dataChunk;
}


std::vector<SyncDictionaryInfo> CSWebSyncService::GetDictionaries()
{
    const JsonNode json_node = m_cswebConnection->GetDictionariesList();

    try
    {
        return json_node.GetArray().GetVector<SyncDictionaryInfo>();
    }

    catch( const JsonParseException& exception )
    {
        m_cswebConnection->HandleServerErrorResponse(exception, json_node);
    }
}


std::string CSWebSyncService::GetDictionary(const std::string& dictionary_name)
{
    return m_cswebConnection->GetDictionarySpec(dictionary_name);
}


void CSWebSyncService::PutDictionary(const CDataDict& dictionary)
{
    m_cswebConnection->PutDictionarySpec(dictionary.GetJson(true));
}


void CSWebSyncService::DeleteDictionary(const std::string& dictionary_name)
{
    m_cswebConnection->DeleteDictionarySpec(dictionary_name);
}


std::string CSWebSyncService::GetHeaderFallbackToEtag(const HeaderList& response_headers, const std::string_view header_sv)
{
    std::string revision = response_headers.GetValue(header_sv);

    if( revision.empty() )
        revision = response_headers.GetValue("Etag");

    return revision;
}


std::string CSWebSyncService::GetChunkMaxRevision(const HeaderList& response_headers)
{
    // For newer CSWeb we use CHUNK_MAX_REVISION_HEADER but for older versions we used etag
    return GetHeaderFallbackToEtag(response_headers, SyncCustomHeaders::CHUNK_MAX_REVISION_HEADER);
}


std::string CSWebSyncService::GetServerRevision(const HeaderList& response_headers)
{
    // For newer CSWeb we use CURRENT_REVISION_HEADER but for older versions we used etag
    return GetHeaderFallbackToEtag(response_headers, SyncCustomHeaders::CURRENT_REVISION_HEADER);
}


SyncGetResponse CSWebSyncService::DownloadServerCases(const std::string& url, std::shared_ptr<const CaseAccess> case_access,
                                                      const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                                      const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions)
{
    SYNCLOG_INFO << "Start case download with chunk size of " << m_dataChunk->GetCaseSize();

    std::string server_revision = last_server_revision;

    if( !last_case_uuid.empty() )
        SYNCLOG_INFO << "Download chunk starting from case " << last_case_uuid;

    auto sync_case_incremental_parser = std::make_shared<SyncCaseIncrementalParser>(SyncCaseSerializer::CreateFromCSWebApiVersion(std::move(case_access), m_cswebConnection->GetApiVersion()));

    while( true )
    {
        m_dataChunk->ResetForNextChunk();

        HttpResponse http_response = SendDownloadServerCasesRequest(url, device_id, universe, server_revision, last_case_uuid, excluded_revisions);

        SYNCLOG_INFO << "Status : " << http_response.http_status;

        if( http_response.http_status == HttpResponse::Status_200_OK ||
            http_response.http_status == HttpResponse::Status_206_PartialContent )
        {
            SYNCLOG_INFO << "Server case count = " << http_response.headers.GetValue(SyncCustomHeaders::RANGE_COUNT_HEADER);

            const SyncGetResponse::SyncGetResult result = ( http_response.http_status == HttpResponse::Status_200_OK ) ? SyncGetResponse::SyncGetResult::Complete :
                                                                                                                         SyncGetResponse::SyncGetResult::MoreData;
            server_revision = GetChunkMaxRevision(http_response.headers);

            if( server_revision.empty() )
            {
                SYNCLOG_ERROR << "Server response missing revision header " << SyncCustomHeaders::CHUNK_MAX_REVISION_HEADER;

                SYNCLOG_ERROR << "HEADERS:";
                for( const std::string& header : http_response.headers.GetHeaders() )
                    SYNCLOG_ERROR << header;

                SYNCLOG_ERROR << "BODY:" << http_response.body.ToString();

                throw SyncError(100121);
            }

            auto observable = std::make_unique<CaseObservable>(rxcpp::observable<>::create<std::shared_ptr<Case>>(
                [sync_case_incremental_parser, http_response, data_chunk = m_dataChunk](rxcpp::subscriber<std::shared_ptr<Case>> case_subscriber)
                {
                    http_response.body.observable.subscribe(
                        // on next
                        [sync_case_incremental_parser, &case_subscriber](const std::string s)
                        {
                            sync_case_incremental_parser->Update(s);

                            const std::unique_ptr<std::vector<std::shared_ptr<Case>>> parseable_cases = sync_case_incremental_parser->ReleaseParseableCases();

                            if( parseable_cases != nullptr )
                            {
                                for( std::shared_ptr<Case>& data_case : *parseable_cases )
                                    case_subscriber.on_next(std::move(data_case));
                            }
                        },

                        // on error (passed to the case subscriber)
                        [&case_subscriber](std::exception_ptr exception)
                        {
                            case_subscriber.on_error(exception);
                        },

                        // on complete
                        [sync_case_incremental_parser, data_chunk, &case_subscriber, &http_response]()
                        {
                            try
                            {
                                for( std::shared_ptr<Case>& data_case : sync_case_incremental_parser->Finish() )
                                    case_subscriber.on_next(std::move(data_case));

                                // Only increase chunk size if chunk is full
                                if( http_response.http_status == HttpResponse::Status_206_PartialContent )
                                {
                                    data_chunk->Optimize(sync_case_incremental_parser->GetParsedCaseCount(),
                                                         sync_case_incremental_parser->GetParsedCaseJsonLength());
                                }

                                case_subscriber.on_completed();
                            }

                            catch(...)
                            {
                                case_subscriber.on_error(std::current_exception());
                            }
                        });
                    }));

            // the range count will come as count/total
            const std::string range_count_header = http_response.headers.GetValue(SyncCustomHeaders::RANGE_COUNT_HEADER);
            const size_t slash_pos = range_count_header.find('/');
            const std::optional<int> total_cases = ( slash_pos != std::string::npos ) ? std::make_optional(atoi(range_count_header.c_str() + slash_pos + 1)) :
                                                                                        std::nullopt;

            return SyncGetResponse(result, std::move(observable), std::move(server_revision), total_cases);
        }

        else if( http_response.http_status == HttpResponse::Status_401_Unauthorized )
        {
            // Get new OAuth 2.0 token and try again
            if( !m_cswebConnection->RefreshOAuth2Token() )
                m_cswebConnection->HandleServerErrorResponse(100101, http_response.http_status, http_response.body.ToString());
        }

        else if( http_response.http_status == HttpResponse::Status_412_Precondition_Failed )
        {
            return SyncGetResponse::SyncGetResult::RevisionNotFound;
        }

        else
        {
            m_cswebConnection->HandleServerErrorResponse(100101, http_response.http_status, http_response.body.ToString());
        }
    }
}


SyncPutResponse CSWebSyncService::UploadClientCases(const std::string& url, const DeviceId& device_id, const std::string& last_server_revision,
                                                    const std::string& case_data, const size_t num_cases)
{
    HeaderList response_headers;
    std::string response_body;

    m_dataChunk->ResetForNextChunk();

    int status = SendUploadClientCasesRequest(url, case_data, device_id, last_server_revision, response_headers, response_body);

    if( status == HttpResponse::Status_401_Unauthorized && m_cswebConnection->RefreshOAuth2Token() )
        status = SendUploadClientCasesRequest(url, case_data, device_id, last_server_revision, response_headers, response_body);

    if( status == HttpResponse::Status_200_OK )
    {
        std::string server_revision = GetServerRevision(response_headers);

        if( server_revision.empty() )
        {
            SYNCLOG_ERROR << "Server response missing revision header " << SyncCustomHeaders::CURRENT_REVISION_HEADER;

            for( const std::string& header : response_headers.GetHeaders() )
                SYNCLOG_ERROR << header;

            SYNCLOG_ERROR << "BODY:" << response_body;

            throw SyncError(100121);
        }

        m_dataChunk->Optimize(num_cases, case_data.size());

        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, std::move(server_revision));
    }

    else if( status == HttpResponse::Status_412_Precondition_Failed )
    {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
    }

    else
    {
        m_cswebConnection->HandleServerErrorResponse(100101, status, response_body);
    }
}


int CSWebSyncService::SendUploadClientCasesRequest(const std::string& url, const std::string& client_cases_json, const DeviceId& device_id, const std::string& last_server_revision,
                                                   HeaderList& response_headers, std::string& response_body)
{
    HeaderList headers;
    headers.Add(m_cswebConnection->m_authorizationHeader)
           .Add_ContentType_Json()
           .Add_Accept_Json()
           .Add("Cookie: XDEBUG_SESSION=netbeans-xdebug")
           .Add_UserAgent_CSProSyncClient()
           .Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id);

    if( m_cswebConnection->ApiSupportsCompressingCaseUpload() )
    {
        if( m_cswebConnection->m_apiVersion >= CSWebVersion::V3 )
        {
            headers.Add_ContentEncoding_Deflate();
        }

        // prior to CSPro 8.1, the compressed data was erroneously referred to as gzip
        else
        {
            ASSERT(m_cswebConnection->m_apiVersion == CSWebVersion::V2);
            headers.Add("Content-Encoding: gzip");
        }
    }

    headers.AddIfNotBlank(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, last_server_revision);

    ExponentialBackOff backOff;
    int retries = 0;

    while( true )
    {
        try
        {
            std::istringstream postDataStream(client_cases_json);
            const std::chrono::steady_clock::time_point requestStartTime = std::chrono::high_resolution_clock::now();

            HttpResponse response = m_cswebConnection->m_httpConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, client_cases_json.size()).build());
            response_body = response.body.ToString();

            response_headers = std::move(response.headers);

            if( !IsRetryableHttpError(response.http_status) )
                return response.http_status;

            SYNCLOG_ERROR << "HTTP error: " << response.http_status;
            SYNCLOG_ERROR << response_body;

            if( retries == MAX_RETRIES )
                break;
        }

        catch( const SyncRetryableNetworkError& exception )
        {
            SYNCLOG_ERROR << "Network error: " << exception.what();

            if( retries == MAX_RETRIES )
                throw exception;
        }

        m_dataChunk->OnError();
        ++retries;

        SYNCLOG_INFO << "Retrying. Attempt #" << retries;
        Sleep(backOff.NextBackOffMillis());
    }

    return HttpResponse::Status_500_InternalServerError; // should never get here
}


HttpResponse CSWebSyncService::SendDownloadServerCasesRequest(const std::string& url, const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                                              const std::string& last_case_uuid, const std::vector<std::string>& exclude_revisions)
{
    HeaderList headers;
    headers.Add(m_cswebConnection->m_authorizationHeader)
           .Add_ContentType_Json()
           .Add_Accept_Json()
           .Add("Cookie: XDEBUG_SESSION=netbeans-xdebug")
           .Add_UserAgent_CSProSyncClient()
           .Add("Cache-Control: no-cache, no-store, must-revalidate")
           .Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id);

    if( !universe.empty() )
        headers.Add(SyncCustomHeaders::UNIVERSE_HEADER, "\"" + universe + "\"");

    headers.AddIfNotBlank(SyncCustomHeaders::START_AFTER_HEADER, last_case_uuid);

    if( !exclude_revisions.empty() )
        headers.Add(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER, SO::CreateSingleString(exclude_revisions, ","));

    headers.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, IntToString(m_dataChunk->GetCaseSize()));

    headers.AddIfNotBlank(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, last_server_revision);

    std::ostringstream result;
    ExponentialBackOff backOff;
    int retries = 0;

    while( true )
    {
        try
        {
            HttpResponse response = m_cswebConnection->m_httpConnection->Request(HttpRequestBuilder(url).headers(headers).build());

            if( !IsRetryableHttpError(response.http_status) )
                return response;

            SYNCLOG_ERROR << "HTTP error: " << response.http_status;
            SYNCLOG_ERROR << response.body.ToString();

            if( retries == MAX_RETRIES )
                break;
        }

        catch( const SyncRetryableNetworkError& exception )
        {
            SYNCLOG_ERROR << "Network error: " << exception.what();

            if( retries == MAX_RETRIES )
                throw exception;
        }

        m_dataChunk->OnError();
        ++retries;

        SYNCLOG_INFO << "Retrying. Attempt #" << retries;
        Sleep(backOff.NextBackOffMillis());
    }

    return HttpResponse::Status_500_InternalServerError; // should never get here
}


bool CSWebSyncService::GetFile(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5)
{
    return m_cswebConnection->GetFile(remote_file_path, local_file_path, existing_file_md5);
}


std::unique_ptr<TemporaryFile> CSWebSyncService::GetFileIfExists(const std::string& remote_file_path)
{
    try
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        if( GetFile(remote_file_path, temporary_file->GetPath(), std::string()) )
            return temporary_file;
    }

    catch( const SyncError& sync_error )
    {
        // rethrow the error if it was anything other than "Error downloading file"
        if( sync_error.GetErrorMessageNumber() != 100110 )
            throw;
    }

    return nullptr;
}


void CSWebSyncService::PutFile(const std::string& local_file_path, const std::string& remote_path)
{
    return m_cswebConnection->PutFile(local_file_path, remote_path);
}


std::vector<FileInfo> CSWebSyncService::GetDirectoryListing(const std::string& remote_path, const bool request_file_md5s)
{
    return m_cswebConnection->GetDirectoryListing(remote_path, request_file_md5s);
}


std::vector<ApplicationPackage> CSWebSyncService::ListApplicationPackages()
{
    const JsonNode json_node = m_cswebConnection->GetApplicationsList();

    try
    {
        return JsonConverter::CreateApplicationPackageListFromJson(json_node);
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_ERROR << "Invalid server response: " << exception.what();
        SYNCLOG_ERROR << Json::ToJson(json_node);
        throw SyncError(100121);
    }
}


bool CSWebSyncService::DownloadApplicationPackage(const std::string& package_name, const std::string& local_file_path,
                                                  const ApplicationPackage* const current_package, const std::string* const current_package_signature)
{
    const std::string endpoint_path = PortableFunctions::PathAppendForwardSlashToPath("apps", Encoders::ToUri(package_name));
    std::unique_ptr<HeaderList> additional_headers;
    std::optional<size_t> package_files_header_size;

    // For an existing package we add the build time and the package file list in the headers
    if( current_package != nullptr )
    {
        additional_headers = std::make_unique<HeaderList>();
        additional_headers->Add(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER, DateTime::TimeToRFC3339(current_package->GetBuildTime()));

        // Convert the file list to base64 encoded, compressed json
        std::vector<ApplicationPackage::File> package_files = current_package->GetFiles();

        // No need to send the only on first install flag to server, setting to false removes from JSON
        for( ApplicationPackage::File& file : package_files )
            file.only_on_first_install = false;

        std::string file_json = JsonConverter::ToJson(package_files);

        if( ZLib::Deflate(file_json) )
        {
            const std::string package_files_header_data = Base64::Encode(file_json);
            package_files_header_size = package_files_header_data.size();
            additional_headers->Add(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER, package_files_header_data);
        }

        else
        {
            SYNCLOG_ERROR << "Failed to compress package json, fallback to full package download";
        }
    }

    const std::function<void(int)> http_status_other_than_200_callback =
        [&](const int http_status)
        {
            if( http_status == HttpResponse::Status_400_BadRequest && package_files_header_size >= static_cast<size_t>(8000) )
            {
                // bad request - likely due to big header
                SYNCLOG_ERROR << "Bad request - size of request header is "
                              << static_cast<int>(*package_files_header_size)
                              << " bytes. That could be greater than the max allowable header on the server (default of 8kb in Apache)."
                              << "Try increasing LimitRequestFieldSize in the Apache config file on the server or decreasing the number of files in your package.";

                throw SyncError(100158, package_name);
            }
        };

    TemporaryFile temporary_file(PortableFunctions::PathGetDirectory(local_file_path));
    const bool file_downloaded = m_cswebConnection->GetFileUsingEndpoint(package_name, temporary_file.GetPath(), cs::cref_optional<std::string>::FromPointer(current_package_signature),
                                                                        endpoint_path, 100157, std::move(additional_headers), &http_status_other_than_200_callback);

    if( file_downloaded )
    {
        if( !temporary_file.Rename_noexcept(local_file_path))
            throw SyncError(100109, local_file_path);

        SYNCLOG_INFO << "Downloaded application package " << package_name;
    }

    else
    {
        SYNCLOG_INFO << "Application package already up to date";
    }

    return file_downloaded;
}


void CSWebSyncService::UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name, const std::string& /*package_spec_json*/)
{
    const int64_t file_size = PortableFunctions::FileSize(local_package_zip_file_path);
    SYNCLOG_INFO << "Uploading app package " << package_name << " " << file_size << " bytes";

    const std::string endpoint_path = PortableFunctions::PathAppendForwardSlashToPath("apps", Encoders::ToUri(package_name));
    m_cswebConnection->PutFileUsingEndpoint(local_package_zip_file_path, endpoint_path, 100140);
}


void CSWebSyncService::DeleteApplication(const std::string& package_name)
{
    m_cswebConnection->DeleteApplication(package_name);
}


std::optional<JsonNode> CSWebSyncService::SendSyncMessage(const DeviceId& device_id, const SyncMessage& sync_message)
{
    auto additional_headers = std::make_unique<HeaderList>();
    additional_headers->Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id);

    return m_cswebConnection->ExecuteRestPostJson<JsonNode>("messages/", 100154,
                                                            Json::ToJson(sync_message), false,
                                                            std::move(additional_headers));
}


std::string CSWebSyncService::StartParadataSync(const std::string& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer>(*this, "/paradata/");

    return m_paradataSyncer->StartParadataSync(log_uuid);
}


void CSWebSyncService::PutParadata(const std::string& paradata_log_file_path)
{
    return m_paradataSyncer->PutParadata(paradata_log_file_path);
}


std::vector<TemporaryFile> CSWebSyncService::GetParadata()
{
    return m_paradataSyncer->GetParadata();
}


void CSWebSyncService::StopParadataSync()
{
    m_paradataSyncer->StopParadataSync();

    m_paradataSyncer.reset();
}


std::shared_ptr<SyncListener> CSWebSyncService::GetSharedSyncListener()
{
    return m_cswebConnection->m_httpConnection->GetSharedSyncListener();
}


void CSWebSyncService::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    m_cswebConnection->m_httpConnection->SetSyncListener(std::move(sync_listener));
}


std::shared_ptr<FileBasedConnection> CSWebSyncService::GetFileBasedConnection()
{
    return nullptr;
}
