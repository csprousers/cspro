#include "StdAfx.h"
#include "FileIO.h"
#include "TextEncoding.h"
#include "Utf8FileStream.h"
#include <fstream>


FileIO::Exception FileIO::Exception::DirectoryNotFound(const InterfaceString directory_path)
{
    ASSERT(!PortableFunctions::FileExists(directory_path));
    return Exception("The directory does not exist: %s", directory_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileNotFound(const InterfaceString file_path)
{
    return Exception("The file does not exist: %s", file_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileOpenError(const InterfaceString file_path)
{
    return Exception("The file could not be opened: %s", file_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileCreateError(const InterfaceString file_path)
{
    return Exception("The file could not be created: %s", file_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileReadError(const InterfaceString file_path)
{
    return Exception("The file '%s' could not be fully read.",
                     PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str());
}


FileIO::Exception FileIO::Exception::FileNotFullyWritten(const InterfaceString file_path, const bool delete_file_from_disk)
{
    if( delete_file_from_disk )
        PortableFunctions::FileDelete(file_path);

    return Exception("The file '%s' could not be fully written.",
                     PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str());
}


FileIO::Exception FileIO::Exception::FileCopyFail(const InterfaceString source_path, const InterfaceString destination_path)
{
    return Exception("There was an error copying '%s' to: %s",
                     source_path.c_str_utf8(), destination_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileCopyFailDestinationExists(const InterfaceString source_path, const InterfaceString destination_path)
{
    return Exception("There was an error copying '%s' to '%s' because the destination file already exists.",
                     source_path.c_str_utf8(), destination_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileMoveFail(const InterfaceString source_path, const InterfaceString destination_path)
{
    return Exception("There was an error moving '%s' to: %s",
                     source_path.c_str_utf8(), destination_path.c_str_utf8());
}


FileIO::Exception FileIO::Exception::FileDeleteFail(const InterfaceString file_path)
{
    return Exception("There was an error deleting '%s'", file_path.c_str_utf8());
}


FileIO::FileAndSize FileIO::OpenFile(const InterfaceString file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        throw Exception::FileNotFound(file_path);

    const int64_t file_size = PortableFunctions::FileSize(file_path);

    if( file_size < 0 )
    {
        throw Exception("The file '%s' has an invalid size.",
                        PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str());
    }

    FILE* const file = PortableFunctions::FileOpen(file_path, "rb");

    if( file == nullptr )
        throw Exception::FileOpenError(file_path);

    return FileAndSize { file, file_size };
}


namespace
{
    template<typename GDSC>
    void ReadWorker(const InterfaceString file_path, const GDSC& get_data_and_size_callback)
    {
        FileIO::FileAndSize file_and_size = FileIO::OpenFile(file_path);

        void* data = get_data_and_size_callback(file_and_size.size);

        const bool read_success = ( fread(data, 1, static_cast<size_t>(file_and_size.size), file_and_size.file) == static_cast<size_t>(file_and_size.size) );

        fclose(file_and_size.file);

        if( !read_success )
            throw FileIO::Exception::FileReadError(file_path);
    }
}


std::unique_ptr<std::vector<std::byte>> FileIO::Read(InterfaceString file_path)
{
    std::unique_ptr<std::vector<std::byte>> file_content;

    ReadWorker(std::move(file_path),
        [&](int64_t& file_size)
        {
            file_content = std::make_unique<std::vector<std::byte>>(static_cast<size_t>(file_size));
            return file_content->data();
        });

    return file_content;
}


BinaryBlock FileIO::ReadBinary(InterfaceString file_path)
{
    std::optional<BinaryBlock> file_content;

    ReadWorker(std::move(file_path),
        [&](int64_t& file_size)
        {
            file_content.emplace(static_cast<size_t>(file_size));
            return file_content->data();
        });

    return std::move(*file_content);
}


std::string FileIO::ReadText(InterfaceString file_path, TextEncoding text_encoding/* = TextEncoding::Type::Utf8*/)
{
    std::string text;

    ReadWorker(std::move(file_path),
        [&](int64_t& file_size)
        {
            text.resize(static_cast<size_t>(file_size));
            return text.data();
        });

    // process a potential BOM
    text_encoding.UpdateEncoding(text);

    // if UTF-8 without a BOM, we can return the text directly
    if( text_encoding.GetType() == TextEncoding::Type::Utf8 )
    {
        return text;
    }

    // if UTF-8 with a BOM, we can remove the BOM and then return the text directly
    else if( text_encoding.GetType() == TextEncoding::Type::Utf8Bom )
    {
        ASSERT81(SO::StartsWith(text, TextEncoding::Utf8Bom_sv));
        return text.erase(0, TextEncoding::Utf8Bom_sv.length());
    }

    // otherwise we have to potentially remove the BOM and then convert the text
    else
    {
        const std::unique_ptr<TextEncoding::Converter> text_converter = text_encoding.CreateConverter();
        ASSERT(text_converter != nullptr);

        return text_converter->ToUtf8(std::string_view(text).substr(text_encoding.GetBomLength()));
    }
}


std::string FileIO::ReadText(InterfaceString file_path, const int64_t max_bytes_to_read, const char* const message/* = nullptr*/)
{
    bool file_is_larger_than_max_bytes;
    std::string text;

    ReadWorker(std::move(file_path),
        [&](int64_t& file_size)
        {
            file_is_larger_than_max_bytes = ( file_size > max_bytes_to_read );

            if( file_is_larger_than_max_bytes )
                file_size = max_bytes_to_read;

            text.resize(static_cast<size_t>(file_size), '\0');

            return text.data();
        });

    // potentially remove the BOM
    if( SO::StartsWith(text, TextEncoding::Utf8Bom_sv) )
        text.erase(0, TextEncoding::Utf8Bom_sv.length());

    if( file_is_larger_than_max_bytes )
    {
        // make sure that the text doesn't end in the middle of a UTF-8 sequence (if the whole file wasn't read in)
        TC::EnsureValidEndingUtf8Sequence(text);

        // add a custom message
        if( message != nullptr )
            text.append(message);
    }

    return text;
}


std::unique_ptr<std::wistream> FileIO::OpenWideTextInputFileStream(InterfaceString file_path)
{
    return std::make_unique<Utf8InputFileStream>(std::move(file_path));
}


std::unique_ptr<std::ifstream> FileIO::OpenTextInputFileStream(const InterfaceString file_path, std::streampos* const file_size/* = nullptr*/)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        throw Exception::FileNotFound(file_path);

    auto stream = std::make_unique<std::ifstream>();

    stream->open(file_path.GetString<std::wstring>().c_str(), std::ifstream::in);

    if( !stream )
        throw Exception::FileOpenError(file_path);

    // potentially skip past the BOM
    char bom[TextEncoding::Utf8Bom_sv.length()];
    stream->read(bom, TextEncoding::Utf8Bom_sv.length());

    const TextEncoding text_encoding(bom, static_cast<size_t>(stream->gcount()));
    bool need_to_reset_stream;

    if( file_size != nullptr )
    {
        stream->seekg(0, std::ios::end);
        *file_size = stream->tellg();

        need_to_reset_stream = true;
    }

    else
    {
        need_to_reset_stream = ( text_encoding.GetType() != TextEncoding::Type::Utf8Bom );
    }

    if( need_to_reset_stream )
    {
        const size_t reset_stream_pos = ( text_encoding.GetType() == TextEncoding::Type::Utf8Bom ) ? TextEncoding::GetBomLength(TextEncoding::Type::Utf8Bom) :
                                                                                                     0;
        stream->seekg(reset_stream_pos, std::ios::beg);
    }

    return stream;
}


void FileIO::CreateDirectories(const InterfaceString directory_path)
{
    if( !PortableFunctions::PathMakeDirectories(directory_path) )
        throw Exception("The directory '%s' could not be created.", directory_path.c_str_utf8());
}


void FileIO::CreateDirectoriesForFile(const InterfaceString file_path)
{
    const std::string directory_path = PortableFunctions::PathGetDirectory(file_path.GetString<std::string>());

    if( !PortableFunctions::PathMakeDirectories(directory_path) )
    {
        throw Exception("The directory '%s' could not be created to create '%s'.",
                        directory_path.c_str(), PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str());
    }
}


std::unique_ptr<std::ofstream> FileIO::OpenOutputFileStream(const InterfaceString file_path)
{
    CreateDirectoriesForFile(file_path);

    auto file_stream = std::make_unique<std::ofstream>(file_path.GetString<std::wstring>().c_str(), std::ios::out | std::ios::binary);

    if( file_stream->fail() )
        throw Exception::FileCreateError(file_path);

    return file_stream;
}


FILE* FileIO::OpenFileForOutput(const InterfaceString file_path)
{
    CreateDirectoriesForFile(file_path);

    FILE* const file = PortableFunctions::FileOpen(file_path, "wb");

    if( file == nullptr )
        throw Exception::FileCreateError(file_path);

    return file;
}


namespace
{
    template<typename WC>
    void WriteWorker(const InterfaceString file_path, const WC& write_callback)
    {
        FILE* const file = FileIO::OpenFileForOutput(file_path);

        const bool write_success = write_callback(file);

        fclose(file);

        if( !write_success )
            throw FileIO::Exception::FileNotFullyWritten(file_path, true);
    }
}


void FileIO::Write(InterfaceString file_path, const void* const content, const size_t content_size)
{
    WriteWorker(std::move(file_path),
        [&](FILE* const file)
        {
            return ( fwrite(content, 1, content_size, file) == content_size );
        });
}


void FileIO::WriteText(InterfaceString file_path, const std::string_view text_content_sv, const bool write_utf8_bom)
{
    WriteWorker(std::move(file_path),
        [&](FILE* const file)
        {
            return ( ( !write_utf8_bom || fwrite(TextEncoding::Utf8Bom_sv.data(), 1, TextEncoding::Utf8Bom_sv.length(), file) == TextEncoding::Utf8Bom_sv.length() ) &&
                     ( fwrite(text_content_sv.data(), 1, text_content_sv.size(), file) == text_content_sv.size() ) );
        });
}
