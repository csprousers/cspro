#include "stdafx.h"
#include "StringBasedCaseConstructionReporter.h"
#include <zMessageO/SystemMessageIssuer.h>


class StringBasedCaseConstructionReporterSystemMessageIssuer : public SystemMessageIssuer
{
public:
    StringBasedCaseConstructionReporterSystemMessageIssuer(StringBasedCaseConstructionReporter& string_based_case_construction_reporter)
        :   m_stringBasedCaseConstructionReporter(string_based_case_construction_reporter)
    {
    }

    void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text) override
    {
        m_stringBasedCaseConstructionReporter.WriteString(SO::EmptyString, message_text);
    }

private:
    StringBasedCaseConstructionReporter& m_stringBasedCaseConstructionReporter;
};
    

void StringBasedCaseConstructionReporter::IssueMessage(MessageType message_type, int message_number, ...)
{
    if( m_systemMessageIssuer == nullptr )
        m_systemMessageIssuer = std::make_shared<StringBasedCaseConstructionReporterSystemMessageIssuer>(*this);

    va_list parg;
    va_start(parg, message_number);
    m_systemMessageIssuer->IssueVA(message_type, message_number, parg);
    va_end(parg);

    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::UnsupportedContentType(const CString& dictionary_name, const std::wstring& content_type_names)
{
    WriteString(CString(), FormatText(
        _T("The data source %s does not support these types: %s"),
        dictionary_name.GetString(), content_type_names.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BadRecordType(const Case& data_case, const CString& record_type, const CString& case_line)
{
    WriteString(data_case.GetKey(), FormatText(
        _T("Line ignored because of an invalid record type ('%s'): %s"),
        record_type.GetString(), case_line.GetString()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::TooManyRecordOccurrences(const Case& data_case,
    const CString& record_name, size_t maximum_occurrences)
{
    WriteString(data_case.GetKey(), FormatText(
        _T("There were more records than allowed for record %s. ")
        _T("Record ignored because it exceeded the maximum allowed (%d)."),
        record_name.GetString(), (int)maximum_occurrences));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BlankRecordAdded(const CString& key, const CString& record_name)
{
    WriteString(key, FormatText(
        _T("A blank record was added for the required record %s."),
        record_name.GetString()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::DuplicateUuid(const Case& data_case)
{
    WriteString(data_case.GetKey(),
        _T("This case has the same uuid as a case already in the output file. ")
        _T("Duplicate case preserved but uuid changed in the output file."));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BinaryDataIOError(const Case& data_case, bool read_error, const std::wstring& error)
{
    WriteString(data_case.GetKey(), FormatText(
        _T("There was an error %s binary data: %s"),
        read_error ? _T("reading") : _T("writing"),
        error.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::ContentTypeConversionError(const Case& data_case, const CString& item_name,
    ContentType input_content_type, ContentType output_content_type)
{
    WriteString(data_case.GetKey(), FormatText(
        _T("The value of %s with type %s could not be converted to type %s"),
        item_name.GetString(), ToString(input_content_type), ToString(output_content_type)));
    m_hadErrors = true;
}
