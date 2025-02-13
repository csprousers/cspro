#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseConstructionReporter.h>

class SystemMessageIssuer;


class ZCASEO_API StringBasedCaseConstructionReporter : public CaseConstructionReporter
{
    friend class StringBasedCaseConstructionReporterSystemMessageIssuer;

public:
    StringBasedCaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary = nullptr)
        :   CaseConstructionReporter(process_summary),
            m_hadErrors(false)
    {
    }

    bool HadErrors() const { return m_hadErrors; }

    void IssueMessage(MessageType message_type, int message_number, ...) override;

    void UnsupportedContentType(const CString& dictionary_name, const std::wstring& content_type_names) override;

    void BadRecordType(const Case& data_case, const CString& record_type, const CString& case_line) override;

    void TooManyRecordOccurrences(const Case& data_case, const CString& record_name, size_t maximum_occurrences) override;

    void BlankRecordAdded(const CString& key, const CString& record_name) override;

    void DuplicateUuid(const Case& data_case) override;

    void BinaryDataIOError(const Case& data_case, bool read_error, const std::wstring& error) override;

    void ContentTypeConversionError(const Case& data_case, const CString& item_name,
        ContentType input_content_type, ContentType output_content_type) override;

protected:
    virtual void WriteString(NullTerminatedString key, NullTerminatedString message) = 0;

private:
    bool m_hadErrors;
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;
};
