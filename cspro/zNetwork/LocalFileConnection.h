#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/FileBasedConnection.h>

class SyncConnectionString;


class ZNETWORK_API LocalFileConnection : public FileBasedConnection
{
public:
    LocalFileConnection(const SyncConnectionString& sync_connection_string);

    const std::string& GetRootDirectory() const { return m_rootDirectory; }

    void Connect();

    // FileBasedConnection overrides
    using FileBasedConnection::Download;
    void Download(const std::string& remote_file_path, const std::string& local_file_path) override;
    void Download(const std::string& remote_file_path, std::ostream& output_stream) override;

    void Upload(const std::string& local_file_path, const std::string& remote_file_path) override;
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
    std::string EvaluatePath(const std::string& remote_path) const;

private:
    std::string m_rootDirectory;
    bool m_createDirectoryIfNotExist;
};
