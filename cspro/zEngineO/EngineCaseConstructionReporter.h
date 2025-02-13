#pragma once

#include <zCaseO/CaseConstructionReporter.h>
#include <zCaseO/Case.h>
#include <zUtilO/ProcessSummary.h>
#include <zMessageO/SystemMessageIssuer.h>


class EngineCaseConstructionReporter : public CaseConstructionReporter
{
public:
    EngineCaseConstructionReporter(std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer, std::shared_ptr<ProcessSummary> process_summary,
        std::optional<std::function<void(const Case&)>> update_case_callback = std::nullopt)
        :   CaseConstructionReporter(std::move(process_summary)),
            m_systemMessageIssuer(std::move(m_systemMessageIssuer)),
            m_updateCaseCallback(std::move(update_case_callback))
    {
    }

    void IssueMessage(MessageType message_type, int message_number, ...) override
    {
        va_list parg;
        va_start(parg, message_number);
        m_systemMessageIssuer->IssueVA(message_type, message_number, parg);
        va_end(parg);
    }

    void UnsupportedContentType(const CString& dictionary_name, const std::wstring& content_type_names) override
    {
        m_systemMessageIssuer->Issue(MessageType::Warning, 10107, dictionary_name.GetString(), content_type_names.c_str());
    }

    void BadRecordType(const Case& data_case, const CString& record_type, const CString& /*case_line*/) override
    {
        UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 10007,
                                     data_case.GetCaseMetadata().GetDictionary().GetName().GetString(), record_type.GetString());
    }

    void TooManyRecordOccurrences(const Case& data_case, const CString& record_name, size_t /*maximum_occurrences*/) override
    {
        UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 10006, record_name.GetString(), data_case.GetKey().GetString());
    }

    void BlankRecordAdded(const CString& key, const CString& record_name) override
    {
        // CR_TODO when Pre74_Case is removed, change the signature from CString->Case and then uncomment below
        // UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 10009, record_name.GetString(), key.GetString());
    }

    void DuplicateUuid(const Case& data_case) override
    {
        UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 100200, data_case.GetKey().GetString());
    }

    void BinaryDataIOError(const Case& data_case, bool read_error, const std::wstring& error) override
    {
        UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 10109, read_error ? _T("reading") : _T("writing"),
                                     data_case.GetCaseMetadata().GetDictionary().GetName().GetString(), error.c_str());
    }

    void ContentTypeConversionError(const Case& data_case, const CString& item_name,
                                    ContentType input_content_type, ContentType output_content_type) override
    {
        UpdateCase(data_case);
        m_systemMessageIssuer->Issue(MessageType::Warning, 10110, item_name.GetString(),
                                     ToString(input_content_type), ToString(output_content_type));
    }

private:
    void UpdateCase(const Case& data_case)
    {
        // because case construction errors will occur before the batch error key is set, change the key
        // before issuing the message
        if( m_updateCaseCallback.has_value() )
            (*m_updateCaseCallback)(data_case);
    }

private:
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;
    std::optional<std::function<void(const Case&)>> m_updateCaseCallback;
};
