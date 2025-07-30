#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/CSWebUser.h>
#include <zNetwork/LoginCredentials.h>
#include <zUtilO/SyncConnectionString.h>

class ConnectResponse;
class FileInfo;
class HeaderList;
class HttpConnection;
struct HttpResponse;
class JsonParseException;
class SyncListener;


namespace CSWebVersion
{
    constexpr double V1 = 1.0;
    constexpr double V2 = 2.0;
    constexpr double V3 = 3.0;
}


class ZNETWORK_API CSWebConnection
{
    friend class CSWebSyncService;

public:
    CSWebConnection(std::unique_ptr<HttpConnection> http_connection, SyncConnectionString sync_connection_string, LoginCredentials login_credentials);
    virtual ~CSWebConnection();

    const std::string& GetHostUrl() const { return m_hostUrl; }

    double GetApiVersion() const { return m_apiVersion; }

    const std::optional<CSWebUser>& GetUser() const { return m_user; }

    // Connects to the server and returns connection information, throwing an exception on error.
    std::unique_ptr<ConnectResponse> Connect(double minimum_api_version_required);

    // --------------------------------------------------------------------------
    // dictionary and case functionality
    // --------------------------------------------------------------------------

    // Gets a list of dictionaries on the server.
    JsonNode GetDictionariesList();

    // Gets / puts / deletes a dictionary specification.
    std::string GetDictionarySpec(const std::string& dictionary_name);
    void PutDictionarySpec(std::string dictionary_spec);
    void DeleteDictionarySpec(const std::string& dictionary_name);

    // --------------------------------------------------------------------------
    // file functionality
    // --------------------------------------------------------------------------

    // Gets a directory listing.
    std::vector<FileInfo> GetDirectoryListing(const std::string& remote_path, bool request_file_md5s);

    // Gets a file using the /files endpoint.
    bool GetFile(const std::string& remote_file_path, const std::string& local_file_path, cs::cref_optional<std::string> existing_file_md5);

    // Gets a file using the specified endpoint.
    // If passing an existing file's MD5, the method returns false if the file was not downloaded because the MD5 matched.
    bool GetFileUsingEndpoint(const std::string& remote_file_path, const std::string& local_file_path, cs::cref_optional<std::string> existing_file_md5,
                              const std::string& endpoint_path, int error_message_number,
                              std::unique_ptr<HeaderList> additional_headers = nullptr,
                              const std::function<void(int)>* http_status_other_than_200_callback = nullptr);

    // Puts a file using the /files endpoint.
    void PutFile(const std::string& local_file_path, const std::string& remote_file_path);

    // Puts a file using the specified endpoint.
    void PutFileUsingEndpoint(const std::string& local_file_path, const std::string& endpoint_path, int error_message_number);

    // --------------------------------------------------------------------------
    // application functionality
    // --------------------------------------------------------------------------

    // Gets a list of applications on the server.
    JsonNode GetApplicationsList();

    // Deletes an application.
    void DeleteApplication(const std::string& package_name);

    // --------------------------------------------------------------------------
    // CSWebRepository functionality
    // --------------------------------------------------------------------------

    // Gets the metadata associated with a dictionary.
    JsonNode GetDictionaryMetadata(const std::string& dictionary_name);

    // Delete all cases associated with a dictionary.
    void DeleteDictionaryData(const std::string& dictionary_name);

    // Executes a query using the /cases endpoint.
    JsonNode QueryCasesRepository(const std::string& dictionary_name, std::string_view arguments_json_text_sv,
                                  std::unique_ptr<std::string> cache_json_text = nullptr);

    // Uploads one of more cases (specified as a JSON array) using the /cases endpoint.
    void UploadCase(const std::string& dictionary_name, const std::string& device_id, std::string case_json_text);

    // Downloads the binary data associated with the dictionary.
    std::string DownloadDictionaryBinaryData(const std::string& dictionary_name, const std::string& signature);

    // --------------------------------------------------------------------------
    // other functionality
    // --------------------------------------------------------------------------
    std::shared_ptr<SyncListener> GetSharedSyncListener();
    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener);

protected:
    // Returns the access token, refresh token, and user details, throwing an exception on error.
    std::tuple<std::string, std::string, std::optional<CSWebUser>> RetrieveOAuth2Token(const UsernamePassword& username_password);

    // Returns true if the refresh token was sucessfully refreshed.
    bool RefreshOAuth2Token() noexcept;

    // API version checks
    bool ApiRequiresCPro74CaseFormat() const      { return ( m_apiVersion < CSWebVersion::V2 ); }
    bool ApiSupportsCompressingCaseUpload() const { return ( m_apiVersion >= CSWebVersion::V2 ); }
    bool ApiSupportsCompressingFileUpload() const { return ( m_apiVersion >= CSWebVersion::V3 ); }

    // Throws an exception based on the erroneous reponse.
    [[noreturn]] void HandleServerErrorResponse(int error_message_number, int http_response_code, const std::string& response_body) const;
    [[noreturn]] void HandleServerErrorResponse(int error_message_number, const HttpResponse& response) const;
    [[noreturn]] void HandleServerErrorResponse(const JsonParseException& exception, const std::string& response_body) const;
    [[noreturn]] void HandleServerErrorResponse(const JsonParseException& exception, const JsonNode& json_node) const;

private:
    void CreateAuthorizationHeader(const std::string& access_token);

    template<bool ResponseAcceptsJson = true>
    HeaderList GetBaseHeaders() const;

    std::shared_ptr<SyncCredentialStore> GetSyncCredentialStore();

    // Returns the access token, refresh token, and user details from the credential store, returning blank strings if not available.
    std::tuple<std::string, std::string, std::optional<CSWebUser>> GetOAuth2TokenFromSyncCredentialStore();

    // Updates the credential store with the access token and m_refreshToken.
    void UpdateOAuth2TokenInSyncCredentialStore(const std::string& access_token);

    // Returns the server info throwing an exception on error.
    std::unique_ptr<ConnectResponse> RetrieveServerInfo(std::string username);

    // Gets a file using the specified endpoint.
    // The callback function should set the downloaded MD5 (when client_md5 is non-null).
    template<typename CF>
    bool GetFileUsingEndpoint(const std::string& remote_file_path, cs::cref_optional<std::string> existing_file_md5,
                              const std::string& endpoint_path, int error_message_number,
                              std::unique_ptr<HeaderList> additional_headers,
                              const std::function<void(int)>* http_status_other_than_200_callback,
                              const CF& process_file_callback_function);

protected:
    // Routines to execute REST API requests.
    // If a 401: Unauthorized error is received, the request attempts to refresh the OAuth 2.0 token and then
    // submits the request again.
    template<typename T>
    T ExecuteRestProcessResponse(const HttpResponse& response, int error_message_number,
                                 const std::function<void(int)>* http_status_other_than_200_callback = nullptr);

    template<typename T>
    T ExecuteRestGet(const std::string& path, int error_message_number,
                     std::unique_ptr<HeaderList> additional_headers = nullptr,
                     const std::function<void(int)>* http_status_other_than_200_callback = nullptr);

    template<typename T>
    T ExecuteRestPostJson(const std::string& path, int error_message_number,
                          std::string data, bool compress_data,
                          std::unique_ptr<HeaderList> additional_headers = nullptr,
                          const std::function<void(int)>* http_status_other_than_200_callback = nullptr);

    template<typename T>
    T ExecuteRestPutBinaryData(const std::string& path, int error_message_number,
                               const BinaryBlock& data, bool compress_data,
                               const std::function<void(int)>* http_status_other_than_200_callback = nullptr);

    void ExecuteRestDelete(const std::string& path, int error_message_number);

private:
    std::unique_ptr<HttpConnection> m_httpConnection;

    SyncConnectionString m_syncConnectionString;
    std::string m_hostUrl;
    LoginCredentials m_loginCredentials;

protected:
    double m_apiVersion;

private:
    std::string m_authorizationHeader;
    std::string m_refreshToken;
    std::optional<CSWebUser> m_user;
};
