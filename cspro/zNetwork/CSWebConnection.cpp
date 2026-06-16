#include "stdafx.h"
#include "CSWebConnection.h"
#include "ConnectResponse.h"
#include "HeaderList.h"
#include "HttpConnection.h"
#include "OAuth2Token.h"
#include "SyncCustomHeaders.h"
#include <zToolsO/ApiKeys.h>
#include <zToolsO/MemoryStream.h>
#include <zJson/ValidJsonAsserter.h>
#include <zUtilO/FileExtensions.h>
#include <zZip/ZLib.h>


CSWebConnection::CSWebConnection(std::unique_ptr<HttpConnection> http_connection, SyncConnectionString sync_connection_string, LoginCredentials login_credentials)
    :   m_httpConnection(std::move(http_connection)),
        m_syncConnectionString(std::move(sync_connection_string)),
        m_hostUrl(PortableFunctions::PathEnsureTrailingForwardSlash(m_syncConnectionString.GetUrl())),
        m_loginCredentials(std::move(login_credentials)),
        m_apiVersion(CSWebVersion::V1)
{
    ASSERT(m_httpConnection != nullptr);
    ASSERT(m_syncConnectionString.GetType() == SyncServiceType::CSWeb);
    ASSERT(!m_hostUrl.empty() && m_hostUrl.back() == '/');

    SyncLog::EnableLogging();
}


CSWebConnection::~CSWebConnection()
{
}


void CSWebConnection::CreateAuthorizationHeader(const std::string& access_token)
{
    ASSERT(!access_token.empty());
    m_authorizationHeader = "Authorization: Bearer " + access_token;
}


template<bool ResponseAcceptsJson/* = true*/>
HeaderList CSWebConnection::GetBaseHeaders() const
{
    HeaderList headers;

    if( !m_authorizationHeader.empty() )
        headers.Add(m_authorizationHeader);

    headers.Add_UserAgent_CSProSyncClient();

    if constexpr(ResponseAcceptsJson)
        headers.Add_Accept_Json();

#ifdef _DEBUG
    headers.Add("Cookie: XDEBUG_SESSION=netbeans-xdebug");
#endif

    return headers;
}


std::unique_ptr<ConnectResponse> CSWebConnection::Connect(const double minimum_api_version_required)
{
    std::string access_token;

    auto create_authorization_header_and_retrieve_server_info = [&](std::string username)
    {
        CreateAuthorizationHeader(access_token);

        std::unique_ptr<ConnectResponse> connect_response = RetrieveServerInfo(std::move(username));

        m_apiVersion = connect_response->GetApiVersion();

        // check that this API version is valid
        if( m_apiVersion < minimum_api_version_required )
        {
            SYNCLOG_ERROR << "Server version " << m_apiVersion << " is not compatible with the operation, which requires version " << minimum_api_version_required;
            throw SyncError(100131);
        }

        if( ( m_apiVersion >= CSWebVersion::V3 ) &&
            ( !m_user.has_value() || m_user->id.empty() || m_user->role_name.empty() ) )
        {
            constexpr const char* error = "Server version is 3.0+ but CSWeb did not send user details.";
            SYNCLOG_ERROR << error;
            throw SyncError(100101, error);
        }

        return connect_response;
    };

    // when specified, log in using a username/password
    std::optional<UsernamePassword> username_password = m_loginCredentials.GetEvaluatedUsernamePassword(m_syncConnectionString);

    if( username_password.has_value() )
    {
        std::tie(access_token, m_refreshToken, m_user) = RetrieveOAuth2Token(*username_password);
        return create_authorization_header_and_retrieve_server_info(username_password->username);
    }

    // otherwise try using saved tokens
    std::tie(access_token, m_refreshToken, m_user) = GetOAuth2TokenFromSyncCredentialStore();

    if( !access_token.empty() )
    {
        try
        {
            return create_authorization_header_and_retrieve_server_info(std::string());
        }

        catch( const SyncError& exception )
        {
            // ignore 401 (unauthorized) errors, pass all others on
            if( exception.GetErrorMessageNumber() != 100101 )
                throw exception;
        }
    }

    // at this point, there is not an access token, or the saved token is bad;
    // keep prompting for credentials until we get successful login, cancel or error
    ASSERT(m_loginCredentials.UseLoginAccessor());

    bool login_failed = false;

    while( true )
    {
        // username/password
        username_password = m_loginCredentials.GetLoginAccessor().QueryUsernamePassword(m_hostUrl, login_failed);

        if( !username_password.has_value() )
            throw SyncCancelException();

        try
        {
            std::tie(access_token, m_refreshToken, m_user) = RetrieveOAuth2Token(*username_password);
            UpdateOAuth2TokenInSyncCredentialStore(access_token);
            return create_authorization_header_and_retrieve_server_info(std::move(username_password->username));
        }

        catch( const SyncLoginDeniedError& )
        {
            login_failed = true;
        }
    }
}


std::tuple<std::string, std::string, std::optional<CSWebUser>> CSWebConnection::RetrieveOAuth2Token(const UsernamePassword& username_password)
{
    const OAuth2TokenRequest token_request = OAuth2TokenRequest::CreatePasswordRequest(CSWebKeys::client_id, CSWebKeys::client_secret,
                                                                                       username_password.username, username_password.password);

    const std::function<void(int)> http_status_other_than_200_callback =
        [](const int http_status)
        {
            if( http_status == HttpResponse::Status_401_Unauthorized )
                throw SyncLoginDeniedError(100126);
        };

    const JsonNode json_node = ExecuteRestPostJson<JsonNode>("token", 100101,
                                                             Json::ToJson(token_request), false,
                                                             nullptr,
                                                             &http_status_other_than_200_callback);

    try
    {
        const bool api_v3_plus_response = json_node.Contains(JK::credentials);
        const OAuth2Token token_response = api_v3_plus_response ? json_node.Get<OAuth2Token>(JK::credentials) :
                                                                  json_node.Get<OAuth2Token>();

        return { token_response.GetAccessToken(),
                 token_response.GetRefreshToken(),
                 api_v3_plus_response ? json_node.GetOptional<CSWebUser>(JK::user) : std::nullopt };
    }

    catch( const JsonParseException& exception )
    {
        HandleServerErrorResponse(exception, json_node);
    }
}


bool CSWebConnection::RefreshOAuth2Token() noexcept
{
    if( m_refreshToken.empty() )
        return false;

    SYNCLOG_INFO << "Refreshing OAuth 2.0 token";

    try
    {
        const OAuth2TokenRequest token_request = OAuth2TokenRequest::CreateRefreshRequest(CSWebKeys::client_id, CSWebKeys::client_secret, m_refreshToken);

        const JsonNode json_node = ExecuteRestPostJson<JsonNode>("token", 100101,
                                                                 Json::ToJson(token_request), false);

        try
        {
            const OAuth2Token token_response = json_node.Get<OAuth2Token>();

            CreateAuthorizationHeader(token_response.GetAccessToken());
            m_refreshToken = token_response.GetRefreshToken();

            // save the tokens
            UpdateOAuth2TokenInSyncCredentialStore(token_response.GetAccessToken());

            SYNCLOG_INFO << "Got new OAuth 2.0 token";

            return true;
        }

        catch( const JsonParseException& exception )
        {
            HandleServerErrorResponse(exception, json_node);
        }
    }

    catch(...)
    {
        SYNCLOG_ERROR << "Failed to refresh OAuth 2.0 token ";
        return false;
    }
}


std::shared_ptr<SyncCredentialStore> CSWebConnection::GetSyncCredentialStore()
{
    return m_loginCredentials.UseLoginAccessor() ? m_loginCredentials.GetLoginAccessor().GetSyncCredentialStore() :
                                                   std::make_shared<SyncCredentialStore>();
}


std::tuple<std::string, std::string, std::optional<CSWebUser>> CSWebConnection::GetOAuth2TokenFromSyncCredentialStore()
{
    const std::shared_ptr<SyncCredentialStore> sync_credential_store = GetSyncCredentialStore();
    ASSERT(sync_credential_store != nullptr);

    try
    {
        const JsonNode json_node = Json::Parse(sync_credential_store->Retrieve(m_hostUrl));

        return { json_node.Get<std::string>(JK::accessToken),
                 json_node.Get<std::string>(JK::refreshToken),
                 json_node.GetOptional<CSWebUser>(JK::user) };
    }

    catch(...)
    {
        return { };
    }
}


void CSWebConnection::UpdateOAuth2TokenInSyncCredentialStore(const std::string& access_token)
{
    const std::shared_ptr<SyncCredentialStore> sync_credential_store = GetSyncCredentialStore();
    ASSERT(sync_credential_store != nullptr);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::accessToken, access_token)
                .Write(JK::refreshToken, m_refreshToken)
                .WriteIfHasValue(JK::user, m_user)
                .EndObject();

    sync_credential_store->Store(m_hostUrl, json_writer->GetString());
}


void CSWebConnection::HandleServerErrorResponse(const int error_message_number, const int http_response_code, const std::string& response_body) const
{
    std::string error;

    try
    {
        // try to parse the error from the HTTP response
        const JsonNode json_node = Json::Parse(response_body);
        const std::string code = json_node.Get<std::string>(JK::code);
        const std::string message = json_node.Get<std::string>(JK::message);

        if( http_response_code == HttpResponse::Status_404_NotFound &&
            code == "fatal_error" && SO::StartsWith(message, "No route found for") )
        {
            throw SyncNotSupportedByServiceVersionError();
        }

        error = FormatText("%s. (%d)", message.c_str(), http_response_code);
    }

    catch( const JsonParseException& )
    {
        SYNCLOG_ERROR << "Error server response: ";
        SYNCLOG_ERROR << response_body;

        // if the message can't be parsed, just use the HTTP error code;
        // don't show the body since it is probably HTML

        // the error is most likely an invalid server URL
        if( http_response_code == HttpResponse::Status_404_NotFound )
            throw SyncError(100144, m_hostUrl);

        error = FormatText("Error %d", http_response_code);
    }

    ASSERT(!error.empty());

    SyncError::ThrowByMessageNumberAndHttpResponseCode(error_message_number, http_response_code, error);
}


void CSWebConnection::HandleServerErrorResponse(const int error_message_number, const HttpResponse& response) const
{
    HandleServerErrorResponse(error_message_number, response.http_status, response.body.ToString());
}


void CSWebConnection::HandleServerErrorResponse(const JsonParseException& exception, const std::string& response_body) const
{
    SYNCLOG_ERROR << "Invalid server response: " << exception.what();
    SYNCLOG_ERROR << response_body;
    throw SyncError(100121);
}


void CSWebConnection::HandleServerErrorResponse(const JsonParseException& exception, const JsonNode& json_node) const
{
    HandleServerErrorResponse(exception, json_node.GetNodeAsString());
}


std::unique_ptr<ConnectResponse> CSWebConnection::RetrieveServerInfo(std::string username)
{
    const JsonNode json_node = ExecuteRestGet<JsonNode>("server", 100101);

    try
    {
        ConnectResponse connect_response = json_node.Get<ConnectResponse>();
        SYNCLOG_INFO << "Server API version: " << connect_response.GetApiVersion();

        connect_response.SetServerName(m_hostUrl);
        connect_response.SetUsername(std::move(username));

        return std::make_unique<ConnectResponse>(std::move(connect_response));
    }

    catch( const JsonParseException& exception )
    {
        HandleServerErrorResponse(exception, json_node);
    }
}


template<typename T>
T CSWebConnection::ExecuteRestProcessResponse(const HttpResponse& response, const int error_message_number,
                                              const std::function<void(int)>* const http_status_other_than_200_callback/* = nullptr*/)
{
    if( response.http_status != HttpResponse::Status_200_OK )
    {
        if( http_status_other_than_200_callback != nullptr )
            (*http_status_other_than_200_callback)(response.http_status);

        HandleServerErrorResponse(error_message_number, response);
    }

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

    else if constexpr(std::is_same_v<T, std::string>)
    {
        return response.body.ToString();
    }

    else
    {
        static_assert(std::is_same_v<T, void>);
    }
}


template<typename T>
T CSWebConnection::ExecuteRestGet(const std::string& path, const int error_message_number,
                                  const std::unique_ptr<HeaderList> additional_headers/* = nullptr*/,
                                  const std::function<void(int)>* const http_status_other_than_200_callback/* = nullptr*/)
{
    HeaderList headers = GetBaseHeaders();

    if( additional_headers != nullptr )
        headers.Append(std::move(*additional_headers));

    HttpRequestBuilder request_builder(m_hostUrl + path, std::move(headers));

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_401_Unauthorized && RefreshOAuth2Token() )
        response = m_httpConnection->Request(request);

    return ExecuteRestProcessResponse<T>(response, error_message_number, http_status_other_than_200_callback);
}


template<typename T>
T CSWebConnection::ExecuteRestPostJson(const std::string& path, const int error_message_number,
                                       std::string data, const bool compress_data,
                                       const std::unique_ptr<HeaderList> additional_headers/* = nullptr*/,
                                       const std::function<void(int)>* const http_status_other_than_200_callback/* = nullptr*/)
{
    HeaderList headers = GetBaseHeaders();
    headers.Add_ContentType_Json();

    if( compress_data )
    {
        headers.Add_ContentEncoding_Deflate();
        ZLib::Deflate(data);
    }

    const size_t data_length = data.length();
    headers.Add_ContentLength(data_length);

    if( additional_headers != nullptr )
        headers.Append(std::move(*additional_headers));

    std::istringstream data_stream(std::move(data));

    HttpRequestBuilder request_builder(m_hostUrl + path, std::move(headers));
    request_builder.post(data_stream, data_length);

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_401_Unauthorized && RefreshOAuth2Token() )
    {
        ASSERT(data_stream.tellg() == 0);
        response = m_httpConnection->Request(request);
    }

    return ExecuteRestProcessResponse<T>(response, error_message_number, http_status_other_than_200_callback);
}

// exported for use in zSyncO
template ZNETWORK_API JsonNode CSWebConnection::ExecuteRestPostJson(const std::string& path, int error_message_number,
                                                                    std::string data, bool compress_data,
                                                                    const std::unique_ptr<HeaderList> additional_headers = nullptr,
                                                                    const std::function<void(int)>* http_status_other_than_200_callback = nullptr);


template<typename T>
T CSWebConnection::ExecuteRestPutBinaryData(const std::string& path, int error_message_number,
                                            const BinaryBlock& data, const bool compress_data,
                                            const std::function<void(int)>* const http_status_other_than_200_callback/* = nullptr*/)
{
    HeaderList headers = GetBaseHeaders();
    headers.Add_ContentType_OctetStream()
           .Add_ContentMD5(Hash::Md5::Create(data));

    std::unique_ptr<const std::string> compressed_data;
    std::optional<MemoryStream> memory_stream;
    size_t content_length;

    if( compress_data )
    {
        headers.Add_ContentEncoding_Deflate();

        compressed_data = std::make_unique<std::string>(ZLib::Deflate(data));
        content_length = compressed_data->length();
        memory_stream.emplace(*compressed_data);
    }

    else
    {
        content_length = data.size();
        memory_stream.emplace(data);
    }

    headers.Add_ContentLength(content_length);

    HttpRequestBuilder request_builder(m_hostUrl + path, std::move(headers));
    request_builder.put(*memory_stream, content_length);

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_401_Unauthorized && RefreshOAuth2Token() )
    {
        ASSERT(memory_stream->tellg() == 0);
        response = m_httpConnection->Request(request);
    }

    return ExecuteRestProcessResponse<T>(response, error_message_number, http_status_other_than_200_callback);
}


void CSWebConnection::ExecuteRestDelete(const std::string& path, const int error_message_number)
{
    HttpRequestBuilder request_builder(m_hostUrl + path, GetBaseHeaders());
    request_builder.del();

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_401_Unauthorized && RefreshOAuth2Token() )
        response = m_httpConnection->Request(request);

    return ExecuteRestProcessResponse<void>(response, error_message_number);
}


JsonNode CSWebConnection::GetDictionariesList()
{
    return ExecuteRestGet<JsonNode>("dictionaries/", 100129);
}


std::string CSWebConnection::GetDictionarySpec(const std::string& dictionary_name)
{
    return ExecuteRestGet<std::string>("dictionaries/" + dictionary_name, 100130);
}


void CSWebConnection::PutDictionarySpec(std::string dictionary_spec)
{
    try
    {
        ExecuteRestPostJson<void>("dictionaries/", 100143,
                                  std::move(dictionary_spec), ApiSupportsCompressingFileUpload());
    }

    catch( const SyncError& exception )
    {
        if( exception.GetHttpResponseCode() == HttpResponse::Status_403_Forbidden )
        {
            SyncError::ThrowByMessageNumberAndHttpResponseCode(GetGenericErrorMessageNumber(), HttpResponse::Status_403_Forbidden,
                FormatText("You cannot add or update a dictionary on CSWeb due to insufficient privileges using the role '%s'.",
                           m_user.has_value() ? m_user->role_name.c_str() : "<unknown>"));
        }

        throw;
    }
}


void CSWebConnection::DeleteDictionarySpec(const std::string& dictionary_name)
{
    ExecuteRestDelete("dictionaries/" + dictionary_name, GetGenericErrorMessageNumber());
}


std::vector<FileInfo> CSWebConnection::GetDirectoryListing(const std::string& remote_path, const bool request_file_md5s)
{
    const std::string path = PortableFunctions::PathAppendForwardSlashToPath("folders", Encoders::ToUri(remote_path));
    ASSERT(path == PortableFunctions::PathToForwardSlash(path));

    auto additional_headers = std::make_unique<HeaderList>();
    additional_headers->AddJson(SyncCustomHeaders::GET_FILE_MD5_HEADER, Json::ToJson(request_file_md5s));

    const JsonNode json_node = ExecuteRestGet<JsonNode>(path, 100112, std::move(additional_headers));

    try
    {
        return json_node.GetArray().GetVector<FileInfo>();
    }

    catch( const JsonParseException& exception )
    {
        HandleServerErrorResponse(exception, json_node);
    }
}


bool CSWebConnection::GetFile(const std::string& remote_file_path, const std::string& local_file_path, cs::cref_optional<std::string> existing_file_md5)
{
    const std::string endpoint_path = PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathAppendForwardSlashToPath("files", Encoders::ToUri(remote_file_path)), "content");
    return GetFileUsingEndpoint(remote_file_path, local_file_path, std::move(existing_file_md5), endpoint_path, 100110);
}


template<typename CF>
bool CSWebConnection::GetFileUsingEndpoint(const std::string& remote_file_path, cs::cref_optional<std::string> existing_file_md5,
                                           const std::string& endpoint_path, const int error_message_number,
                                           const std::unique_ptr<HeaderList> additional_headers,
                                           const std::function<void(int)>* const http_status_other_than_200_callback,
                                           const CF& process_file_callback_function)
{
    HeaderList headers = GetBaseHeaders<false>();
    headers.Add_Accept_OctetStream();

    if( existing_file_md5.has_value() && !existing_file_md5->empty() )
        headers.Add_IfNoneMatch(*existing_file_md5);

    if( additional_headers != nullptr )
        headers.Append(std::move(*additional_headers));

    HttpRequestBuilder request_builder(m_hostUrl + endpoint_path, std::move(headers));

    const HttpRequest request = request_builder.build();
    HttpResponse response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_401_Unauthorized && RefreshOAuth2Token() )
        response = m_httpConnection->Request(request);

    if( response.http_status == HttpResponse::Status_304_NotModified )
    {
        ASSERT(existing_file_md5.has_value() && !existing_file_md5->empty());
        return false;
    }

    if( response.http_status != HttpResponse::Status_200_OK )
        ExecuteRestProcessResponse<void>(response, error_message_number, http_status_other_than_200_callback);

    try
    {
        const std::string server_md5 = response.headers.GetValue("Content-MD5");

        std::unique_ptr<std::string> client_md5 = !server_md5.empty() ? std::make_unique<std::string>() :
                                                                        nullptr;

        process_file_callback_function(response.body, client_md5.get());

        // check that the downloaded file matches the signature in response headers
        if( client_md5 != nullptr && !SO::EqualsNoCase(server_md5, *client_md5) )
            throw SyncError(100127, remote_file_path);

        return true;
    }

    catch( const std::exception& exception )
    {
        throw SyncError(error_message_number, exception);
    }
}


bool CSWebConnection::GetFileUsingEndpoint(const std::string& remote_file_path, const std::string& local_file_path, cs::cref_optional<std::string> existing_file_md5,
                                           const std::string& endpoint_path, const int error_message_number,
                                           std::unique_ptr<HeaderList> additional_headers/* = nullptr*/,
                                           const std::function<void(int)>* const http_status_other_than_200_callback/* = nullptr*/)
{
    try
    {
        return GetFileUsingEndpoint(remote_file_path, std::move(existing_file_md5), endpoint_path, error_message_number, std::move(additional_headers), http_status_other_than_200_callback,
            [&](ObservableResponseBody& body, std::string* const client_md5)
            {
                std::ofstream local_output_file_stream(local_file_path, std::ios::binary);
                local_output_file_stream << body;
                local_output_file_stream.close();

                if( client_md5 != nullptr )
                    *client_md5 = Hash::Md5::CreateFromFile(local_file_path);
            });
    }

    catch(...)
    {
        SYNCLOG_ERROR << "Failed to download file from " << remote_file_path << " to " << local_file_path;
        throw;
    }
}


void CSWebConnection::PutFile(const std::string& local_file_path, const std::string& remote_file_path)
{
    const std::string endpoint_path = PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathAppendForwardSlashToPath("files", Encoders::ToUri(remote_file_path)), "content");
    PutFileUsingEndpoint(local_file_path, endpoint_path, 100111);
}


void CSWebConnection::PutFileUsingEndpoint(const std::string& local_file_path, const std::string& endpoint_path, const int error_message_number)
{
    ASSERT(endpoint_path == PortableFunctions::PathToForwardSlash(endpoint_path));

    std::optional<BinaryBlock> content;

    try
    {
        content = FileIO::ReadBinary(local_file_path);
    }

    catch(...)
    {
        throw SyncError(100111, local_file_path);
    }

    SYNCLOG_INFO << "Uploading " << content->size() << " bytes";

#ifdef ANDROID
    // Due to limitations in Android httpconnection API we can only post
    // data up to 2GB. The Java method HttpUrlConnection.setFixedLengthStreamingMode
    // takes a Java int. In Android API 19 a method that takes a long was added but
    // in order to support earlier versions of Android we use the older version
    // of the method that takes an int which means we can only handle posts up to 2GB.
    if( content->size() > 0x7FFFFFFF )
        throw SyncError(100117, local_file_path);
#endif

    // don't compress data that is likely already compressed
    const bool compress_data = ( ApiSupportsCompressingFileUpload() &&
                                 !FileExtensions::IsFileCompressedData(local_file_path) );

    ExecuteRestPutBinaryData<void>(endpoint_path, error_message_number,
                                   *content, compress_data);
}


JsonNode CSWebConnection::GetApplicationsList()
{
    return ExecuteRestGet<JsonNode>("apps", 100141);
}


void CSWebConnection::DeleteApplication(const std::string& package_name)
{
    ExecuteRestDelete("apps/" + Encoders::ToUri(package_name), GetGenericErrorMessageNumber());
}


JsonNode CSWebConnection::GetDictionaryMetadata(const std::string& dictionary_name)
{
    try
    {
        return ExecuteRestGet<JsonNode>("dictionaries/" + dictionary_name + "/metadata", GetGenericErrorMessageNumber());
    }

    catch( const SyncError& exception )
    {
        if( exception.GetHttpResponseCode() == HttpResponse::Status_403_Forbidden )
        {
            SyncError::ThrowByMessageNumberAndHttpResponseCode(GetGenericErrorMessageNumber(), HttpResponse::Status_403_Forbidden,
                FormatText("The data source '%s' is not accessible due to insufficient privileges using the role '%s'.",
                           dictionary_name.c_str(),
                           m_user.has_value() ? m_user->role_name.c_str() : "<unknown>"));
        }

        throw;
    }
}


std::set<CSWebDictionaryPermission> CSWebConnection::ParseDictionaryPermissions(const JsonNode& dictionary_metadata_json_node) noexcept
{
    std::set<CSWebDictionaryPermission> permissions;

    try
    {
        const JsonNodeArray permissions_json_node_array = dictionary_metadata_json_node.Get(JK::permissions).GetArray();

        for( const JsonNode& permission_json_node : permissions_json_node_array )
        {
            const std::string_view permission_sv = permission_json_node.Get<std::string_view>();

            if( permission_sv == "data" )
            {
                ASSERT(permissions_json_node_array.size() == 1);
                permissions.emplace(CSWebDictionaryPermission::Read);
                permissions.emplace(CSWebDictionaryPermission::Write);
                permissions.emplace(CSWebDictionaryPermission::Clear);
            }

            else if( permission_sv == "data.read" )
            {
                permissions.emplace(CSWebDictionaryPermission::Read);
            }

            else if( permission_sv == "data.write" )
            {
                permissions.emplace(CSWebDictionaryPermission::Write);
            }

            else if( permission_sv == "data.clear" )
            {
                permissions.emplace(CSWebDictionaryPermission::Clear);
            }

            else
            {
                ASSERT(( permission_sv == "data.none" && permissions.empty() ) ||
                       ( permission_sv == "data.clear.dashboard" ));
            }
        }
    }
    catch(...) { ASSERT(false); }

    return permissions;
}


void CSWebConnection::DeleteDictionaryData(const std::string& dictionary_name)
{
    try
    {
        return ExecuteRestDelete("dictionaries/" + dictionary_name + "/data", GetGenericErrorMessageNumber());
    }

    catch( const SyncError& exception )
    {
        if( exception.GetHttpResponseCode() == HttpResponse::Status_403_Forbidden )
        {
            SyncError::ThrowByMessageNumberAndHttpResponseCode(GetGenericErrorMessageNumber(), HttpResponse::Status_403_Forbidden,
                FormatText("The cases in data source '%s' cannot be deleted due to insufficient privileges using the role '%s'.",
                           dictionary_name.c_str(),
                           m_user.has_value() ? m_user->role_name.c_str() : "<unknown>"));
        }

        throw;
    }
}


JsonNode CSWebConnection::QueryCasesRepository(const std::string& dictionary_name, const std::string_view arguments_json_text_sv,
                                               std::unique_ptr<std::string> cache_header_json_text/* = nullptr*/)
{
    auto additional_headers = std::make_unique<HeaderList>();
    additional_headers->AddJson(SyncCustomHeaders::CASES_REPOSITORY_OPTIONS_HEADER, arguments_json_text_sv);

    // the cache header must by compressed and added as Base64
    if( cache_header_json_text != nullptr )
    {
        AssertValidJson(*cache_header_json_text);
        additional_headers->AddAsDeflatedBase64(SyncCustomHeaders::CASES_REPOSITORY_CACHE_HEADER, std::move(*cache_header_json_text));
    }

    return ExecuteRestGet<JsonNode>("dictionaries/" + dictionary_name + "/cases", GetGenericErrorMessageNumber(), std::move(additional_headers));
}


void CSWebConnection::UploadCase(const std::string& dictionary_name, const std::string& device_id, std::string case_json_text)
{
    auto additional_headers = std::make_unique<HeaderList>();
    additional_headers->Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id);

    ExecuteRestPostJson<void>("dictionaries/" + dictionary_name + "/cases", 100178,
                              std::move(case_json_text), ApiSupportsCompressingCaseUpload(),
                              std::move(additional_headers));
}


std::string CSWebConnection::DownloadDictionaryBinaryData(const std::string& dictionary_name, const std::string& signature)
{
    const std::string endpoint_path = SO::Concatenate("dictionaries/", dictionary_name, "/binary-data/", signature);
    std::string data;

    GetFileUsingEndpoint(endpoint_path, std::nullopt, endpoint_path, 100110, nullptr, nullptr,
        [&](ObservableResponseBody& body, std::string* const client_md5)
        {
            data = body.ToString();

            if( client_md5 != nullptr )
                *client_md5 = Hash::Md5::Create(data);
        });

    return data; // BINARY_BLOCK_TODO CSWebConnection::DownloadDictionaryBinaryData could return a BinaryBlock
}


std::shared_ptr<SyncListener> CSWebConnection::GetSharedSyncListener()
{
    return m_httpConnection->GetSharedSyncListener();
}


void CSWebConnection::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    m_httpConnection->SetSyncListener(std::move(sync_listener));
}
