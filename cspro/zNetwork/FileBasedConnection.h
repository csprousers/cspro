#pragma once

#include <zNetwork/zNetwork.h>

class FileInfo;
class SyncListener;


class ZNETWORK_API FileBasedConnection
{
public:
    virtual ~FileBasedConnection() { }

    // Sets the sync listener.
    // Subclasses can override this method if they need to do any additional processing.
    virtual void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) { m_syncListener = std::move(sync_listener); }


    // Downloads from the remote file path to the local file path.
    // Callers should ensure that the directories exist for creating the local file.
    // The base class implementation uses a file stream and calls the stream-based Download.
    virtual void Download(const std::string& remote_file_path, const std::string& local_file_path);

    // Downloads from the remote file path to the local file path.
    // If set, the existing_file_md5 parameter gives the MD5 of the current file (even if that file
    // does not exist at local_file_path, since the download may be to a temporary location).
    // Subclasses should return false if the file did not need to be downloaded because the server file's matched the MD5.
    // Callers should ensure that the directories exist for creating the local file.
    // The base class implementation ignores the MD5 and calls the other file-based Download.
    virtual bool Download(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5);

    // Downloads from the remote file path to a output stream.
    virtual void Download(const std::string& remote_file_path, std::ostream& output_stream) = 0;


    // Uploads from the local file path to the remote file path.
    // The base class implementation uses a file stream and calls the stream-based Upload.
    virtual void Upload(const std::string& local_file_path, const std::string& remote_file_path);

    // Uploads from an input stream to the remote file path.
    virtual void Upload(std::istream& input_stream, int64_t input_size_bytes, const std::string& remote_file_path) = 0;


    // Returns true if the remote file exists (as a file or directory).
    virtual bool FileExists(const std::string& remote_path) = 0;

    // Returns true if the remote file exists and is not a directory.
    virtual bool FileIsRegular(const std::string& remote_path) = 0;

    // Returns true if the remote file exists and is a directory.
    // The path may be specified with or without a trailing slash.
    virtual bool FileIsDirectory(const std::string& remote_path) = 0;

    // Returns the last modified time for the remote file.
    virtual int64_t FileModifiedTime(const std::string& remote_path) = 0;

    // Returns the listing of files in a directory.
    // Implementations can decide to ignore the request_file_md5s flag.
    virtual std::vector<FileInfo> GetDirectoryListing(const std::string& remote_directory_path, bool request_file_md5s) = 0;


    // Renames a remote file.
    // Implementions can decide whether to rename a directory at this path or to throw an exception.
    virtual void FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path) = 0;

    // Deletes a remote file.
    // Implementions can decide whether to delete a directory at this path or to throw an exception.
    virtual void FileDelete(const std::string& remote_file_path) = 0;

    // Deletes a remote directory along with all of its files.
    // Implementions can decide whether to delete a file at this path or to throw an exception.
    virtual void DirectoryDelete(const std::string& remote_directory_path) = 0;


protected:
    std::shared_ptr<SyncListener> m_syncListener;
};
