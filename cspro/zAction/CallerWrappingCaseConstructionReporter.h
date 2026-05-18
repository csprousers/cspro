#pragma once

#include <zAction/Caller.h>
#include <zCaseO/CaseConstructionReporter.h>

namespace ActionInvoker { class CallerWrappingCaseConstructionReporter; }


class ActionInvoker::CallerWrappingCaseConstructionReporter : public CaseConstructionReporter
{
public:
    CallerWrappingCaseConstructionReporter(Runtime& runtime, std::shared_ptr<Caller*> current_caller);

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
    CaseConstructionReporter* GetCurrentCaseConstructionReporter();

private:
    Runtime& m_runtime;
    std::shared_ptr<Caller*> m_currentCaller;
    std::map<int, std::shared_ptr<CaseConstructionReporter>> m_caseConstructionReporters; // caller ID -> reporter
};
