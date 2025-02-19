#include "stdafx.h"
#include "JsonSpecFile.h"
#include <zToolsO/Utf8.h>
#include <zUtilO/Versioning.h>


// --------------------------------------------------------------------------
// writing spec files creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonFileWriter> JsonSpecFile::CreateWriter(InterfaceString file_path, const std::string_view file_type_sv)
{
    std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(std::move(file_path));

    json_writer->BeginObject();

    WriteHeading(*json_writer, file_type_sv);

    return json_writer;
}


void JsonSpecFile::WriteHeading(JsonWriter& json_writer, const std::string_view file_type_sv)
{
    json_writer.Write(JK::software, "CSPro")
               .Write(JK::version, Versioning::Number)
               .Write(JK::fileType, file_type_sv);
}



// --------------------------------------------------------------------------
// JsonSpecFile::ReaderMessageLogger
// --------------------------------------------------------------------------

void JsonSpecFile::ReaderMessageLogger::LogWarning(const std::string& file_path, std::string message)
{
    auto message_lookup = std::find_if(m_messageSets.begin(), m_messageSets.end(),
                                       [&](const auto& m) { return SO::EqualsNoCase(std::get<0>(m), file_path); });

    auto& message_set = ( message_lookup == m_messageSets.end() ) ? m_messageSets.emplace_back(file_path, std::vector<std::string>()) :
                                                                    *message_lookup;
    std::get<1>(message_set).emplace_back(std::move(message));
}


std::string JsonSpecFile::ReaderMessageLogger::GetErrorText(std::string initial_text) const
{
    ASSERT(!m_messageSets.empty());

    std::string& combined_message_text = initial_text;

    for( const auto& [file_path, messages] : m_messageSets )
    {
        if( m_messageSets.size() == 1 )
        {
            combined_message_text.append(FormatText(" were problems reading '%s':\n", PortableFunctions::PathGetFilename(file_path).c_str()));
        }

        else
        {
            if( combined_message_text.empty() )
                combined_message_text.append(" were problems reading multiple files:");

            const std::string filename = PortableFunctions::PathGetFilename(file_path);

            combined_message_text.append(FormatText("\n\n%s\n%s\n", filename.c_str(), SO::GetDashedLine(filename.length())));
        }

        for( const std::string& message : messages )
        {
            combined_message_text.append(u8"\n  • ")
                                 .append(message);
        }
    }

    return combined_message_text;
}


void JsonSpecFile::ReaderMessageLogger::DisplayWarnings(const bool silent/* = false*/) const
{
    if( !silent && !m_messageSets.empty() )
        ErrorMessage::Display(GetErrorText("There"));
}


void JsonSpecFile::ReaderMessageLogger::RethrowException(const InterfaceString file_path, const CSProException& exception) const
{
    std::string message = FormatText(u8"There was an error reading '%s':\n\n  ⚠️ %s",
                                     PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str(), exception.what());

    if( !m_messageSets.empty() )
        message.append(GetErrorText("\n\nIn addition, there"));

    throw CSProException(message);
}



// --------------------------------------------------------------------------
// JsonSpecFile::Reader
// --------------------------------------------------------------------------

JsonSpecFile::Reader::Reader(const std::string_view json_text_sv, InterfaceString file_path, std::shared_ptr<ReaderMessageLogger> message_logger)
    :   JsonNode(json_text_sv, this),
        JsonReaderInterface(PortableFunctions::PathGetDirectory(file_path.GetString<std::string>())),
        m_filePath(file_path.Release<std::string>()),
        m_messageLogger(message_logger)
{
    ASSERT(m_messageLogger != nullptr);
}


double JsonSpecFile::Reader::CheckVersion()
{
    const double version = GetOrDefault(JK::version, Versioning::Number);

    if( version > Versioning::Number )
        LogWarning("The file was created using CSPro %0.1f and may use features not supported by this version (CSPro %0.1f)", version, Versioning::Number);

    return version;
}


void JsonSpecFile::Reader::CheckFileType(const std::string_view file_type_sv)
{
    const std::optional<std::string_view> file_type_json_sv = GetOptional<std::string_view>(JK::fileType);

    if( file_type_json_sv.has_value() && file_type_json_sv != file_type_sv )
        throw CSProException("The file type '%s' cannot be read by this program", std::string(*file_type_json_sv).c_str());

#ifdef WIN_DESKTOP
    if( !GetOrDefault(JK::editable, true) )
    {
        auto exe_name = std::make_unique_for_overwrite<wchar_t[]>(_MAX_PATH);
        GetModuleFileName(AfxGetApp()->m_hInstance, exe_name.get(), _MAX_PATH);

        if( SO::EqualsNoCase(Path::GetFilenameWithoutExtension(TC::ToUtf8(exe_name.get())), "CSPro") )
            throw CSProException("This file has been locked and cannot be edited using the CSPro Designer");
    }
#endif
}



// --------------------------------------------------------------------------
// reading spec file creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(const InterfaceString file_path,
                                                                 const std::string_view json_text_sv,
                                                                 std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger)
{
    if( message_logger == nullptr )
        message_logger = std::make_unique<JsonSpecFile::ReaderMessageLogger>();

    try
    {
        return std::make_unique<JsonSpecFile::Reader>(json_text_sv, file_path, std::move(message_logger));
    }

    catch( const JsonParseException& exception )
    {
        throw CSProException("There was an error reading '%s' as it contains invalid JSON:\n\n%s",
                             PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str(),
                             exception.what());
    }
}


std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(InterfaceString file_path,
                                                                 std::shared_ptr<ReaderMessageLogger> message_logger/* = nullptr*/)
{
    const std::string json_text = FileIO::ReadText(file_path);
    return CreateReader(std::move(file_path), json_text, std::move(message_logger));
}


std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(InterfaceString file_path,
                                                                 std::shared_ptr<ReaderMessageLogger> message_logger,
                                                                 const std::function<std::string()>& pre_80_spec_file_converter)
{
    const FileIO::FileAndSize file_and_size = FileIO::OpenFile(file_path);
    const size_t file_size = static_cast<size_t>(file_and_size.size);
    auto buffer = std::make_unique_for_overwrite<char[]>(file_size);
    TextEncoding text_encoding;

    auto read_file = [&](char* const buffer_pos, const size_t length)
    {
        if( fread(buffer_pos, 1, length, file_and_size.file) != length )
        {
            fclose(file_and_size.file);
            throw FileIO::Exception::FileReadError(file_path);
        }

        // calculate the BOM on the first read
        if( buffer_pos == buffer.get() )
            text_encoding.UpdateEncoding(buffer_pos, length);
    };

    // for a file to be an old spec file, it must be at least 4 bytes (3 bytes for the BOM, and
    // then the starting left bracket); even without a BOM, the file should have had at least
    // 4 bytes of header content)
    constexpr size_t InitialReadSize = TextEncoding::Utf8Bom_sv.length() + 1;

    if( file_size >= InitialReadSize )
    {
        read_file(buffer.get(), InitialReadSize);

        if( buffer[text_encoding.GetBomLength()] == Pre80SpecFileStartCharacter )
        {
            // convert the old spec file
            fclose(file_and_size.file);
            return CreateReader(std::move(file_path), pre_80_spec_file_converter(), std::move(message_logger));
        }

        // otherwise read the rest of the file
        read_file(buffer.get() + InitialReadSize, file_size - InitialReadSize);
    }

    else
    {
        // read the entire (small) file
        read_file(buffer.get(), file_size);
    }

    fclose(file_and_size.file);

    // if here, this should be a JSON spec file, so read as UTF-8
    std::string_view buffer_for_conversion_sv(buffer.get(), file_size);

    if( text_encoding.UsesBom() )
        buffer_for_conversion_sv.remove_prefix(text_encoding.GetBomLength());

    return CreateReader(std::move(file_path), buffer_for_conversion_sv, std::move(message_logger));
}


bool JsonSpecFile::IsPre80SpecFile(FileIO::FileAndSize& file_and_size)
{
    ASSERT(ftell(file_and_size.file) == 0 && file_and_size.size >= 0);

    constexpr size_t MaxReadSize = TextEncoding::Utf8Bom_sv.length() + 1;
    char buffer[MaxReadSize];

    const size_t actual_read_size = std::min(MaxReadSize, static_cast<size_t>(file_and_size.size));

    if( fread(buffer, 1, actual_read_size, file_and_size.file) == actual_read_size )
    {
        const TextEncoding text_encoding(buffer, actual_read_size);

        if( buffer[text_encoding.GetBomLength()] == Pre80SpecFileStartCharacter )
            return true;
    }

    return false;
}


bool JsonSpecFile::IsPre80SpecFile(const InterfaceString file_path)
{
    bool is_pre80_spec_file = false;

    try
    {
        FileIO::FileAndSize file_and_size = FileIO::OpenFile(file_path);

        is_pre80_spec_file = IsPre80SpecFile(file_and_size);

        fclose(file_and_size.file);
    }

    catch(...)
    {
        // ignore errors
    }

    return is_pre80_spec_file;
}
