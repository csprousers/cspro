#include "StdAfx.h"
#include "FileIO.h"
#include "Utf8FileStream.h"
#include <fstream>


FileIO::Exception FileIO::Exception::DirectoryNotFound(const NullTerminatedString path)
{
    ASSERT(!PortableFunctions::FileExists(path));
    return Exception(_T("The directory does not exist: %s"), path.c_str());
}


FileIO::Exception FileIO::Exception::FileNotFound(const NullTerminatedString filename)
{
    return Exception(_T("The file does not exist: %s"), filename.c_str());
}


FileIO::Exception FileIO::Exception::FileOpenError(const NullTerminatedString filename)
{
    return Exception(_T("The file could not be opened: %s"), filename.c_str());
}


FileIO::Exception FileIO::Exception::FileNotFullyWritten(const NullTerminatedString filename, const bool delete_file_from_disk)
{
    if( delete_file_from_disk )
        PortableFunctions::FileDelete(filename);

    return Exception(_T("The file '%s' could not be fully written."), PortableFunctions::PathGetFilename(filename));
}


FileIO::Exception FileIO::Exception::FileCopyFail(const NullTerminatedString source_path, const NullTerminatedString destination_path)
{
    return Exception(_T("There was an error copying '%s' to: %s"), source_path.c_str(), destination_path.c_str());
}


FileIO::Exception FileIO::Exception::FileCopyFailDestinationExists(const NullTerminatedString source_path, const NullTerminatedString destination_path)
{
    return Exception(_T("There was an error copying '%s' to '%s' because the destination file already exists."), source_path.c_str(), destination_path.c_str());
}


inline FileIO::FileAndSize FileIO::OpenFile(NullTerminatedString filename)
{
    if( !PortableFunctions::FileIsRegular(filename) )
        throw Exception::FileNotFound(filename);

    const int64_t file_size = PortableFunctions::FileSize(filename);

    if( file_size < 0 )
        throw Exception(_T("The file '%s' has an invalid size."), PortableFunctions::PathGetFilename(filename));

    FILE* file = PortableFunctions::FileOpen(filename, _T("rb"));

    if( file == nullptr )
        throw Exception::FileOpenError(filename);

    return FileAndSize { file, file_size };
}


namespace
{
    template<typename GDSC>
    void ReadWorker(NullTerminatedString filename, GDSC get_data_and_size_callback)
    {
        FileIO::FileAndSize file_and_size = FileIO::OpenFile(filename);

        void* data = get_data_and_size_callback(file_and_size.size);

        bool read_success = ( fread(data, 1, static_cast<size_t>(file_and_size.size), file_and_size.file) == file_and_size.size );

        fclose(file_and_size.file);

        if( !read_success )
            throw FileIO::Exception(_T("The file '%s' could not be fully read."), PortableFunctions::PathGetFilename(filename));
    }
}


std::unique_ptr<std::vector<std::byte>> FileIO::Read(NullTerminatedString filename)
{
    std::unique_ptr<std::vector<std::byte>> file_content;

    ReadWorker(filename,
        [&](int64_t& file_size)
        {
            file_content = std::make_unique<std::vector<std::byte>>(static_cast<size_t>(file_size));
            return file_content->data();
        });

    return file_content;
}


template<>
CLASS_DECL_ZTOOLSO std::string FileIO::ReadText(NullTerminatedString filename)
{
    std::string text;

    ReadWorker(filename,
        [&](int64_t& file_size)
        {
            text.resize(static_cast<size_t>(file_size));
            return text.data();
        });

    if( HasUtf8BOM(text.data(), text.size()) )
        return text.substr(Utf8BOM_sv.length());

    return text;
}


template<>
CLASS_DECL_ZTOOLSO std::wstring FileIO::ReadText(NullTerminatedString filename)
{
    std::unique_ptr<char[]> buffer;
    size_t buffer_size;

    ReadWorker(filename,
        [&](int64_t& file_size)
        {
            buffer_size = static_cast<size_t>(file_size);
            buffer = std::make_unique<char[]>(buffer_size);
            return buffer.get();
        });

    const char* buffer_start = buffer.get();

    if( HasUtf8BOM(buffer_start, buffer_size) )
    {
        buffer_start += Utf8BOM_sv.length();
        buffer_size -= Utf8BOM_sv.length();
    }

    return UTF8Convert::UTF8ToWide(buffer_start, buffer_size);
}


CString FileIO::ReadText(NullTerminatedString filename, int64_t max_bytes_to_read, const TCHAR* message/* = nullptr*/)
{
    bool file_is_larger_than_max_bytes;
    std::unique_ptr<char[]> buffer;
    size_t buffer_size;

    ReadWorker(filename,
        [&](int64_t& file_size)
        {
            file_is_larger_than_max_bytes = ( file_size > max_bytes_to_read );

            if( file_is_larger_than_max_bytes )
                file_size = max_bytes_to_read;

            buffer_size = static_cast<size_t>(file_size);
            buffer = std::make_unique<char[]>(buffer_size);
            return buffer.get();
        });

    const char* buffer_start = buffer.get();

    if( HasUtf8BOM(buffer_start, buffer_size) )
    {
        buffer_start += Utf8BOM_sv.length();
        buffer_size -= Utf8BOM_sv.length();
    }

    // make sure that the data doesn't end in the middle of a UTF-8 sequence (if the whole file wasn't read in)
    if( file_is_larger_than_max_bytes && buffer_size > 0 )
    {
        const char* buffer_end_itr = buffer_start + buffer_size;

        if( ( *( buffer_end_itr - 1 ) & 0x80 ) == 0x80 )
        {
            do
            {
                --buffer_size;
                --buffer_end_itr;

            } while( buffer_size > 0 && ( *buffer_end_itr & 0xC0 ) != 0xC0 );
        }
    }

    CString text = UTF8Convert::UTF8ToWide<CString>(buffer_start, buffer_size);

    if( file_is_larger_than_max_bytes )
        text.Append(message);

    return text;
}


std::unique_ptr<std::wistream> FileIO::OpenWideTextInputFileStream(NullTerminatedString filename)
{
    return std::make_unique<Utf8InputFileStream>(filename);
}


std::unique_ptr<std::ifstream> FileIO::OpenTextInputFileStream(NullTerminatedString filename)
{
    if( !PortableFunctions::FileIsRegular(filename) )
        throw Exception::FileNotFound(filename);

    auto stream = std::make_unique<std::ifstream>();

    stream->open(filename.c_str(), std::ifstream::in);

    if( !stream )
        throw Exception::FileOpenError(filename);

    // potentially skip past the BOM
    char bom[Utf8BOM_sv.length()];
    stream->read(bom, Utf8BOM_sv.length());

    if( !HasUtf8BOM(bom, static_cast<size_t>(stream->gcount())) )
        stream->seekg(0, std::ios::beg);

    return stream;
}


void FileIO::CreateDirectories(const std::wstring& directory)
{
    if( !PortableFunctions::PathMakeDirectories(directory) )
        throw Exception(_T("The directory '%s' could not be created."), directory.c_str());
}


void FileIO::CreateDirectoriesForFile(NullTerminatedString filename)
{
    const std::wstring directory = PortableFunctions::PathGetDirectory(filename);

    if( !PortableFunctions::PathMakeDirectories(directory) )
    {
        throw Exception(_T("The directory '%s' could not be created to create '%s'."),
                        directory.c_str(), PortableFunctions::PathGetFilename(filename));
    }
}


std::unique_ptr<std::ofstream> FileIO::OpenOutputFileStream(NullTerminatedString filename)
{
    CreateDirectoriesForFile(filename);

    auto file_stream = std::make_unique<std::ofstream>(filename.c_str(), std::ios::out | std::ios::binary);

    if( file_stream->fail() )
        throw Exception(_T("The file '%s' could not be created."), PortableFunctions::PathGetFilename(filename));

    return file_stream;
}


FILE* FileIO::OpenFileForOutput(NullTerminatedString filename)
{
    CreateDirectoriesForFile(filename);

    FILE* file = PortableFunctions::FileOpen(filename, _T("wb"));

    if( file == nullptr )
        throw Exception(_T("The file '%s' could not be created."), PortableFunctions::PathGetFilename(filename));

    return file;
}


namespace
{
    template<typename WC>
    void WriteWorker(NullTerminatedString filename, WC write_callback)
    {
        FILE* file = FileIO::OpenFileForOutput(filename);

        const bool write_success = write_callback(file);

        fclose(file);

        if( !write_success )
            throw FileIO::Exception::FileNotFullyWritten(filename, true);
    }
}


void FileIO::Write(NullTerminatedString filename, const std::byte* content, size_t content_size)
{
    WriteWorker(filename,
        [&](FILE* file)
        {
            return ( fwrite(content, 1, content_size, file) == content_size );
        });
}


void FileIO::WriteText(NullTerminatedString filename, std::string_view text_content, bool write_utf8_bom)
{
    WriteWorker(filename,
        [&](FILE* file)
        {
            return ( ( !write_utf8_bom || fwrite(Utf8BOM_sv.data(), 1, Utf8BOM_sv.length(), file) == Utf8BOM_sv.length() ) &&
                     ( fwrite(text_content.data(), 1, text_content.size(), file) == text_content.size() ) );
        });
}


void FileIO::WriteText(NullTerminatedString filename, wstring_view text_content, bool write_utf8_bom)
{
    WriteText(filename, UTF8Convert::WideToUTF8(text_content), write_utf8_bom);
}
