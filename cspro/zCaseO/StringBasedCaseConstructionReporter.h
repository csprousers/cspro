#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseConstructionReporter.h>

class SystemMessageIssuer;


class ZCASEO_API StringBasedCaseConstructionReporter : public CaseConstructionReporter
{
    friend class StringBasedCaseConstructionReporterSystemMessageIssuer;

public:
    StringBasedCaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary = nullptr);
    ~StringBasedCaseConstructionReporter();

    bool HadErrors() const { return m_hadErrors; }

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

    virtual void WriteString(const std::string& key, std::string message) = 0;

private:
    bool m_hadErrors;
    std::unique_ptr<SystemMessageIssuer> m_systemMessageIssuer;
};
