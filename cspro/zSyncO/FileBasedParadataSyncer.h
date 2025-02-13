#pragma once

#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/TemporaryFile.h>
#include <zZipo/IZip.h>
#include <zSyncO/SyncException.h>


template<typename T>
class FileBasedParadataSyncer
{
public:
    FileBasedParadataSyncer(T& server_connection, const TCHAR* paradata_directory = _T("/CSPro/paradata/"))
        :   m_serverConnection(server_connection),
            m_paradataDirectory(paradata_directory),
            m_paradataLogPrefix(_T("pl-")) // pl = paradata log
    {
    }

    CString startParadataSync(const CString& log_uuid)
    {
        m_clientLogUuid = log_uuid;

        // pds = paradata details (server)
        CString server_log_uuid_filename = m_paradataDirectory + _T("pds.txt");

        // if a paradata sync has already occurred, read the server log UUID
        std::optional<CString> server_log_uuid = ReaderServerText(server_log_uuid_filename);

        // if this is the first sync, create and upload a server log UUID
        if( !server_log_uuid.has_value() )
        {
            server_log_uuid = WS2CS(CreateUuid());
            WriteServerText(server_log_uuid_filename, *server_log_uuid);
        }

        return *server_log_uuid;
    }

    void putParadata(const CString& filename)
    {
        CString compressed_log_filename;
        compressed_log_filename.Format(_T("%s%s%s-") Formatter_int64_t _T(".zip"), m_paradataDirectory.GetString(),
									   m_paradataLogPrefix.GetString(), m_clientLogUuid.GetString(), GetTimestamp<int64_t>());

        try
        {
            // compress the file before uploading
            TemporaryFile compressed_log_temporary_file;
            PortableFunctions::FileDelete(compressed_log_temporary_file.GetPath());

            std::unique_ptr<IZip> zipper(IZip::Create());

            zipper->AddFile(compressed_log_temporary_file.GetPath(), filename,
                GetParadataLogFilenameFromZipFilename(compressed_log_filename), true);

            // put the paradata
            m_serverConnection.putFile(WS2CS(compressed_log_temporary_file.GetPath()), compressed_log_filename);
        }

        catch( const CZipError& zip_error )
        {
            throw SyncError(100173, WS2CS(zip_error.GetErrorMessage()));
        }
    }

    std::vector<std::shared_ptr<TemporaryFile>> getParadata()
    {
        // see when the client last did a get
        time_t last_get_time = 0;

        // pdc = paradata details (client)
        m_clientStateFilename.Format(_T("%splc-%s.txt"), m_paradataDirectory.GetString(), m_clientLogUuid.GetString());

        std::optional<CString> read_time = ReaderServerText(m_clientStateFilename);

        if( read_time.has_value() )
            last_get_time = (time_t)CIMSAString(*read_time).Val();

        // determine all files that need to be downloaded
        std::unique_ptr<std::vector<FileInfo>> directory_listing(m_serverConnection.getDirectoryListing(m_paradataDirectory));
        std::vector<CString> compressed_logs_to_download;

        for( const auto& file_info : *directory_listing )
        {
            if( file_info.getType() == FileInfo::FileType::File && file_info.getLastModified() > last_get_time )
            {
                const auto& filename = file_info.getName();

                if( filename.Find(m_paradataLogPrefix) == 0 )
                {
                    const TCHAR* log_uuid_start = filename.GetString() + m_paradataLogPrefix.GetLength();

                    if( _tcsncmp(log_uuid_start, m_clientLogUuid, m_clientLogUuid.GetLength()) != 0 )
                    {
                        compressed_logs_to_download.emplace_back(file_info.getName());
                        m_highestGetFileTime = std::max(m_highestGetFileTime.value_or(0), file_info.getLastModified());
                    }
                }
            }
        }

        // download and decompress the logs
        std::vector<std::shared_ptr<TemporaryFile>> received_temporary_files;

        for( const auto& compressed_log_filename : compressed_logs_to_download )
        {
            TemporaryFile compressed_log_temporary_file;

            m_serverConnection.getFile(m_paradataDirectory + compressed_log_filename,
                WS2CS(compressed_log_temporary_file.GetPath()), WS2CS(compressed_log_temporary_file.GetPath()), CString());

            try
            {
                received_temporary_files.emplace_back(std::make_shared<TemporaryFile>());

                std::unique_ptr<IZip> zipper(IZip::Create());

                zipper->ExtractFile(compressed_log_temporary_file.GetPath(), GetParadataLogFilenameFromZipFilename(compressed_log_filename),
                                    received_temporary_files.back()->GetPath());
            }

            catch( const CZipError& zip_error )
            {
                throw SyncError(100173, WS2CS(zip_error.GetErrorMessage()));
            }
        }

        return received_temporary_files;
    }

    void stopParadataSync()
    {
        // if paradata was successfully received, update the get timestamp on the server
        if( m_highestGetFileTime.has_value() )
        {
            ASSERT(!m_clientStateFilename.IsEmpty());
            WriteServerText(m_clientStateFilename, IntToString((int64_t)*m_highestGetFileTime));
        }
    }

private:
    std::optional<CString> ReaderServerText(const CString& server_filename)
    {
        std::unique_ptr<TemporaryFile> temporary_file = m_serverConnection.getFileIfExists(server_filename);

        if( temporary_file != nullptr )
        {
            try
            {
                std::wstring contents = FileIO::ReadText(temporary_file->GetPath());
                SO::MakeTrim(contents);

                if( !contents.empty() )
                    return WS2CS(contents);
            }
            catch(...) { }
        }

        return std::nullopt;
    }

    void WriteServerText(const CString& server_filename, const CString& contents)
    {
        auto temporary_file = std::make_unique<TemporaryFile>();
        std::ofstream output_file_stream(temporary_file->GetPath().c_str(), std::ios::binary);

        output_file_stream << Utf8BOM_sv.data() << UTF8Convert::WideToUTF8(contents);

        output_file_stream.close();

        m_serverConnection.putFile(WS2CS(temporary_file->GetPath()), server_filename);
    }

    static CString GetParadataLogFilenameFromZipFilename(const CString& zip_filename)
    {
        return PortableFunctions::PathGetFilenameWithoutExtension<CString>(zip_filename) + FileExtensions::WithDot::Paradata;
    }

private:
    T& m_serverConnection;
    const CString m_paradataDirectory;
    const CString m_paradataLogPrefix;
    CString m_clientLogUuid;
    CString m_clientStateFilename;
    std::optional<time_t> m_highestGetFileTime;
};
