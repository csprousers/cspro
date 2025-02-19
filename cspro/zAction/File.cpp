#include "stdafx.h"
#include <zToolsO/File.h>
#include <zUtilO/PortableFileSystem.h>


CREATE_ENUM_JSON_SERIALIZER(FileOverwriteFlag,
    { FileOverwriteFlag::Always,    "always" },
    { FileOverwriteFlag::Never,     "never" },
    { FileOverwriteFlag::Fail,      "fail" },
    { FileOverwriteFlag::Different, "different" })


namespace FileHelper
{
    std::string ReadFileText(const JsonNode& json_node, ActionInvoker::Caller& caller);

    FileIO::TextFile OpenTextFileForWriting(const JsonNode& json_node, ActionInvoker::Caller& caller);
}


FileOverwriteFlag ActionInvoker::Runtime::EvaluateFileOverwriteFlag(const JsonNode& json_node)
{
    if( !json_node.Contains(JK::overwrite) )
        return FileOverwriteFlag::Different;

    const JsonNode overwrite_json_node = json_node.Get(JK::overwrite);

    return !overwrite_json_node.IsBoolean() ? overwrite_json_node.Get<FileOverwriteFlag>() :
           overwrite_json_node.Get<bool>()  ? FileOverwriteFlag::Different :
                                              FileOverwriteFlag::Fail;
}


ActionInvoker::Result ActionInvoker::Runtime::File_copy(const JsonNode& json_node, Caller& caller)
{
    const auto [source_paths, return_results_as_an_array] = EvaluateFilePaths(json_node.Get(JK::source), caller, true, true);
    std::string base_destination_path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::destination));
    const FileOverwriteFlag file_overwrite_flag = EvaluateFileOverwriteFlag(json_node);

    if( Path::HasWildcardCharacters(base_destination_path) )
        throw CSProException("The destination path cannot include wildcards: " + base_destination_path);

    bool destination_is_directory;

    if( PortableFunctions::FileIsDirectory(base_destination_path) )
    {
        destination_is_directory = true;
    }

    // allow a directory to be specified by appending a slash to the end of the path
    else if( !base_destination_path.empty() && Path::IsSlashChar(base_destination_path.back()) )
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

    std::unique_ptr<JsonStringWriter> json_writer;

    if( return_results_as_an_array )
    {
        json_writer = Json::CreateStringWriter();
        json_writer->BeginArray();
    }

    for( const std::string& source_path : source_paths )
    {
        std::string destination_path = destination_is_directory ? Path::Combine(base_destination_path, PortableFunctions::PathGetFilename(source_path)) :
                                                                  base_destination_path;

        PortableFileSystem::FileCopy(source_path, destination_path, file_overwrite_flag);

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

    return Result::JsonText(*json_writer);
}


std::string FileHelper::ReadFileText(const JsonNode& json_node, ActionInvoker::Caller& caller)
{
    const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

    // if the encoding is not explicitly using the "encoding" key specified, the default will be UTF-8
    static_assert(TextEncoding::DefaultEncodingIfNoBom == TextEncoding::Type::Utf8);

    TextEncoding text_encoding;
    FileIO::TextFile::ParseProperties(json_node, &text_encoding, nullptr);

    return FileIO::ReadText(path, text_encoding);
}


ActionInvoker::Result ActionInvoker::Runtime::File_readText(const JsonNode& json_node, Caller& caller)
{
    return Result::String(FileHelper::ReadFileText(json_node, caller));
}


ActionInvoker::Result ActionInvoker::Runtime::File_readLines(const JsonNode& json_node, Caller& caller)
{
    const std::string file_text = FileHelper::ReadFileText(json_node, caller);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginArray();

    SO::ForeachLine(file_text, true,
        [&](const std::string_view line_sv)
        {
            json_writer->Write(line_sv);
        });

    json_writer->EndArray();

    return Result::JsonText(*json_writer);
}


FileIO::TextFile FileHelper::OpenTextFileForWriting(const JsonNode& json_node, ActionInvoker::Caller& caller)
{
    std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

    // default to writing text as UTF-8 without a BOM and writing newlines as \n...
    static_assert(TextEncoding::DefaultEncoding != TextEncoding::Type::Utf8);

    FileIO::TextFile text_file;
    text_file.SetTextEncoding(TextEncoding::Type::Utf8);
    text_file.SetWriteNewline(false);

    // ...but the "encoding" and "newline" values can be modified
    text_file.SetProperties(json_node);

    // open the file
    text_file.OpenForTextWritingCreate(std::move(path));

    return text_file;
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeText(const JsonNode& json_node, Caller& caller)
{
    const std::string text = json_node.Get<std::string>(JK::text);
    FileIO::TextFile text_file = FileHelper::OpenTextFileForWriting(json_node, caller);

    text_file.WriteString(text);

    text_file.Close();

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeLines(const JsonNode& json_node, Caller& caller)
{
    const std::vector<std::string> lines = json_node.Get<std::vector<std::string>>(JK::lines);
    FileIO::TextFile text_file = FileHelper::OpenTextFileForWriting(json_node, caller);

    for( const std::string& line : lines )
        text_file.WriteLine(line);

    text_file.Close();

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::File_readBytes(const JsonNode& json_node, Caller& caller)
{
    const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));
    BytesToStringConverter bytes_to_string_converter(this, json_node, JK::bytesFormat);

    return Result::String(bytes_to_string_converter.Convert(FileIO::Read(path),
                                                            ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path)))));
}


ActionInvoker::Result ActionInvoker::Runtime::File_writeBytes(const JsonNode& json_node, Caller& caller)
{
    const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));
    const std::string_view bytes_sv = json_node.Get<std::string_view>(JK::bytes);
    const std::shared_ptr<const std::vector<std::byte>> bytes = StringToBytesConverter::Convert(*this, bytes_sv, json_node, JK::bytesFormat);

    FileIO::Write(path, *bytes);

    return Result::Undefined();
}
