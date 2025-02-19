#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/FileBasedConnection.h>

class HeaderList;
class HttpConnection;
struct HttpResponse;
class LoginAccessor;
class OAuth2Authorizer;
class SettingsDb;


// Dropbox implementation of FileBasedConnection

class ZNETWORK_API DropboxConnection : public FileBasedConnection
{
public:
    DropboxConnection(std::shared_ptr<LoginAccessor> login_accessor, cs::cref_optional<SyncConnectionString> sync_connection_string);

    DropboxConnection(const DropboxConnection&) = delete;
    DropboxConnection(DropboxConnection&&);

    ~DropboxConnection();

    bool IsConnected() const { return !m_authorizationHeader.empty(); }

    // Returns the account's email address.
    std::string Connect()                      { return Connect(nullptr); }
    std::string Connect(std::string base_path) { return Connect(&base_path); }

    void Disconnect();

    // FileBasedConnection overrides
    void Download(const std::string& remote_file_path, const std::string& local_file_path) override;
    bool Download(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5) override;
    void Download(const std::string& remote_file_path, std::ostream& output_stream) override;

    using FileBasedConnection::Upload;
    void Upload(std::istream& input_stream, int64_t input_size_bytes, const std::string& remote_file_path) override;

    bool FileExists(const std::string& remote_path) override;
    bool FileIsRegular(const std::string& remote_path) override;
    bool FileIsDirectory(const std::string& remote_path) override;
    int64_t FileModifiedTime(const std::string& remote_path) override;
    std::vector<FileInfo> GetDirectoryListing(const std::string& remote_directory_path, bool request_file_md5s) override;

    void FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path) override;
    void FileDelete(const std::string& remote_file_path) override;
    void DirectoryDelete(const std::string& remote_directory_path) override;

    // Evaluates the sync connection string to determine if a local Dropbox directory should be used.
    // A valid local directory is returned, or a blank string if a remote connection should be used.
    // An exception is thrown if the settings require a local directory be used, and it does not exist.
    static std::string GetEvaluatedLocalDropboxDirectory(const SyncConnectionString& sync_connection_string);

private:
    // Throws an exception based on the erroneous reponse.
    [[noreturn]] void HandleServerErrorResponse(int message_number, int http_response_code, const std::string& response_body) const;
    [[noreturn]] void HandleServerErrorResponse(int message_number, const HttpResponse& response) const;
    [[noreturn]] void HandleServerErrorResponse(const JsonParseException& exception, const std::string& response_body) const;
    [[noreturn]] void HandleServerErrorResponse(const JsonParseException& exception, const JsonNode& json_node) const;

    // Routines to execute REST API requests.
    HeaderList GetBaseHeaders() const;

    template<typename T>
    T ExecuteRestProcessResponse(const HttpResponse& response, int error_message_number);

    template<typename T = JsonNode>
    T ExecuteRestPost(std::string endpoint, int error_message_number, HeaderList headers,
                      std::istream& input_stream, int64_t input_size_bytes);

    // Executes a POST to the RPC endpoint:
    //     "These endpoints accept arguments as JSON in the request body and return results as JSON in the response body."
    //     "RPC endpoints are on the api.dropboxapi.com domain."
    JsonNode ExecuteRPC(std::string_view path_sv, int error_message_number, std::string json_text);

    // Executes a POST to the Content-download endpoint:
    //     "As with content-upload endpoints, arguments are passed in the Dropbox-API-Arg request header or arg URL parameter."
    //     "The response body contains file content, so the result will appear as JSON in the Dropbox-API-Result response header."
    //     "These endpoints are also on the content.dropboxapi.com domain."
    HttpResponse ExecuteContentDownload(const std::string& remote_file_path, const std::string* etag);

    // Executes a POST to the Content-upload endpoint:
    //     "These endpoints accept file content in the request body, so their arguments are instead passed as JSON in the"
    //     "Dropbox-API-Arg request header or arg URL parameter. These endpoints are on the content.dropboxapi.com domain."
    template<typename T = JsonNode>
    T ExecuteContentUpload(std::string_view path_sv, int error_message_number,
                           std::string_view dropbox_api_arg_json_text_sv,
                           std::istream& input_stream, int64_t input_size_bytes);

    // Routines to hit various endpoints and other functionality.
    std::string Connect(std::string* base_path_override);

    // Makes a path valid for Dropbox, evaluating it from the base path, and returns it as a JSON string.
    std::string GetDropboxPathJsonString(std::string_view path_sv, bool specify_root_folder_as_empty_string) const;

    std::string GetAccountEmail();

    void ProcessNon304Download(const HttpResponse& response, std::ostream& output_stream);

    void UploadUsingSession(std::istream& input_stream, int64_t input_size_bytes, const std::string& remote_file_path);

    FileInfo GetFileMetadata(const std::string& remote_path);
    std::optional<FileInfo::FileType> GetFileMetadataType(const std::string& remote_path) noexcept;

    std::vector<FileInfo> GetFolderListing(const std::string& remote_path);

    void DeleteFileOrFolder(const std::string& remote_path);

    static std::string GetLocalDropboxInfoFilePath();
    static std::string GetLocalDropboxDirectory(const std::string* account_name);
    static std::string GetLocalDropboxDirectoryFromInfoFilePath(const std::string& local_dropbox_info_file_path, const std::string* account_name);

private:
    std::shared_ptr<LoginAccessor> m_loginAccessor;
    std::unique_ptr<std::string> m_loginEmail;
    std::shared_ptr<HttpConnection> m_httpConnection;
    std::unique_ptr<OAuth2Authorizer> m_oauth2Authorizer;
    std::string m_basePath;
    std::string m_authorizationHeader;
    std::unique_ptr<SettingsDb> m_etagsDb;
};
