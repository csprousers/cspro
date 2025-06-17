#include "stdafx.h"
#include "ZipFile.h"
#include <zToolsO/CallbackFunctionProcessor.h>


// --------------------------------------------------------------------------
// ZipImpl
// --------------------------------------------------------------------------

class ZipImpl
{
public:
    ZipImpl();

    mz_zip_archive* GetZipArchive() { return m_zipArchive.get(); }

    // Because the paths in ZIP files can be created using forward or backward slashes,
    // this function we will check against both. The function can return:
    // - bool
    // - int / std::unique_ptr<mz_zip_archive_file_stat> (with an exception thrown if not found).
    template<typename T>
    T LocateFile(const std::string& path_in_zip);

    // Calls FileIO::CreateDirectories/CreateDirectoriesForFile and rethrows exceptions as a ZipException.
    static void CreateDirectories(const std::string& directory_or_file_path, bool is_directory);

    // Returns the last miniz error.
    const char* GetLastError() const;

private:
    std::unique_ptr<mz_zip_archive> m_zipArchive;
};


ZipImpl::ZipImpl()
    :   m_zipArchive(std::make_unique<mz_zip_archive>(mz_zip_archive{ 0 }))
{
}


template<typename T>
T ZipImpl::LocateFile(const std::string& path_in_zip)
{
    int file_index;

    auto locate_file = [&](const cs::string_sz this_path_in_zip)
    {
        file_index = mz_zip_reader_locate_file(m_zipArchive.get(), this_path_in_zip.c_str(), nullptr, 0);
        return ( file_index >= 0 );
    };

    ( locate_file(path_in_zip) ) ||
    ( path_in_zip.find('\\') != std::string::npos && locate_file(PortableFunctions::PathToForwardSlash(path_in_zip)) ) ||
    ( path_in_zip.find('/') != std::string::npos && locate_file(PortableFunctions::PathToBackwardSlash(path_in_zip)) );

    if constexpr(std::is_same_v<T, bool>)
    {
        return ( file_index >= 0 );
    }

    else
    {
        if( file_index >= 0 )
        {
            if constexpr(std::is_same_v<T, int>)
            {
                return file_index;
            }

            else
            {
                auto file_stat = std::make_unique<mz_zip_archive_file_stat>();

                if( mz_zip_reader_file_stat(m_zipArchive.get(), file_index, file_stat.get()) )
                    return file_stat;
            }
        }

        throw ZipException::NoEntryInZip(path_in_zip);
    }
}


void ZipImpl::CreateDirectories(const std::string& directory_or_file_path, const bool is_directory)
{
    try
    {
        is_directory ? FileIO::CreateDirectories(directory_or_file_path) :
                       FileIO::CreateDirectoriesForFile(directory_or_file_path);
    }

    catch( const CSProException& exception )
    {
        throw ZipException::FileIOError(exception);
    }
}


const char* ZipImpl::GetLastError() const
{
    return mz_zip_get_error_string(mz_zip_get_last_error(m_zipArchive.get()));
}



// --------------------------------------------------------------------------
// ZipReader
// --------------------------------------------------------------------------

bool ZipReader::IsZipHeader(const cs::string_sz data)
{
    return ( data[0] == 'P' &&
             data[1] == 'K' &&
             data[2] == '\03' &&
             data[3] == '\04' );
}


ZipReader::ZipReader(const std::string& file_path)
    :   m_impl(std::make_unique<ZipImpl>())
{
    if( !mz_zip_reader_init_file(m_impl->GetZipArchive(), file_path.c_str(), 0) )
        throw ZipException::NotValidZipFile(file_path);
}


ZipReader::ZipReader(const char* const archive_buffer, const size_t archive_length, const char* const file_path_for_error_message/* = nullptr*/)
    :   m_impl(std::make_unique<ZipImpl>())
{
    if( !mz_zip_reader_init_mem(m_impl->GetZipArchive(), archive_buffer, archive_length, 0) )
        throw ZipException::NotValidZipFile(( file_path_for_error_message != nullptr ) ? file_path_for_error_message : "<memory>");
}


ZipReader::ZipReader(ZipReader&&) = default;


ZipReader::~ZipReader()
{
    // ignore any errors closing the file
    if( m_impl != nullptr )
        mz_zip_reader_end(m_impl->GetZipArchive());
}


std::unique_ptr<ZipImpl> ZipReader::ReleaseZipImpl()
{
    return std::move(m_impl);
}


template<typename FileStat>
BinaryBlock ZipReader::Read(const FileStat& file_stat, const cs::string_sz path_in_zip)
{
    BinaryBlock binary_block(static_cast<size_t>(file_stat.m_uncomp_size));

    if( !mz_zip_reader_extract_to_mem(m_impl->GetZipArchive(), file_stat.m_file_index, binary_block.data(), binary_block.size(), 0) )
        throw ZipException::ReadError(path_in_zip);

    return binary_block;
}


BinaryBlock ZipReader::Read(const std::string& path_in_zip)
{
    const std::unique_ptr<mz_zip_archive_file_stat> file_stat = m_impl->LocateFile<std::unique_ptr<mz_zip_archive_file_stat>>(path_in_zip);

    return Read(*file_stat, path_in_zip);
}


void ZipReader::Extract(const std::string& path_in_zip, const std::string& output_path)
{
    const std::unique_ptr<mz_zip_archive_file_stat> file_stat = m_impl->LocateFile<std::unique_ptr<mz_zip_archive_file_stat>>(path_in_zip);

    ZipImpl::CreateDirectories(output_path, false);

    if( !mz_zip_reader_extract_to_file(m_impl->GetZipArchive(), file_stat->m_file_index, output_path.c_str(), 0) )
        throw ZipException::ExtractError(path_in_zip, m_impl->GetLastError());
}


int ZipReader::ExtractAll(const std::string& output_directory)
{
    ZipImpl::CreateDirectories(output_directory, true);

    const int num_files = mz_zip_reader_get_num_files(m_impl->GetZipArchive());
    int files_extracted = 0;

    for( int i = 0; i < num_files; ++i )
    {
        mz_zip_archive_file_stat file_stat;

        if( !mz_zip_reader_file_stat(m_impl->GetZipArchive(), i, &file_stat) )
            throw ZipException::ExtractError(FormatText("<File %d>", i + 1), m_impl->GetLastError());

        const std::string output_path = MakeFullPath(output_directory, file_stat.m_filename);

        // Check for Zip Slip vulnerability where relative path in ZIP is outside output directory
        if( !SO::StartsWithNoCase(output_path, output_directory) )
            throw ZipException::ExtractZipSlipError(file_stat.m_filename, output_path);

        if( mz_zip_reader_is_file_a_directory(m_impl->GetZipArchive(), i) )
        {
            ZipImpl::CreateDirectories(output_path, true);
        }

        else
        {
            // Handle archives without directories - seems like 7-Zip and Windows ZIP always generate
            // directories but it is possible to write an archive with just the files so ensure
            // that the directory exists.
            const std::string this_output_directory = PortableFunctions::PathGetDirectory(output_path);

            if( !this_output_directory.empty() )
                ZipImpl::CreateDirectories(this_output_directory, true);

            if( !mz_zip_reader_extract_to_file(m_impl->GetZipArchive(), i, output_path.c_str(), 0) )
                throw ZipException::ExtractError(file_stat.m_filename, m_impl->GetLastError());

            ++files_extracted;
        }
    }

    return files_extracted;
}


template<typename CF>
void ZipReader::ForeachFileWorker(const CF& callback_function)
{
    const int num_files = mz_zip_reader_get_num_files(m_impl->GetZipArchive());
    mz_zip_archive_file_stat file_stat;

    for( int i = 0; i < num_files; ++i )
    {
        if( !mz_zip_reader_file_stat(m_impl->GetZipArchive(), i, &file_stat) )
            throw ZipException("Error reading the ZIP file entries.");

        if( !CallbackFunctionProcessor::KeepProcessing(callback_function, file_stat) )
            return;
    }
}


void ZipReader::ForeachFilePath(const std::function<bool(const char*)>& callback_function)
{
    ForeachFileWorker(
        [&](const mz_zip_archive_file_stat& file_stat)
        {
            return callback_function(file_stat.m_filename);
        });
}


void ZipReader::ForeachFileContents(const std::function<bool(const char*, BinaryBlock)>& callback_function)
{
    ForeachFileWorker(
        [&](const mz_zip_archive_file_stat& file_stat)
        {
            return callback_function(file_stat.m_filename,
                                     Read(file_stat, file_stat.m_filename));
        });
}


std::optional<std::string> ZipReader::FindPathInZip(const std::string_view filename_sv)
{
    ASSERT(filename_sv == PortableFunctions::PathGetFilename(filename_sv));

    std::optional<std::string> path;

    ForeachFilePath(
        [&](const char* const path_in_zip)
        {
            if( SO::EqualsNoCase(filename_sv, PortableFunctions::PathGetFilename(path_in_zip)) )
            {
                path = path_in_zip;
                return false;
            }

            return true;
        });

    return path;
}



// --------------------------------------------------------------------------
// ZipCreator
// --------------------------------------------------------------------------

ZipCreator::ZipCreator(std::string file_path)
    :   m_impl(std::make_unique<ZipImpl>()),
        m_filePath(std::move(file_path))
{
    if( PortableFunctions::FileExists(m_filePath) )
    {
        try
        {
            PortableFunctions::FileDeleteWithExceptions(m_filePath);
        }

        catch( const CSProException& exception )
        {
            throw ZipException::FileIOError(exception);
        }
    }

    else
    {
        ZipImpl::CreateDirectories(m_filePath, false);
    }

    if( !mz_zip_writer_init_file(m_impl->GetZipArchive(), m_filePath.c_str(), 0) )
        throw ZipException::CreationError(m_filePath, m_impl->GetLastError());
}


ZipCreator::ZipCreator(ZipCreator&&) = default;


ZipCreator::~ZipCreator()
{
    try
    {
        Close();
    }
    catch(...) { }
}


void ZipCreator::Close(std::unique_ptr<ZipImpl>& impl, const std::string& file_path)
{
    if( impl == nullptr )
        return;

    const bool finalize_success = mz_zip_writer_finalize_archive(impl->GetZipArchive());

    // ignore any errors closing the file
    mz_zip_writer_end(impl->GetZipArchive());

    impl.reset();

    if( !finalize_success )
    {
        PortableFunctions::FileDelete(file_path);
        throw ZipException::CreationError(file_path, impl->GetLastError());
    }
}


void ZipCreator::Close()
{
    Close(m_impl, m_filePath);
}


void ZipCreator::NormalizeFilePathAndCreateDirectoriesForFile(std::string& file_path_in_zip, std::set<std::string>* const directories_in_zip)
{
    // write file paths with forward slashes
    PortableFunctions::MakePathToForwardSlash(file_path_in_zip);

    // a "." directory is created in the archive if you use "./" so remove it
    if( SO::StartsWith(file_path_in_zip, "./") )
        file_path_in_zip = file_path_in_zip.substr(2);

    std::string directory_in_zip = PortableFunctions::PathGetDirectory(file_path_in_zip);

    // add the directory if it hasn't been added yet
    if( !directory_in_zip.empty() && ( directories_in_zip == nullptr ||
                                       directories_in_zip->find(directory_in_zip) == directories_in_zip->cend() ) )
    {
        if( !mz_zip_writer_add_mem(m_impl->GetZipArchive(), directory_in_zip.c_str(), nullptr, 0, MZ_BEST_COMPRESSION) )
            throw ZipException::CreationError(m_filePath, m_impl->GetLastError());

        if( directories_in_zip != nullptr )
            directories_in_zip->insert(std::move(directory_in_zip));
    }
}


int ZipCreator::AddFiles(const std::vector<std::string>& file_paths, const std::vector<std::string>& file_paths_in_zip)
{
    ASSERT(file_paths.size() == file_paths_in_zip.size());

    int files_compressed = 0;
    auto file_paths_itr = file_paths.begin();

    std::set<std::string> directories_in_zip;

    for( std::string file_path_in_zip : file_paths_in_zip )
    {
        NormalizeFilePathAndCreateDirectoriesForFile(file_path_in_zip, &directories_in_zip);

        // only add files, not directories
        if( !PortableFunctions::FileIsDirectory(*file_paths_itr) )
        {
            if( !mz_zip_writer_add_file(m_impl->GetZipArchive(), file_path_in_zip.c_str(), file_paths_itr->c_str(), nullptr, 0, MZ_BEST_COMPRESSION) )
                throw ZipException::CreationError(m_filePath, m_impl->GetLastError());

            ++files_compressed;
        }

        ++file_paths_itr;
    }

    return files_compressed;
}


int ZipCreator::AddFiles(const std::vector<std::string>& file_paths)
{
    // calculate the common root directory for all files
    const std::string root_directory = Path::GetCommonRoot(file_paths);

    if( root_directory.empty() )
        throw ZipException::CreationError(m_filePath, "It is not possible to create a ZIP file when the files exist on two or more drives.");

    // create the relative file paths for the zip file
    std::vector<std::string> file_paths_in_zip;

    for( const std::string& file_path : file_paths )
    {
        file_paths_in_zip.emplace_back(file_path.substr(root_directory.size()));

        ASSERT(file_paths_in_zip.back().find(":\\") == std::string::npos &&
               file_paths_in_zip.back().find("\\\\") == std::string::npos);

        ASSERT(MakeFullPath(root_directory, file_paths_in_zip.back()) == file_path);
    }

    return AddFiles(file_paths, file_paths_in_zip);
}


void ZipCreator::AddFiles(ZipReader& zip_reader, const std::vector<std::string>& file_paths_in_zip)
{
    for( const std::string& file_path_in_zip : file_paths_in_zip )
    {
        const int file_index = zip_reader.m_impl->LocateFile<int>(file_path_in_zip);

        if( !mz_zip_writer_add_from_zip_reader(m_impl->GetZipArchive(), zip_reader.m_impl->GetZipArchive(), file_index) )
            throw ZipException::CreationError(m_filePath, m_impl->GetLastError());
    }
}


void ZipCreator::AddContent(std::string file_path_in_zip, const void* const buffer, const size_t buffer_size)
{
    NormalizeFilePathAndCreateDirectoriesForFile(file_path_in_zip, nullptr);

    if( !mz_zip_writer_add_mem(m_impl->GetZipArchive(), file_path_in_zip.c_str(), buffer, buffer_size, MZ_BEST_COMPRESSION) )
        throw ZipException::CreationError(m_filePath, m_impl->GetLastError());
}


void ZipCreator::AddContent(std::string file_path_in_zip, const std::string_view text_sv)
{
    AddContent(std::move(file_path_in_zip), text_sv.data(), text_sv.length());
}



// --------------------------------------------------------------------------
// ZipUtility
// --------------------------------------------------------------------------

void ZipUtility::Combine(const std::string& base_file_path, const std::string& combine_file_path)
{
    // use ZipReader temporarily to open the base ZIP file to determine what files to combine
    ZipReader base_zip_reader(base_file_path);

    // use ZipReader to read the files to combine
    ZipReader combine_zip_reader(combine_file_path);

    std::vector<int> file_indices_to_combine;

    combine_zip_reader.ForeachFileWorker(
        [&](const mz_zip_archive_file_stat& file_stat)
        {
            if( !base_zip_reader.m_impl->LocateFile<bool>(file_stat.m_filename) )
                file_indices_to_combine.emplace_back(file_stat.m_file_index);
        });

    if( file_indices_to_combine.empty() )
        return;

    std::unique_ptr<ZipImpl> base_zip_impl = base_zip_reader.ReleaseZipImpl();
    std::exception_ptr combine_exception;

    try
    {
        // if there are files to combine, reopen the file for writing
        if( !mz_zip_writer_init_from_reader(base_zip_impl->GetZipArchive(), base_file_path.c_str()) )
            throw ZipException::CreationError(base_file_path, base_zip_impl->GetLastError());

        for( const int file_index_to_combine : file_indices_to_combine )
        {
            if( !mz_zip_writer_add_from_zip_reader(base_zip_impl->GetZipArchive(), combine_zip_reader.m_impl->GetZipArchive(), file_index_to_combine) )
                throw ZipException::CreationError(base_file_path, base_zip_impl->GetLastError());
        }
    }

    catch( const CSProException& )
    {
        combine_exception = std::current_exception();
    }

    ZipCreator::Close(base_zip_impl, base_file_path);

    if( combine_exception )
        std::rethrow_exception(combine_exception);
}
