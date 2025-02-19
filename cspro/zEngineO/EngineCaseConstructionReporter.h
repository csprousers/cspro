#pragma once

#include <zCaseO/CaseConstructionReporter.h>
#include <zCaseO/Case.h>
#include <zUtilO/ProcessSummary.h>
#include <zMessageO/SystemMessageIssuer.h>


class EngineCaseConstructionReporter : public CaseConstructionReporter
{
public:
    EngineCaseConstructionReporter(std::shared_ptr<SystemMessageIssuer> system_message_issuer, std::shared_ptr<ProcessSummary> process_summary,
                                   std::function<void(const Case&)> update_case_callback = std::function<void(const Case&)>());

    void UnsupportedContentType(const std::string& dictionary_name, const std::string& content_type_names) override;
    void BadRecordType(const Case& data_case, const std::string& record_type, const std::string& case_line) override;
    void TooManyRecordOccurrences(const Case& data_case, const std::string& record_name, size_t maximum_occurrences) override;
    void BlankRecordAdded(const std::string& key, const std::string& record_name) override;
    void DuplicateUuid(const Case& data_case) override;
    void BinaryDataIOError(const Case& data_case, bool read_error, const std::string& error) override;
    void ContentTypeConversionError(const Case& data_case, const std::string& item_name,
                                    ContentType input_content_type, ContentType output_content_type) override;

protected:
    void OnIssueMessage(MessageType message_type, int message_number, va_list parg) override;

private:
    void UpdateCase(const Case& data_case);

private:
    const std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;
    const std::function<void(const Case&)> m_updateCaseCallback;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline EngineCaseConstructionReporter::EngineCaseConstructionReporter(std::shared_ptr<SystemMessageIssuer> system_message_issuer, std::shared_ptr<ProcessSummary> process_summary,
                                                                      std::function<void(const Case&)> update_case_callback/* = std::function<void(const Case&)>()*/)
    :   CaseConstructionReporter(std::move(process_summary)),
        m_systemMessageIssuer(std::move(system_message_issuer)),
        m_updateCaseCallback(std::move(update_case_callback))
{
    ASSERT(m_systemMessageIssuer != nullptr);
}


inline void EngineCaseConstructionReporter::OnIssueMessage(const MessageType message_type, const int message_number, va_list parg)
{
    m_systemMessageIssuer->IssueVA(message_type, message_number, parg);
}


inline void EngineCaseConstructionReporter::UnsupportedContentType(const std::string& dictionary_name, const std::string& content_type_names)
{
    m_systemMessageIssuer->Issue(MessageType::Warning, 10107,
                                 dictionary_name.c_str(),
                                 content_type_names.c_str());
}


inline void EngineCaseConstructionReporter::BadRecordType(const Case& data_case, const std::string& record_type, const std::string& /*case_line*/)
{
    UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 10007,
                                 data_case.GetCaseMetadata().GetDictionary().GetName().c_str(),
                                 record_type.c_str());
}


inline void EngineCaseConstructionReporter::TooManyRecordOccurrences(const Case& data_case, const std::string& record_name, size_t /*maximum_occurrences*/)
{
    UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 10006,
                                 record_name.c_str(),
                                 data_case.GetKey().c_str());
}


inline void EngineCaseConstructionReporter::BlankRecordAdded(const std::string& key, const std::string& record_name)
{
    // CR_TODO when Pre74_Case is removed, change the signature from CString->Case and then uncomment below
    // UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 10009,
                                 record_name.c_str(),
                                 key.c_str());
}


inline void EngineCaseConstructionReporter::DuplicateUuid(const Case& data_case)
{
    UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 100200,
                                 data_case.GetKey().c_str());
}


inline void EngineCaseConstructionReporter::BinaryDataIOError(const Case& data_case, const bool read_error, const std::string& error)
{
    UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 10109,
                                 read_error ? "reading" : "writing",
                                 data_case.GetCaseMetadata().GetDictionary().GetName().c_str(),
                                 error.c_str());
}


inline void EngineCaseConstructionReporter::ContentTypeConversionError(const Case& data_case, const std::string& item_name,
                                                                       const ContentType input_content_type, const ContentType output_content_type)
{
    UpdateCase(data_case);
    m_systemMessageIssuer->Issue(MessageType::Warning, 10110,
                                 item_name.c_str(),
                                 ToString(input_content_type),
                                 ToString(output_content_type));
}


inline void EngineCaseConstructionReporter::UpdateCase(const Case& data_case)
{
    // because case construction errors will occur before the batch error key is set, change the key
    // before issuing the message
    if( m_updateCaseCallback )
        m_updateCaseCallback(data_case);
}
