#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/FtpConnection.h>
#include <zToolsO/DateTime.h>

class CurlWrapper;


// FTP client implementation using libcurl

class ZNETWORK_API CurlFtpConnection : public FtpConnection
{
public:
    CurlFtpConnection();
    ~CurlFtpConnection();

    // FtpConnection + FileBasedConnection overrides

    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) override;

protected:
    std::string DoConnect(const std::string& username, const std::string& password) override;
    void DoDisconnect() override;

public:
    using FileBasedConnection::Download;
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

private:
    std::string CreateRemoteUrl(const std::string& path) const;

    void QueryFeatures();

    // queries a path for existence (bool) or modified time (int64_t);
    // exceptions are only thrown when checking for modified time
    template<typename T>
    T QueryPath(const std::string& remote_path);

    static bool IncludeInDirectoryListing(std::string_view filename_sv);

    struct DirectoryListingInfo;
    void GetDirectoryListingMLSD(DirectoryListingInfo& directory_listing_info);
    void GetDirectoryListingLIST(DirectoryListingInfo& directory_listing_info);

    static long DirectoryListingWildcardMatchCallback(const void* transfer_info, DirectoryListingInfo* directory_listing_info, int remains);

    // returns -1 if there is not a valid numeric token; text_sv is adjusted when there is
    static int GetNextNumericToken(std::string_view& text_sv);

    // validates the date/time
    static std::optional<int64_t> ValidateFileTime(const DateTime::Components& date_time_components);
    static std::optional<int64_t> ParseUnixFileTime(std::string_view time_text_sv);
    static std::optional<int64_t> ParseDosFileTime(std::string_view time_text_sv);

    void DeleteWorker(const char* command, const std::string& remote_path);

private:
    bool m_supportsMLSD;
    void* m_curl;
    std::unique_ptr<CurlWrapper> m_curlWrapper;
};
