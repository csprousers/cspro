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

    void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::string& message_text) override
    {
        WriteString(message_text);
    }

    void OnIssue(const Logic::ParserMessage& parser_message) override
    {
        ASSERT(false);
        WriteString(parser_message.message_text);
    }

    void OnAbort(const std::string& message_text) override
    {
        ASSERT(false);
        WriteString(message_text);
    }

public:
    void WriteString(const std::string& message_text)
    {
        m_stringBasedCaseConstructionReporter.WriteString(SO::Empty_string, message_text);
    }

private:
    StringBasedCaseConstructionReporter& m_stringBasedCaseConstructionReporter;
};



StringBasedCaseConstructionReporter::StringBasedCaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary/* = nullptr*/)
    :   CaseConstructionReporter(std::move(process_summary)),
        m_hadErrors(false)
{
}


StringBasedCaseConstructionReporter::~StringBasedCaseConstructionReporter()
{
}


void StringBasedCaseConstructionReporter::OnIssueMessage(const MessageType message_type, const int message_number, va_list parg)
{
    if( m_systemMessageIssuer == nullptr )
        m_systemMessageIssuer = std::make_unique<StringBasedCaseConstructionReporterSystemMessageIssuer>(*this);

    m_systemMessageIssuer->IssueVA(message_type, message_number, parg);

    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::UnsupportedContentType(const std::string& dictionary_name, const std::string& content_type_names)
{
    WriteString(SO::Empty_string, FormatText("The data source %s does not support these types: %s",
                                             dictionary_name.c_str(),
                                             content_type_names.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BadRecordType(const Case& data_case, const std::string& record_type, const std::string& case_line)
{
    WriteString(data_case.GetKey(), FormatText("Line ignored because of an invalid record type ('%s'): %s",
                                               record_type.c_str(),
                                               case_line.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::TooManyRecordOccurrences(const Case& data_case, const std::string& record_name, const size_t maximum_occurrences)
{
    WriteString(data_case.GetKey(), FormatText("There were more records than allowed for record %s. "
                                               "Record ignored because it exceeded the maximum allowed (%d).",
                                               record_name.c_str(),
                                               static_cast<int>(maximum_occurrences)));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BlankRecordAdded(const std::string& key, const std::string& record_name)
{
    WriteString(key, FormatText("A blank record was added for the required record %s.",
                                record_name.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::DuplicateUuid(const Case& data_case)
{
    WriteString(data_case.GetKey(), "This case has the same uuid as a case already in the output file. "
                                    "Duplicate case preserved but uuid changed in the output file.");
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::BinaryDataIOError(const Case& data_case, const bool read_error, const std::string& error)
{
    WriteString(data_case.GetKey(), FormatText("There was an error %s binary data: %s",
                                               read_error ? "reading" : "writing",
                                               error.c_str()));
    m_hadErrors = true;
}


void StringBasedCaseConstructionReporter::ContentTypeConversionError(const Case& data_case, const std::string& item_name,
                                                                     const ContentType input_content_type, const ContentType output_content_type)
{
    WriteString(data_case.GetKey(), FormatText("The value of %s with type %s could not be converted to type %s",
                                               item_name.c_str(),
                                               ToString(input_content_type),
                                               ToString(output_content_type)));
    m_hadErrors = true;
}
