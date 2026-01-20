#include "stdafx.h"
#include "FileBasedParadataSyncer.h"
#include "ISyncService.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ImsaStr.h>


namespace
{
    constexpr const char* ParadataLogPrefix                  = "pl-";     // pl  = paradata log
    constexpr const char* ParadataSyncServiceDetailsFilename = "pds.txt"; // pds = paradata details (sync service)
    constexpr const char* ParadataClientDetailsPrefix        = "plc-";    // plc = paradata details (client)
}


FileBasedParadataSyncer::FileBasedParadataSyncer(ISyncService& sync_service, std::string paradata_directory/* = "/CSPro/paradata/"*/)
    :   m_syncService(sync_service),
        m_paradataDirectory(std::move(paradata_directory))
{
}


std::string FileBasedParadataSyncer::StartParadataSync(const std::string& log_uuid)
{
    m_clientLogUuid = log_uuid;

    const std::string sync_service_log_uuid_file_path = Path::Combine(m_paradataDirectory, ParadataSyncServiceDetailsFilename);

    // if a paradata sync has already occurred, read the sync service log UUID
    std::optional<std::string> sync_service_log_uuid = ReadTextOnSyncService(sync_service_log_uuid_file_path);

    // if this is the first sync, create and upload a sync service log UUID
    if( !sync_service_log_uuid.has_value() )
    {
        sync_service_log_uuid = CreateUuid();
        WriteTextOnSyncService(sync_service_log_uuid_file_path, *sync_service_log_uuid);
    }

    return std::move(*sync_service_log_uuid);
}


void FileBasedParadataSyncer::PutParadata(const std::string& paradata_log_file_path)
{
    const std::string compressed_log_filename = FormatText("%s%s-" Formatter_int64_t ".zip", ParadataLogPrefix, m_clientLogUuid.c_str(), GetTimestamp());
    const std::string compressed_log_file_path = Path::Combine(m_paradataDirectory, compressed_log_filename);

    try
    {
        // compress the file before uploading
        const TemporaryFile compressed_log_temporary_file;

        ZipCreator zip_creator(compressed_log_temporary_file.GetPath());
        zip_creator.AddFiles({ paradata_log_file_path }, { GetParadataLogFilenameFromZipFilename(compressed_log_filename) });
        zip_creator.Close();

        // put the paradata
        m_syncService.PutFile(compressed_log_temporary_file.GetPath(), compressed_log_file_path);
    }

    catch( const ZipException& exception )
    {
        throw SyncError(100173, exception);
    }
}


std::vector<TemporaryFile> FileBasedParadataSyncer::GetParadata()
{
    // see when the client last did a get
    int64_t last_get_time = 0;

    const std::string client_state_filename = SO::Concatenate(ParadataClientDetailsPrefix, m_clientLogUuid, ".txt");
    m_clientStateFilePath = Path::Combine(m_paradataDirectory, client_state_filename);

    const std::optional<std::string> read_time = ReadTextOnSyncService(m_clientStateFilePath);

    if( read_time.has_value() )
        last_get_time = CIMSAString::Val(*read_time);

    // determine all files that need to be downloaded
    const std::vector<FileInfo> directory_listing = m_syncService.GetDirectoryListing(m_paradataDirectory, false);
    std::vector<std::string> compressed_logs_to_download;

    for( const FileInfo& file_info : directory_listing )
    {
        if( file_info.GetType() == FileInfo::FileType::File && file_info.GetLastModified() > last_get_time )
        {
            const std::string& filename = file_info.GetName();
            constexpr std::string_view ParadataLogPrefix_sv(ParadataLogPrefix);

            if( SO::StartsWith(filename, ParadataLogPrefix_sv) )
            {
                const char* const log_uuid_start = filename.c_str() + ParadataLogPrefix_sv.length();

                if( !SO::StartsWith(log_uuid_start, m_clientLogUuid) )
                {
                    compressed_logs_to_download.emplace_back(filename);
                    m_highestGetFileTime = std::max(m_highestGetFileTime.value_or(0), file_info.GetLastModified());
                }
            }
        }
    }

    // download and decompress the logs
    std::vector<TemporaryFile> received_temporary_files;

    for( const std::string& compressed_log_filename : compressed_logs_to_download )
    {
        const TemporaryFile compressed_log_temporary_file;

        m_syncService.GetFile(Path::Combine(m_paradataDirectory, compressed_log_filename),
                             compressed_log_temporary_file.GetPath(), std::string());

        try
        {
            ZipReader zip_reader(compressed_log_temporary_file.GetPath());
            zip_reader.Extract(GetParadataLogFilenameFromZipFilename(compressed_log_filename), received_temporary_files.emplace_back().GetPath());
        }

        catch( const ZipException& exception )
        {
            throw SyncError(100173, exception);
        }
    }

    return received_temporary_files;
}


void FileBasedParadataSyncer::StopParadataSync()
{
    // if paradata was successfully received, update the get timestamp on the sync service
    if( m_highestGetFileTime.has_value() )
    {
        ASSERT(!m_clientStateFilePath.empty());
        WriteTextOnSyncService(m_clientStateFilePath, IntToString(*m_highestGetFileTime));
    }
}


std::optional<std::string> FileBasedParadataSyncer::ReadTextOnSyncService(const std::string& remote_file_path)
{
    const std::unique_ptr<TemporaryFile> temporary_file = m_syncService.GetFileIfExists(remote_file_path);

    if( temporary_file != nullptr )
    {
        try
        {
            std::string text = FileIO::ReadText(temporary_file->GetPath());
            SO::MakeTrim(text);

            if( !text.empty() )
                return text;
        }
        catch(...) { }
    }

    return std::nullopt;
}


void FileBasedParadataSyncer::WriteTextOnSyncService(const std::string& path,const std::string_view text_sv)
{
    const TemporaryFile temporary_file;
    FileIO::WriteText(temporary_file.GetPath(), text_sv, false);

    m_syncService.PutFile(temporary_file.GetPath(), path);
}


std::string FileBasedParadataSyncer::GetParadataLogFilenameFromZipFilename(const std::string& zip_filename)
{
    ASSERT(PortableFunctions::PathGetDirectory(zip_filename).empty());

    return Path::ReplaceExtension(zip_filename, FileExtensions::Paradata);
}
