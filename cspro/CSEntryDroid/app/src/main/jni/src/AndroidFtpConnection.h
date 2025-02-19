#pragma once

#include <engine/StandardSystemIncludes.h>
#include <zNetwork/FtpConnection.h>
#include <jni.h>


// Ftp connection implementation for Android that calls through to ftp4j library

class AndroidFtpConnection : public FtpConnection
{
public:
    AndroidFtpConnection();
    ~AndroidFtpConnection();

    // FtpConnection + FileBasedConnection overrides

    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) override;

protected:
    std::string DoConnect(const std::string& username, const std::string& password) override;
    void DoDisconnect() override;

public:
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
    JNIEnv* m_env;
    jobject m_javaImpl;
};
