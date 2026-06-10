#include "stdafx.h"
#include "LocalFileConnection.h"
#include "StreamCopier.h"
#include <zToolsO/DirectoryLister.h>


LocalFileConnection::LocalFileConnection(const SyncConnectionString& sync_connection_string)
    :   m_rootDirectory(sync_connection_string.GetEvaluatedDirectoryPath()),
        m_createDirectoryIfNotExist(sync_connection_string.HasProperty(SCSProperty::createDirectory, SCSValue::true_, true))
{
}


void LocalFileConnection::Connect()
{
    if( !PortableFunctions::FileIsDirectory(m_rootDirectory) )
    {
        if( !m_createDirectoryIfNotExist )
        {
            SYNCLOG_ERROR << "Local file directory " << m_rootDirectory << " not found on this device";
            throw SyncError(100147, m_rootDirectory);
        }

        try
        {
            FileIO::CreateDirectories(m_rootDirectory);
        }

        catch( const CSProException& exception )
        {
            SYNCLOG_ERROR << "Local file directory " << m_rootDirectory << " could not be created";
            throw SyncConnectionError(exception.what());
        }
    }
}


std::string LocalFileConnection::EvaluatePath(const std::string& remote_path) const
{
    return Path::ToNativeSlash(Path::Combine(m_rootDirectory, remote_path));
}


void LocalFileConnection::Download(const std::string& remote_file_path, const std::string& local_file_path)
{
    const std::string full_remote_file_path = EvaluatePath(remote_file_path);

    if( !PortableFunctions::FileCopy(full_remote_file_path, local_file_path, false) )
    {
        SYNCLOG_ERROR << "Failed to download file " << full_remote_file_path << " to " << local_file_path;
        throw SyncError(100110, remote_file_path);
    }
}


void LocalFileConnection::Download(const std::string& remote_file_path, std::ostream& output_stream)
{
    const std::string full_remote_file_path = EvaluatePath(remote_file_path);
    const int64_t file_size = PortableFunctions::FileSize(full_remote_file_path);

    std::ifstream input_stream(full_remote_file_path, std::ios::binary);

    if( input_stream.fail() )
        throw SyncError(100111, full_remote_file_path);

    StreamCopier::Copy(m_syncListener.get(), input_stream, output_stream, file_size);

    input_stream.close();
}


void LocalFileConnection::Upload(const std::string& local_file_path, const std::string& remote_file_path)
{
    const std::string full_remote_file_path = EvaluatePath(remote_file_path);
    PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(full_remote_file_path));

    if( !PortableFunctions::FileCopy(local_file_path, full_remote_file_path, false) )
    {
        SYNCLOG_ERROR << "Failed to upload file " << local_file_path << " to " << full_remote_file_path;
        throw SyncError(100111, local_file_path);
    }
}


void LocalFileConnection::Upload(std::istream& input_stream, const int64_t input_size_bytes, const std::string& remote_file_path)
{
    const std::string full_remote_file_path = EvaluatePath(remote_file_path);
    PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(full_remote_file_path));

    std::ofstream output_stream(full_remote_file_path, std::ios::binary);

    if( output_stream.fail() )
        throw SyncError(100111, full_remote_file_path);

    StreamCopier::Copy(m_syncListener.get(), input_stream, output_stream, input_size_bytes);

    output_stream.close();
}


bool LocalFileConnection::FileExists(const std::string& remote_path)
{
    return PortableFunctions::FileExists(EvaluatePath(remote_path));
}


bool LocalFileConnection::FileIsRegular(const std::string& remote_path)
{
    return PortableFunctions::FileIsRegular(EvaluatePath(remote_path));
}


bool LocalFileConnection::FileIsDirectory(const std::string& remote_path)
{
    return PortableFunctions::FileIsDirectory(EvaluatePath(remote_path));
}


int64_t LocalFileConnection::FileModifiedTime(const std::string& remote_path)
{
    try
    {
        return PortableFunctions::FileModifiedTime<true>(EvaluatePath(remote_path));
    }

    catch( const std::exception& exception )
    {
        throw SyncError(100153, exception);
    }
}


std::vector<FileInfo> LocalFileConnection::GetDirectoryListing(const std::string& remote_directory_path, const bool request_file_md5s)
{
    std::vector<FileInfo> directory_listing;

    const std::string full_remote_directory_path = EvaluatePath(remote_directory_path);

    // store the relative path that when concatenated with the root path will construct the full path
    std::string path_from_root = PortableFunctions::PathToForwardSlash(full_remote_directory_path);

    if( !SO::StartsWith(path_from_root, "./") && !SO::StartsWith(path_from_root, "../") )
        path_from_root = PortableFunctions::PathAppendForwardSlashToPath("/", path_from_root);

    for( const std::string& full_path : DirectoryLister().SetIncludeDirectories()
                                                         .GetPaths(full_remote_directory_path) )
    {
        std::string filename = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetFilename(full_path));

        const FileInfo::FileType type = PortableFunctions::FileIsDirectory(full_path) ? FileInfo::FileType::Directory :
                                                                                        FileInfo::FileType::File;

        if( type == FileInfo::FileType::File )
        {
            directory_listing.emplace_back(type, std::move(filename), path_from_root,
                                           PortableFunctions::FileSize(full_path),
                                           PortableFunctions::FileModifiedTime(full_path),
                                           request_file_md5s ? Hash::Md5::CreateFromFile(full_path) : std::string());
        }

        else
        {
            directory_listing.emplace_back(type, std::move(filename), path_from_root);
        }
    }

    return directory_listing;
}


void LocalFileConnection::FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path)
{
    const std::string full_old_remote_file_path = EvaluatePath(old_remote_file_path);
    const std::string full_new_remote_file_path = EvaluatePath(new_remote_file_path);
    PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(full_new_remote_file_path));

    try
    {
        PortableFunctions::FileRenameWithExceptions(full_old_remote_file_path, full_new_remote_file_path);
    }

    catch( const std::exception& exception )
    {
        throw SyncError(100177, exception);
    }
}


void LocalFileConnection::FileDelete(const std::string& remote_file_path)
{
    const std::string full_remote_file_path = EvaluatePath(remote_file_path);

    if( !PortableFunctions::FileDelete(full_remote_file_path) )
        throw SyncError(100175, full_remote_file_path);
}


void LocalFileConnection::DirectoryDelete(const std::string& remote_directory_path)
{
    const std::string full_remote_directory_path = EvaluatePath(remote_directory_path);

    if( !PortableFunctions::DirectoryDelete(full_remote_directory_path, true) )
        throw SyncError(100175, full_remote_directory_path);
}
