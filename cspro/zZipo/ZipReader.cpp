#include "stdafx.h"
#include "ZipReader.h"
#include "ZipHelpers.h"


ZipReader::ZipReader(NullTerminatedString filename)
    :   m_zipArchive(std::make_unique<mz_zip_archive_tag>(mz_zip_archive { 0 }))
{
    if( !mz_zip_reader_init_file(m_zipArchive.get(), filename.c_str(), 0) )
        throw CSProException(_T("The file \"%s\" is not a valid ZIP file."), PortableFunctions::PathGetFilename(filename));
}


ZipReader::~ZipReader()
{
    // ignore any errors closing the file
    mz_zip_reader_end(m_zipArchive.get());
}


std::unique_ptr<std::vector<std::byte>> ZipReader::Read(wstring_view path_in_zip)
{
    int file_index = ZipHelpers::LocateFile(m_zipArchive.get(), path_in_zip);
    mz_zip_archive_file_stat file_stat;

    if( file_index < 0 || !mz_zip_reader_file_stat(m_zipArchive.get(), file_index, &file_stat) )
        throw CSProException(_T("The ZIP file does not contain the entry: \"%s\""), std::wstring(path_in_zip).c_str());

    auto buffer = std::make_unique<std::vector<std::byte>>(static_cast<size_t>(file_stat.m_uncomp_size));

    if( !mz_zip_reader_extract_to_mem(m_zipArchive.get(), file_index, buffer->data(), buffer->size(), 0) )
        throw CSProException(_T("Error reading the ZIP file entry: \"%s\""), std::wstring(path_in_zip).c_str());

    return buffer;
}


void ZipReader::ForeachFile(const std::function<bool(const std::wstring&)>& callback)
{
    mz_zip_archive_file_stat file_stat;
    mz_uint num_files = mz_zip_reader_get_num_files(m_zipArchive.get());

    for( mz_uint i = 0; i < num_files; ++i )
    {
        if( !mz_zip_reader_file_stat(m_zipArchive.get(), i, &file_stat) )
            throw CSProException("Error reading the ZIP file entries.");

        if( !callback(UTF8Convert::UTF8ToWide(file_stat.m_filename)) )
            break;
    }
}


std::optional<std::wstring> ZipReader::FindPathInZip(wstring_view filename)
{
    ASSERT(filename == PortableFunctions::PathGetFilename(std::wstring(filename)));

    std::optional<std::wstring> path;

    ForeachFile(
        [&](const std::wstring& path_in_zip)
        {
            if( SO::EqualsNoCase(filename, PortableFunctions::PathGetFilename(path_in_zip)) )
            {
                path = path_in_zip;
                return false;
            }

            return true;
        });

    return path;
}
