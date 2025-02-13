#include "stdafx.h"
#include "EncodedTextWriter.h"


EncodedTextWriter::EncodedTextWriter(const DataRepositoryType type, const CaseAccess& case_access,
                                     const std::wstring& filename, const ConnectionString& connection_string)
{
    m_ansi = connection_string.HasProperty(CSProperty::encoding, CSValue::ANSI);

#ifdef WIN32
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(case_access);
#else
    if( m_ansi )
    {
        m_ansi = false;

        if( case_access.GetCaseConstructionReporter() != nullptr )
        {
            case_access.GetCaseConstructionReporter()->IssueMessage(MessageType::Warning, 31102, ToString(type),
                                                                    _T("ANSI mode will be ignored on Android and the file will be written using UTF-8"));
        }
    }
#endif

    // open the file
    SetupEnvironmentToCreateFile(filename);

    m_file = PortableFunctions::FileOpen(filename, _T("wb"));

    if( m_file == nullptr )
        throw CSProException(_T("Could not create the text file: ") + filename);

    // write the UTF-8 BOM if necessary
    if( !m_ansi && !connection_string.HasProperty(CSProperty::encoding, CSValue::UTF_8) )
        fwrite(Utf8BOM_sv.data(), 1, Utf8BOM_sv.length(), m_file);
}


EncodedTextWriter::~EncodedTextWriter()
{
    fclose(m_file);
}


void EncodedTextWriter::WriteLine(const wstring_view line_sv)
{
    // one wide character can map to four UTF-8 characters
    const size_t output_length_needed = m_ansi ? line_sv.length() :
                                                 ( line_sv.length() * 4 );

    // if necessary, resize the output buffer (to more than necessary to minimize these allocations)
    if( m_outputBuffer.size() < output_length_needed )
        m_outputBuffer.resize(output_length_needed * 2);

    char* output_buffer = m_outputBuffer.data();
    size_t output_length;

#ifdef WIN32
    if( m_ansi )
    {
        output_length = WideCharToMultiByte(CP_ACP, 0, line_sv.data(), line_sv.length(), output_buffer, m_outputBuffer.size(), nullptr, nullptr);
        ASSERT(output_length <= output_length_needed);
    }

    else
#endif
    {
        output_length = UTF8Convert::WideBufferToUTF8Buffer(line_sv.data(), line_sv.length(), output_buffer, m_outputBuffer.size());
    }

    fwrite(output_buffer, output_length, 1, m_file);

    // write the newline
    fwrite("\r\n", 2, 1, m_file);
}
