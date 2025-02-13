#include "stdafx.h"
#include "JsonSpecFile.h"
#include <zUtilO/Versioning.h>


// --------------------------------------------------------------------------
// writing spec files creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonFileWriter> JsonSpecFile::CreateWriter(NullTerminatedString filename, wstring_view file_type)
{
    auto json_writer = Json::CreateFileWriter(filename);

    json_writer->BeginObject();

    WriteHeading(*json_writer, file_type);

    return json_writer;
}


void JsonSpecFile::WriteHeading(JsonWriter& json_writer, wstring_view file_type)
{
    json_writer.Write(JK::software, _T("CSPro"))
               .Write(JK::version, CSPRO_VERSION_NUMBER)
               .Write(JK::fileType, file_type);
}



// --------------------------------------------------------------------------
// JsonSpecFile::ReaderMessageLogger
// --------------------------------------------------------------------------

void JsonSpecFile::ReaderMessageLogger::LogWarning(const std::wstring& filename, std::wstring message)
{
    auto message_lookup = std::find_if(m_messageSets.begin(), m_messageSets.end(),
                                       [&](const auto& m) { return SO::EqualsNoCase(std::get<0>(m), filename); });

    auto& message_set = ( message_lookup == m_messageSets.end() ) ? m_messageSets.emplace_back(filename, std::vector<std::wstring>()) :
                                                                    *message_lookup;
    std::get<1>(message_set).emplace_back(std::move(message));
}


std::wstring JsonSpecFile::ReaderMessageLogger::GetErrorText(std::wstring initial_text) const
{
    ASSERT(!m_messageSets.empty());

    std::wstring& combined_message_text = initial_text;

    for( const auto& [filename, messages] : m_messageSets )
    {
        if( m_messageSets.size() == 1 )
        {
            SO::AppendFormat(combined_message_text, _T(" were problems reading '%s':\n"), PortableFunctions::PathGetFilename(filename));
        }

        else
        {
            if( combined_message_text.empty() )
                combined_message_text.append(_T(" were problems reading multiple files:"));

            const TCHAR* filename_only = PortableFunctions::PathGetFilename(filename);

            SO::AppendFormat(combined_message_text, _T("\n\n%s\n%s\n"), filename_only, SO::GetDashedLine(_tcslen(filename_only)));
        }

        for( const std::wstring& message : messages )
            SO::Append(combined_message_text, _T("\n  • "), message);
    }

    return combined_message_text;
}


void JsonSpecFile::ReaderMessageLogger::DisplayWarnings(bool silent/* = false*/) const
{
    if( !silent && !m_messageSets.empty() )
        ErrorMessage::Display(GetErrorText(_T("There")));
}


void JsonSpecFile::ReaderMessageLogger::RethrowException(NullTerminatedString filename, const CSProException& exception) const
{
    std::wstring message = FormatTextCS2WS(_T("There was an error reading '%s':\n\n  ⚠️ %s"),
                                           PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());

    if( !m_messageSets.empty() )
        message.append(GetErrorText(_T("\n\nIn addition, there")));

    throw CSProException(message);
}



// --------------------------------------------------------------------------
// JsonSpecFile::Reader
// --------------------------------------------------------------------------

JsonSpecFile::Reader::Reader(std::basic_string_view<TCHAR> json_text, std::wstring filename, std::shared_ptr<ReaderMessageLogger> message_logger)
    :   JsonNode<TCHAR>(json_text, this),
        JsonReaderInterface(PortableFunctions::PathGetDirectory(filename)),
        m_filename(std::move(filename)),
        m_messageLogger(message_logger)
{
    ASSERT(m_messageLogger != nullptr);
}


double JsonSpecFile::Reader::CheckVersion()
{
    double version = GetOrDefault(JK::version, CSPRO_VERSION_NUMBER);

    if( version > CSPRO_VERSION_NUMBER )
        LogWarning(_T("The file was created using CSPro %0.1f and may use features not supported by this version (CSPro %0.1f)"), version, CSPRO_VERSION_NUMBER);

    return version;
}


void JsonSpecFile::Reader::CheckFileType(wstring_view file_type)
{
    std::optional<std::wstring> file_type_json = GetOptional<wstring_view>(JK::fileType);

    if( file_type_json.has_value() && file_type_json != file_type )
        throw CSProException(_T("The file type '%s' cannot be read by this program"), file_type_json->c_str());


#ifdef WIN_DESKTOP
    if( !GetOrDefault(JK::editable, true) )
    {
        auto exe_name = std::make_unique<TCHAR[]>(_MAX_PATH);
        GetModuleFileName(AfxGetApp()->m_hInstance, exe_name.get(), _MAX_PATH);

        if( SO::EqualsNoCase(PortableFunctions::PathGetFilenameWithoutExtension(exe_name.get()), _T("CSPro")) )
            throw CSProException("This file has been locked and cannot be edited using the CSPro Designer");
    }
#endif
}



// --------------------------------------------------------------------------
// reading spec file creation functions
// --------------------------------------------------------------------------

std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(NullTerminatedString filename,
                                                                 wstring_view json_text,
                                                                 std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger)
{
    if( message_logger == nullptr )
        message_logger = std::make_shared<JsonSpecFile::ReaderMessageLogger>();

    try
    {
        return std::make_unique<JsonSpecFile::Reader>(json_text, filename, std::move(message_logger));
    }

    catch( const JsonParseException& exception )
    {
        throw CSProException(_T("There was an error reading '%s' as it contains invalid JSON:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }
}


std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(NullTerminatedString filename,
                                                                 std::shared_ptr<ReaderMessageLogger> message_logger/* = nullptr*/)
{
    return CreateReader(filename, FileIO::ReadText(filename), std::move(message_logger));
}


std::unique_ptr<JsonSpecFile::Reader> JsonSpecFile::CreateReader(NullTerminatedString filename,
                                                                 std::shared_ptr<ReaderMessageLogger> message_logger,
                                                                 const std::function<std::wstring()>& pre_80_spec_file_converter)
{
    FileIO::FileAndSize file_and_size = FileIO::OpenFile(filename);
    size_t file_size = (size_t)file_and_size.size;
    auto buffer = std::make_unique<char[]>(file_size);
    bool file_has_bom;

    auto read_file = [&](char* buffer_pos, size_t length)
    {
        if( fread(buffer_pos, 1, length, file_and_size.file) != length )
        {
            fclose(file_and_size.file);
            throw FileIO::Exception(_T("The file '%s' could not be fully read."), PortableFunctions::PathGetFilename(filename));
        }

        // calculate the BOM on the first read
        if( buffer_pos == buffer.get() )
            file_has_bom = HasUtf8BOM(buffer_pos, length);
    };

    // for a file to be an old spec file, it must be at least 4 bytes (3 bytes for the BOM, and
    // then the starting left bracket); even without a BOM, the file should have had at least
    // 4 bytes of header content)
    constexpr size_t InitialReadSize = Utf8BOM_sv.length() + 1;

    if( file_size >= InitialReadSize )
    {
        read_file(buffer.get(), InitialReadSize);

        if( buffer[file_has_bom ? Utf8BOM_sv.length() : 0] == Pre80SpecFileStartCharacter )
        {
            // convert the old spec file
            fclose(file_and_size.file);
            return CreateReader(filename, pre_80_spec_file_converter(), std::move(message_logger));
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

    // if here, this should be a JSON spec file, so convert from UTF-8
    const char* buffer_for_conversion = buffer.get();

    if( file_has_bom )
    {
        buffer_for_conversion += Utf8BOM_sv.length();
        file_size -= Utf8BOM_sv.length();
    }

    return CreateReader(filename, UTF8Convert::UTF8ToWide(buffer_for_conversion, file_size), std::move(message_logger));
}


bool JsonSpecFile::IsPre80SpecFile(FileIO::FileAndSize& file_and_size)
{
    ASSERT(ftell(file_and_size.file) == 0 && file_and_size.size >= 0);

    constexpr size_t MaxReadSize = Utf8BOM_sv.length() + 1;
    char buffer[MaxReadSize];

    size_t actual_read_size = std::min(MaxReadSize, (size_t)file_and_size.size);

    if( fread(buffer, 1, actual_read_size, file_and_size.file) == actual_read_size )
    {
        if( buffer[HasUtf8BOM(buffer, actual_read_size) ? Utf8BOM_sv.length() : 0] == Pre80SpecFileStartCharacter )
            return true;
    }

    return false;
}


bool JsonSpecFile::IsPre80SpecFile(NullTerminatedString filename)
{
    bool is_pre80_sec_file = false;

    try
    {
        FileIO::FileAndSize file_and_size = FileIO::OpenFile(filename);

        is_pre80_sec_file = IsPre80SpecFile(file_and_size);

        fclose(file_and_size.file);
    }

    catch(...)
    {
        // ignore errors
    }

    return is_pre80_sec_file;
}
