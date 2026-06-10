#include "stdafx.h"
#include "DropboxConnection.h"
#include "HttpConnection.h"
#include "LoginAccessor.h"
#include "OAuth2Authorizer.h"
#include <zToolsO/MemoryStream.h>
#include <zJson/ValidJsonAsserter.h>
#include <zUtilO/SettingsDb.h>


CREATE_JSON_KEY(content_hash)
CREATE_JSON_KEY(cursor)
CREATE_JSON_KEY(entries)
CREATE_JSON_KEY(error_summary)
CREATE_JSON_KEY(has_more)
CREATE_JSON_KEY(session_id)


namespace
{
    constexpr std::string_view DropboxRPCEndpoint_sv     = "https://api.dropboxapi.com/2";
    constexpr std::string_view DropboxContentEndpoint_sv = "https://content.dropboxapi.com/2";

    constexpr int64_t DropboxUploadChunkSize             = 10 * 1024 * 1024; // 10 MB
}


DropboxConnection::DropboxConnection(std::shared_ptr<LoginAccessor> login_accessor, const cs::cref_optional<SyncConnectionString> sync_connection_string)
    :   m_loginAccessor(std::move(login_accessor)),
        m_loginEmail(sync_connection_string.has_value() ? CreateCopyOfPointerValue(sync_connection_string->GetProperty(SCSProperty::email)) : nullptr),
        m_httpConnection(m_loginAccessor->CreateHttpConnection())
{
    ASSERT(m_httpConnection != nullptr);
    ASSERT(!sync_connection_string.has_value() || sync_connection_string->GetType() == SyncServiceType::Dropbox);

    SyncLog::EnableLogging();
}


DropboxConnection::DropboxConnection(DropboxConnection&&) = default;


DropboxConnection::~DropboxConnection()
{
}


void DropboxConnection::HandleServerErrorResponse(const int message_number, const int http_response_code, const std::string& response_body) const
{
    std::string error;

    try
    {
        // try to parse the error from the HTTP response
        const JsonNode json_node = Json::Parse(response_body);
        error = FormatText("%s. (%d)", json_node.Get<std::string>(JK::error_summary).c_str(), http_response_code);
    }

    catch( const JsonParseException& )
    {
        SYNCLOG_ERROR << "Error server response: ";
        SYNCLOG_ERROR << response_body;

        // if the message can't be parsed, just use the HTTP error code;
        // don't show the body since it is probably HTML
        error = FormatText("Error %d", http_response_code);
    }

    SyncError::ThrowByMessageNumber(message_number, error);
}


void DropboxConnection::HandleServerErrorResponse(const int message_number, const HttpResponse& response) const
{
    HandleServerErrorResponse(message_number, response.http_status, response.body.ToString());
}


void DropboxConnection::HandleServerErrorResponse(const JsonParseException& exception, const std::string& response_body) const
{
    SYNCLOG_ERROR << "Invalid server response: " << exception.what();
    SYNCLOG_ERROR << response_body;
    throw SyncError(100121);
}


void DropboxConnection::HandleServerErrorResponse(const JsonParseException& exception, const JsonNode& json_node) const
{
    HandleServerErrorResponse(exception, json_node.GetNodeAsString());
}


HeaderList DropboxConnection::GetBaseHeaders() const
{
    HeaderList headers;

    if( !m_authorizationHeader.empty() )
        headers.Add(m_authorizationHeader);

    headers.Add_Accept_Json();

    return headers;
}


template<typename T>
T DropboxConnection::ExecuteRestProcessResponse(const HttpResponse& response, const int error_message_number)
{
    if( response.http_status != HttpResponse::Status_200_OK )
        HandleServerErrorResponse(error_message_number, response);

    if constexpr(std::is_same_v<T, JsonNode>)
    {
        const std::string response_body = response.body.ToString();

        try
        {
            return Json::Parse(response_body);
        }

        catch( const JsonParseException& exception )
        {
            HandleServerErrorResponse(exception, response_body);
        }
    }

    else
    {
        static_assert(std::is_same_v<T, void>);
    }
}


template<typename T/*= JsonNode>*/>
T DropboxConnection::ExecuteRestPost(std::string endpoint, const int error_message_number, HeaderList headers,
                                     std::istream& input_stream, const int64_t input_size_bytes)
{
    headers.Add_ContentLength(input_size_bytes);

    HttpRequestBuilder request_builder(std::move(endpoint), std::move(headers));
    request_builder.post(input_stream, input_size_bytes);

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if constexpr(std::is_same_v<T, HttpResponse>)
    {
        return response;
    }

    else
    {
        return ExecuteRestProcessResponse<T>(response, error_message_number);
    }
}


JsonNode DropboxConnection::ExecuteRPC(const std::string_view path_sv, const int error_message_number, std::string json_text)
{
    ASSERT(!path_sv.empty() && path_sv.front() == '/');
    AssertValidJson(json_text);

    HeaderList headers = GetBaseHeaders();
    headers.Add_ContentType_Json();

    const int64_t json_text_length = json_text.length();
    std::istringstream input_stream(std::move(json_text));

    return ExecuteRestPost(SO::Concatenate(DropboxRPCEndpoint_sv, path_sv), error_message_number,
                           std::move(headers),
                           input_stream, json_text_length);
}


HttpResponse DropboxConnection::ExecuteContentDownload(const std::string& remote_file_path, const std::string* const etag)
{
    HeaderList headers = GetBaseHeaders();
    headers.Add("Content-Type:")
           .AddJson("Dropbox-API-Arg", FormatText("{\"path\":%s}", GetDropboxPathJsonString(remote_file_path, false).c_str()));

    if( etag != nullptr )
        headers.Add_IfNoneMatch(*etag);

    std::istringstream empty_input_stream;

    return ExecuteRestPost<HttpResponse>(SO::Concatenate(DropboxContentEndpoint_sv, "/files/download"), 100110,
                                         std::move(headers),
                                         empty_input_stream, 0);
}


template<typename T/*= JsonNode>*/>
T DropboxConnection::ExecuteContentUpload(const std::string_view path_sv, const int error_message_number,
                                          const std::string_view dropbox_api_arg_json_text_sv,
                                          std::istream& input_stream, const int64_t input_size_bytes)
{
    ASSERT(!path_sv.empty() && path_sv.front() == '/');
    AssertValidJson(dropbox_api_arg_json_text_sv);

    HeaderList headers = GetBaseHeaders();
    headers.Add_ContentType_OctetStream()
           .AddJson("Dropbox-API-Arg", dropbox_api_arg_json_text_sv);

    return ExecuteRestPost<T>(SO::Concatenate(DropboxContentEndpoint_sv, path_sv), error_message_number,
                              std::move(headers),
                              input_stream, input_size_bytes);
}


std::string DropboxConnection::GetDropboxPathJsonString(std::string_view path_sv, const bool specify_root_folder_as_empty_string/* = false*/) const
{
    ASSERT(m_basePath.front() == '/' && m_basePath.back() == '/');
    ASSERT(path_sv == PortableFunctions::PathToForwardSlash(std::string(path_sv)));

    // paths ending in "/" are considered malformed in Dropbox API
    if( !path_sv.empty() && path_sv.back() == '/' )
        path_sv = path_sv.substr(0, path_sv.length() - 1);

    const std::string path = PortableFunctions::PathAppendForwardSlashToPath(m_basePath, path_sv);

    if( specify_root_folder_as_empty_string && path == "/" )
        return std::string(Json::Text::BlankString_sv);

    return Encoders::ToJsonString(path);
}


std::string DropboxConnection::GetAccountEmail()
{
    const JsonNode json_node = ExecuteRPC("/users/get_current_account", 100101,
                                          std::string(Json::Text::Null_sv));

    return json_node.Get<std::string>(JK::email);
}


std::string DropboxConnection::Connect(const std::string* const base_path_override)
{
    ASSERT(base_path_override == nullptr || *base_path_override == PortableFunctions::PathToForwardSlash(*base_path_override));

    Disconnect();
    ASSERT(m_authorizationHeader.empty());

    if( m_oauth2Authorizer == nullptr )
        m_oauth2Authorizer = std::make_unique<OAuth2Authorizer>(OAuth2ClientType::Dropbox, m_httpConnection);

    // check for a saved refresh token, which can be used to get a fresh access token
    const std::shared_ptr<SyncCredentialStore> sync_credential_store = m_loginAccessor->GetSyncCredentialStore();
    ASSERT(sync_credential_store != nullptr);

    SyncCredentialStore::OAuth2TokenCredentialManager oauth2_token_credential_manager = sync_credential_store->CreateOAuth2TokenCredentialManager("DropboxV2");
    std::optional<OAuth2Token> oauth2_token = oauth2_token_credential_manager.GetToken(m_loginEmail.get());
    bool is_new_authorization = false;

    if( oauth2_token.has_value() && !oauth2_token->GetRefreshToken().empty() )
    {
        try
        {
            oauth2_token = m_oauth2Authorizer->Refresh(oauth2_token->GetRefreshToken());
        }
        catch(...) { } // ignore errors getting the new token
    }

    // if no token exists, authorize (or reauthorize) access to Dropbox
    if( !oauth2_token.has_value() )
    {
        oauth2_token = m_oauth2Authorizer->Authorize();
        is_new_authorization = true;
    }

    ASSERT(oauth2_token.has_value() && !oauth2_token->GetAccessToken().empty());

    m_authorizationHeader = "Authorization: Bearer " + oauth2_token->GetAccessToken();

    // test that we can connect by getting the account's email address
    try
    {
        std::string account_email = GetAccountEmail();
        ASSERT(!account_email.empty());

        // when an email is specified, make sure that the user logged into the correct Dropbox account
        if( m_loginEmail != nullptr && *m_loginEmail != account_email )
        {
            throw SyncConnectionError("The user incorrectly logged into the account '%s', not '%s'.",
                                      account_email.c_str(), m_loginEmail->c_str());
        }

        SYNCLOG_INFO << "Dropbox account " << account_email;

        // save the authorization details if a new authorization
        if( is_new_authorization )
            oauth2_token_credential_manager.SaveToken(account_email, *oauth2_token);

        if( base_path_override == nullptr || SO::IsWhitespace(*base_path_override) )
        {
            m_basePath = "/";
        }

        else
        {
            m_basePath = PortableFunctions::PathEnsureTrailingForwardSlash(
                Path::CombineForwardSlash("/", *base_path_override)
            );
        }

        return account_email;
    }

    // on error, clear the authorization header so that it does not appear that we are connected
    catch(...)
    {
        m_authorizationHeader.clear();
        throw;
    }
}


void DropboxConnection::Disconnect()
{
    m_authorizationHeader.clear();

    ASSERT(!IsConnected());
}


void DropboxConnection::ProcessNon304Download(const HttpResponse& response, std::ostream& output_stream)
{
    ASSERT(response.http_status != HttpResponse::Status_304_NotModified);

    if( response.http_status != HttpResponse::Status_200_OK )
        HandleServerErrorResponse(100110, response);

    output_stream << response.body;

#ifdef _DEBUG
    const JsonNode result_json_node = Json::Parse(response.headers.GetValue("Dropbox-Api-Result"));
    SYNCLOG_INFO << "Downloaded " << result_json_node.Get<int64_t>(JK::size) << " bytes with content hash: " << result_json_node.GetOrConstruct<std::string>(JK::content_hash);
#endif
}


void DropboxConnection::Download(const std::string& remote_file_path, const std::string& local_file_path)
{
    DropboxConnection::Download(remote_file_path, local_file_path, Hash::Md5::CreateFromFile(local_file_path));
}


bool DropboxConnection::Download(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5)
{
    // check if the file has been downloaded before
    if( m_etagsDb == nullptr )
        m_etagsDb = std::make_unique<SettingsDb>("ETag.db", "Dropbox");

    const std::string* const existing_etag = !existing_file_md5.empty() ? m_etagsDb->Read<std::string*>(existing_file_md5) :
                                                                          nullptr;

    std::ofstream local_output_file_stream(local_file_path, std::ios::binary);

    const HttpResponse response = ExecuteContentDownload(remote_file_path, existing_etag);

    if( response.http_status == HttpResponse::Status_304_NotModified )
    {
        ASSERT(existing_etag != nullptr);
        return false;
    }

    ProcessNon304Download(response, local_output_file_stream);

    local_output_file_stream.close();

    // save the ETag
    const std::string new_etag = response.headers.GetValue("ETag");

    if( !new_etag.empty() )
    {
        const std::string downloaded_file_md5 = Hash::Md5::CreateFromFile(local_file_path);
        ASSERT(!downloaded_file_md5.empty() && ( existing_etag == nullptr || downloaded_file_md5 != existing_file_md5 ));

        m_etagsDb->Write(downloaded_file_md5, new_etag);
    }

    return true;
}


void DropboxConnection::Download(const std::string& remote_file_path, std::ostream& output_stream)
{
    const HttpResponse response = ExecuteContentDownload(remote_file_path, nullptr);

    ProcessNon304Download(response, output_stream);
}


void DropboxConnection::Upload(std::istream& input_stream, const int64_t input_size_bytes, const std::string& remote_file_path)
{
    // upload large amounts of data using an upload session
    if( input_size_bytes > DropboxUploadChunkSize )
        return UploadUsingSession(input_stream, input_size_bytes, remote_file_path);

    const std::string dropbox_api_arg_json_text = FormatText("{\"path\":%s,\"mode\":\"overwrite\",\"mute\":true}", GetDropboxPathJsonString(remote_file_path, false).c_str());

    const JsonNode json_node = ExecuteContentUpload("/files/upload", 100111,
                                                    dropbox_api_arg_json_text,
                                                    input_stream, input_size_bytes);

#ifdef _DEBUG
    ASSERT(json_node.Get<int64_t>(JK::size) == input_size_bytes);
    SYNCLOG_INFO << "Uploaded " << input_size_bytes << " bytes with content hash: " << json_node.GetOrConstruct<std::string>(JK::content_hash);
#endif
}


void DropboxConnection::UploadUsingSession(std::istream& input_stream, const int64_t input_size_bytes, const std::string& remote_file_path)
{
    if( m_syncListener != nullptr )
        m_syncListener->SetProgressTotal(input_size_bytes);

    auto buffer = std::make_unique_for_overwrite<char[]>(DropboxUploadChunkSize);
    int64_t buffer_remaining = input_size_bytes;
    int64_t offset = 0;

    std::optional<MemoryStream> memory_stream;
    int64_t memory_stream_size;

    auto read_into_memory_stream = [&]()
    {
        memory_stream_size = std::min(DropboxUploadChunkSize, buffer_remaining);

        input_stream.read(buffer.get(), memory_stream_size);

        if( !input_stream )
            throw SyncError(100111, "stream reading error");

        buffer_remaining -= memory_stream_size;
        offset += memory_stream_size;

        memory_stream.emplace(buffer.get(), static_cast<size_t>(memory_stream_size));
    };

    // start the session
    std::string session_id;

    {
        read_into_memory_stream();

        const JsonNode start_json_node = ExecuteContentUpload("/files/upload_session/start", 100111,
                                                              "{\"close\":false}",
                                                              *memory_stream, memory_stream_size);

        try
        {
            session_id = start_json_node.Get<std::string>(JK::session_id);
        }

        catch( const JsonParseException& exception )
        {
            HandleServerErrorResponse(exception, start_json_node);
        }
    }

    if( m_syncListener != nullptr )
        m_syncListener->AddToProgressPreviousStepsTotal(memory_stream_size);

    // send additional chunks
    ASSERT(!session_id.empty());

    do
    {
        const std::string cursor_json = FormatText("\"cursor\":{\"session_id\":%s,\"offset\":%d}",
                                                   Encoders::ToJsonString(session_id).c_str(), static_cast<int>(offset));

        read_into_memory_stream();

        // append data if there are still chunks remaining
        if( buffer_remaining > 0 )
        {
            ExecuteContentUpload<void>("/files/upload_session/append_v2", 100111,
                                       FormatText("{\"close\":false,%s}", cursor_json.c_str()),
                                       *memory_stream, memory_stream_size);
        }

        // otherwise finalize the transfer
        else
        {
            ASSERT(buffer_remaining == 0);

            const std::string dropbox_api_arg_json_text = FormatText("{%s,\"commit\":{\"path\":%s,\"mode\":\"overwrite\",\"mute\":true}}",
                                                                     cursor_json.c_str(), GetDropboxPathJsonString(remote_file_path, false).c_str());

            const JsonNode finish_json_node = ExecuteContentUpload("/files/upload_session/finish", 100111,
                                                                   dropbox_api_arg_json_text,
                                                                   *memory_stream, memory_stream_size);

#ifdef _DEBUG
            ASSERT(finish_json_node.Get<int64_t>(JK::size) == input_size_bytes);
            SYNCLOG_INFO << "Uploaded " << input_size_bytes << " bytes with content hash: " << finish_json_node.GetOrConstruct<std::string>(JK::content_hash);
#endif
        }

        if( m_syncListener != nullptr )
            m_syncListener->AddToProgressPreviousStepsTotal(memory_stream_size);

    } while( buffer_remaining > 0 );

    ASSERT(buffer_remaining == 0 && offset == input_size_bytes);
}


FileInfo DropboxConnection::GetFileMetadata(const std::string& remote_path)
{
    const JsonNode json_node = ExecuteRPC("/files/get_metadata", 100153,
                                          FormatText("{\"path\":%s}", GetDropboxPathJsonString(remote_path, false).c_str()));

    try
    {
        return FileInfo::CreateFromDropboxJson(json_node);
    }

    catch( const JsonParseException& exception )
    {
        HandleServerErrorResponse(exception, json_node);
    }
}


std::optional<FileInfo::FileType> DropboxConnection::GetFileMetadataType(const std::string& remote_path) noexcept
{
    try
    {
        return GetFileMetadata(remote_path).GetType();
    }

    catch(...)
    {
        return std::nullopt;
    }
}


bool DropboxConnection::FileExists(const std::string& remote_path)
{
    return GetFileMetadataType(remote_path).has_value();
}


bool DropboxConnection::FileIsRegular(const std::string& remote_path)
{
    return ( GetFileMetadataType(remote_path) == FileInfo::FileType::File );
}


bool DropboxConnection::FileIsDirectory(const std::string& remote_path)
{
    return ( GetFileMetadataType(remote_path) == FileInfo::FileType::Directory );
}


int64_t DropboxConnection::FileModifiedTime(const std::string& remote_path)
{
    const FileInfo file_info = GetFileMetadata(remote_path);

    if( file_info.GetType() == FileInfo::FileType::File )
        return file_info.GetLastModified();

    throw SyncError(100153, FormatText("'%s' is not a file", remote_path.c_str()));
}


std::vector<FileInfo> DropboxConnection::GetFolderListing(const std::string& remote_path)
{
    std::vector<FileInfo> directory_listing;
    std::optional<std::string> continue_cursor;

    auto process_listing = [&](const JsonNode& json_node)
    {
        try
        {
            const JsonNodeArray entries_node = json_node.GetArray(JK::entries);

            directory_listing.reserve(directory_listing.size() + entries_node.size());

            for( const JsonNode& entry_node : entries_node )
                directory_listing.emplace_back(FileInfo::CreateFromDropboxJson(entry_node));

            // see if there are additional listings
            if( json_node.Get<bool>(JK::has_more) )
            {
                continue_cursor = json_node.Get<std::string>(JK::cursor);
            }

            else
            {
                continue_cursor.reset();
            }
        }

        catch( const JsonParseException& exception )
        {
            HandleServerErrorResponse(exception, json_node);
        }
    };

    // process the initial listing
    process_listing(ExecuteRPC("/files/list_folder", 100112,
                               FormatText("{\"path\":%s}", GetDropboxPathJsonString(remote_path, true).c_str())));

    // process any additional listings
    while( continue_cursor.has_value() )
    {
        if( m_syncListener != nullptr && m_syncListener->IsCanceled() )
            throw SyncCancelException();

        process_listing(ExecuteRPC("/files/list_folder/continue", 100112,
                                   FormatText("{\"cursor\":%s}", Encoders::ToJsonString(*continue_cursor).c_str())));
    }

    return directory_listing;
}


std::vector<FileInfo> DropboxConnection::GetDirectoryListing(const std::string& remote_directory_path, bool /*request_file_md5s*/)
{
    return GetFolderListing(remote_directory_path);
}


void DropboxConnection::FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path)
{
    const JsonNode json_node = ExecuteRPC("/files/move_v2", 100177,
                                          FormatText("{\"from_path\":%s,\"to_path\":%s}", GetDropboxPathJsonString(old_remote_file_path, false).c_str(),
                                                                                          GetDropboxPathJsonString(new_remote_file_path, false).c_str()));

#ifdef _DEBUG
    try
    {
        const FileInfo file_info = FileInfo::CreateFromDropboxJson(json_node.Get(JK::metadata));
    }

    catch( const JsonParseException& exception )
    {
        ErrorMessage::Display(exception);
    }
#endif
}


void DropboxConnection::DeleteFileOrFolder(const std::string& remote_path)
{
    const JsonNode json_node = ExecuteRPC("/files/delete", 100175,
                                          FormatText("{\"path\":%s}", GetDropboxPathJsonString(remote_path, false).c_str()));

#ifdef _DEBUG
    try
    {
        const FileInfo file_info = FileInfo::CreateFromDropboxJson(json_node);
    }

    catch( const JsonParseException& exception )
    {
        ErrorMessage::Display(exception);
    }
#endif
}


void DropboxConnection::FileDelete(const std::string& remote_file_path)
{
    DeleteFileOrFolder(remote_file_path);
}


void DropboxConnection::DirectoryDelete(const std::string& remote_directory_path)
{
    DeleteFileOrFolder(remote_directory_path);
}


std::string DropboxConnection::GetLocalDropboxInfoFilePath()
{
    std::string local_dropbox_info_file_path;

#ifdef WIN_DESKTOP
    auto locate = [&](REFKNOWNFOLDERID rfid)
    {
        PWSTR app_data_path;

        if( SUCCEEDED(SHGetKnownFolderPath(rfid, 0, nullptr, &app_data_path)) )
            local_dropbox_info_file_path = Path::Combine(TC::ToUtf8(app_data_path), "Dropbox", "info.json");

        CoTaskMemFree(app_data_path);

        return ( !local_dropbox_info_file_path.empty() &&
                 PortableFunctions::FileIsRegular(local_dropbox_info_file_path) );
    };

    // look in both the roaming and local app data directories
    if( !locate(FOLDERID_RoamingAppData) )
        locate(FOLDERID_LocalAppData);
#endif

    return local_dropbox_info_file_path;
}


std::string DropboxConnection::DropboxConnection::GetLocalDropboxDirectory(const std::string* const account_name)
{
    // Get directory from Dropbox info.json file see https://help.dropbox.com/installs-integrations/desktop/locate-dropbox-folder
    const std::string local_dropbox_info_file_path = GetLocalDropboxInfoFilePath();

    if( !local_dropbox_info_file_path.empty() )
    {
        std::string local_dropbox_directory = GetLocalDropboxDirectoryFromInfoFilePath(local_dropbox_info_file_path, account_name);

        if( !local_dropbox_directory.empty() )
            return local_dropbox_directory;
    }

    // Couldn't find it using the input file, try the default location
    std::string local_dropbox_directory;

#ifdef WIN_DESKTOP
    if( account_name == nullptr )
    {
        PWSTR user_profile_path;

        if( SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &user_profile_path)) )
        {
            local_dropbox_directory = Path::Combine(TC::ToUtf8(user_profile_path), "Dropbox");

            if( !PortableFunctions::FileIsDirectory(local_dropbox_directory) )
                local_dropbox_directory.clear();
        }

        CoTaskMemFree(user_profile_path);
    }
#endif

    return local_dropbox_directory;
}


std::string DropboxConnection::GetLocalDropboxDirectoryFromInfoFilePath(const std::string& local_dropbox_info_file_path, const std::string* const account_name)
{
    std::string local_dropbox_directory;

    try
    {
        const JsonNode json_node = Json::ParseFile(local_dropbox_info_file_path);

        auto test = [&](const char* const key)
        {
            const JsonNode test_node = json_node.GetOrEmpty(key);

            if( !test_node.IsEmpty() && test_node.Contains(JK::path) )
            {
                local_dropbox_directory = test_node.Get<std::string>(JK::path);
                return true;
            }

            return false;
        };

        // look for a specific account name...
        if( account_name != nullptr )
        {
            test(account_name->c_str());
        }

        // ...or test both the personal and business nodes
        else
        {
            if( !test("personal") )
                test("business");
        }
    }
    catch(...) { ASSERT(!PortableFunctions::FileIsRegular(local_dropbox_info_file_path)); }

    return local_dropbox_directory;
}


std::string DropboxConnection::GetEvaluatedLocalDropboxDirectory(const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Dropbox);

    const std::string* const use_local_property = sync_connection_string.GetProperty(SCSProperty::useLocal);
    bool must_use_local = false;

    // if useLocal is false (or invalid), there is no need to evaluate anything more
    if( use_local_property != nullptr )
    {
        if( SO::EqualsNoCase(*use_local_property, SCSValue::true_) )
        {
            must_use_local = true;
        }

        else if( !SO::EqualsNoCase(*use_local_property, SCSValue::try_) )
        {
            return std::string();
        }
    }

    const std::string* const account_property = sync_connection_string.GetProperty(SCSProperty::account);

    // if useLocal is not defined and no account is defined, default to using the remote
    if( use_local_property == nullptr && account_property == nullptr )
        return std::string();

    std::string local_dropbox_directory = GetLocalDropboxDirectory(account_property);

    if( local_dropbox_directory.empty() && must_use_local )
        throw SyncError(100148, ( account_property != nullptr ) ? *account_property : "personal");

    return local_dropbox_directory;
}
