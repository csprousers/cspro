#include "stdafx.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/TextConverter.h>
#include <zUtilO/PortableFileSystem.h>


namespace FileHelper
{
    std::wstring ReadFileText(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller);

    template<typename CF>
    void WriteFileText(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller, CF callback_function);
}


ActionInvoker::Result ActionInvoker::Runtime::File_copy(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const auto [source_paths, return_results_as_an_array] = EvaluateFilePaths(json_node.Get(JK::source), caller, true);
    std::wstring base_destination_path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::destination));
    const bool overwrite = json_node.GetOrDefault(JK::overwrite, true);

    if( PathHasWildcardCharacters(base_destination_path) )
        throw CSProException(_T("The destination path cannot include wildcards: ") + base_destination_path);

    bool destination_is_directory;

    if( PortableFunctions::FileIsDirectory(base_destination_path) )
    {
        destination_is_directory = true;
    }

    // allow a directory to be specified by appending a slash to the end of the path
    else if( !base_destination_path.empty() && PortableFunctions::IsPathCharacter(base_destination_path.back()) )
    {
        base_destination_path = PortableFunctions::PathRemoveTrailingSlash(base_destination_path);

        FileIO::CreateDirectories(base_destination_path);

        destination_is_directory = true;
    }

    else
    {
        if( source_paths.size() > 1 )
            throw CSProException("When copying multiple files, the destination path must be a directory.");

        FileIO::CreateDirectoriesForFile(base_destination_path);

        destination_is_directory = false;
    }

    std::unique_ptr<JsonStringWriter<wchar_t>> json_writer;

    if( return_results_as_an_array )
    {
        json_writer = Json::CreateStringWriter();
        json_writer->BeginArray();
    }

    for( const std::wstring& source_path : source_paths )
    {
        std::wstring destination_path = destination_is_directory ? PortableFunctions::PathAppendToPath(base_destination_path, PortableFunctions::PathGetFilename(source_path)) :
                                                                   base_destination_path;

        PortableFileSystem::FileCopy(source_path, destination_path, overwrite);

        if( return_results_as_an_array )
        {
            json_writer->Write(destination_path);
        }

        else
        {
            ASSERT(source_paths.size() == 1);
            return Result::String(std::move(destination_path));
        }
    }

    ASSERT(return_results_as_an_array);

    json_writer->EndArray();

    return Result::JsonText(json_writer);
}


std::wstring FileHelper::ReadFileText(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller)
{
    const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

    // if the encoding is not explicitly specified, the default will be UTF-8
    const bool read_as_ansi = ( json_node.Contains(JK::encoding) &&
                                json_node.Get<TextEncoding>(JK::encoding) == TextEncoding::Ansi );

    if( read_as_ansi )
    {
        std::unique_ptr<const std::vector<std::byte>> content = FileIO::Read(path);
        return TextConverter::WindowsAnsiToWide(reinterpret_cast<const char*>(content->data()), content->size());
    }

    else
    {
        return FileIO::ReadText(path);
    }
}


ActionInvoker::Result ActionInvoker::Runtime::File_readText(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    return Result::String(FileHelper::ReadFileText(json_node, caller));
}


ActionInvoker::Result ActionInvoker::Runtime::File_readLines(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring file_text = FileHelper::ReadFileText(json_node, caller);

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginArray();

    SO::ForeachLine(file_text, true,
        [&](const wstring_view line_sv)
        {
            json_writer->Write(line_sv);
            return true;
        });

    json_writer->EndArray();

    return Result::JsonText(json_writer);
}


template<typename CF>
void FileHelper::WriteFileText(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller, CF callback_function)
{
    const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

    const TextEncoding text_encoding = json_node.Contains(JK::encoding) ? json_node.Get<TextEncoding>(JK::encoding) :
                                                                          TextEncoding::Utf8;

    FILE* file = FileIO::OpenFileForOutput(path);

    auto write_bytes_function = [&](const std::string_view text_sv)
    {
        if( fwrite(text_sv.data(), 1, text_sv.length(), file) != text_sv.length() )
        {
            fclose(file);
            throw FileIO::Exception::FileNotFullyWritten(path, true);
        }
    };

    auto write_text_function = [&](const wstring_view text_sv)
    {
        const std::string encoded_text = ( text_encoding == TextEncoding::Ansi ) ? TextConverter::WideToWindowsAnsi(text_sv) :
                                                                                   UTF8Convert::WideToUTF8(text_sv);
        write_bytes_function(encoded_text);
    };

    if( text_encoding == TextEncoding::Utf8Bom )
        write_bytes_function(Utf8BOM_sv);

    callback_function(write_bytes_function, write_text_function);

    fclose(file);
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeText(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring text = json_node.Get<std::wstring>(JK::text);

    FileHelper::WriteFileText(json_node, caller,
        [&](const auto& /*write_bytes_function*/, const auto& write_text_function)
        {
            write_text_function(text);
        });

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeLines(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    constexpr std::string_view NewLine_sv("\n");

    const std::vector<std::wstring> lines = json_node.Get<std::vector<std::wstring>>(JK::lines);

    FileHelper::WriteFileText(json_node, caller,
        [&](const auto& write_bytes_function, const auto& write_text_function)
        {
            for( const std::wstring& line : lines )
            {
                write_text_function(line);
                write_bytes_function(NewLine_sv);
            }
        });

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::File_readBytes(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));
    BytesToStringConverter bytes_to_string_converter(this, json_node, JK::bytesFormat);

    return Result::String(bytes_to_string_converter.Convert(FileIO::Read(path),
                                                            MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path))));
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeBytes(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));
    const wstring_view bytes_sv = json_node.Get<wstring_view>(JK::bytes);
    const std::vector<std::byte> bytes = StringToBytesConverter::Convert(bytes_sv, json_node, JK::bytesFormat);

    FileIO::Write(path, bytes);

    return Result::Undefined();
}
