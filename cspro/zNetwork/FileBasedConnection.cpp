#include "stdafx.h"
#include "FileBasedConnection.h"


void FileBasedConnection::Download(const std::string& remote_file_path, const std::string& local_file_path)
{
    std::ofstream local_output_file_stream(local_file_path, std::ios::binary);
    Download(remote_file_path, local_output_file_stream);
}


bool FileBasedConnection::Download(const std::string& remote_file_path, const std::string& local_file_path, const std::string& /*existing_file_md5*/)
{
    Download(remote_file_path, local_file_path);
    return true;
}


void FileBasedConnection::Upload(const std::string& local_file_path, const std::string& remote_file_path)
{
    const int64_t file_size = PortableFunctions::FileSize(local_file_path);

    if( file_size < 0 )
        throw SyncError(100111, FileIO::Exception::FileNotFound(local_file_path).what());

    std::ifstream local_input_file_stream(local_file_path, std::ios::binary);
    return Upload(local_input_file_stream, file_size, remote_file_path);
}
