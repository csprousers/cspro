#pragma once

#include <zToolsO/File.h>


class EncodedTextWriter : private FileIO::TextFile
{
public:
    EncodedTextWriter(const std::string& file_path, const ConnectionString& connection_string_with_properties)
    {
        // default to writing UTF-8 with a BOM so that other tools (like Excel) know that this is UTF-8,
        // but this can be overridden based on properties from the connection string
        SetTextEncoding(TextEncoding::Type::Utf8Bom);
        SetProperties(connection_string_with_properties);

        SetupEnvironmentToCreateFile(file_path);

        OpenForTextWritingCreate(file_path);
    }

    template<typename... Args>
    void WriteLine(Args const&... args)
    {
        TextFile::WriteLine(args...);
    }

    template<typename... Args>
    void WriteFormattedLine(const char* const formatter, Args const&... args)
    {
        TextFile::WriteFormattedLine(formatter, args...);
    }
};
