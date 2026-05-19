#include "stdafx.h"
#include "CallerWrappingCaseConstructionReporter.h"
#include <zCaseO/StringBasedCaseConstructionReporter.h>


// --------------------------------------------------------------------------
// LogDebuggingCaseConstructionReporter
//
// If a Caller doesn't override CreateCaseConstructionReporter, an instance
// of LogDebuggingCaseConstructionReporter will be created that will log
// messages using Listener::OnLogDebugMessage.
// --------------------------------------------------------------------------

class ActionInvoker::LogDebuggingCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
public:
    LogDebuggingCaseConstructionReporter(CallerWrappingCaseConstructionReporter& caller_wrapping_case_construction_reporter)
        :   m_callerWrappingCaseConstructionReporter(caller_wrapping_case_construction_reporter)
    {
    }

protected:
    void WriteString(const std::string& /*key*/, const std::string message) override
    {
        m_callerWrappingCaseConstructionReporter.LogDebugMessage(message);
    }

private:
    CallerWrappingCaseConstructionReporter m_callerWrappingCaseConstructionReporter;
};



// --------------------------------------------------------------------------
// CallerWrappingCaseConstructionReporter
// --------------------------------------------------------------------------

ActionInvoker::CallerWrappingCaseConstructionReporter::CallerWrappingCaseConstructionReporter(Runtime& runtime, std::shared_ptr<Caller*> current_caller)
    :   m_runtime(runtime),
        m_currentCaller(std::move(current_caller))
{
    ASSERT(m_currentCaller != nullptr);
}


CaseConstructionReporter* ActionInvoker::CallerWrappingCaseConstructionReporter::GetCurrentCaseConstructionReporter()
{
    // the Action Invoker should outlive any uses of this object, but check just in case
    if( *m_currentCaller == nullptr )
        return ReturnProgrammingError(nullptr);

    Caller& caller = *(*m_currentCaller);
    const int caller_id = caller.GetCallerId();
    auto lookup = m_caseConstructionReporters.find(caller_id);

    // if we do not already have a reporter for this caller, create one
    if( lookup == m_caseConstructionReporters.cend() )
    {
        std::shared_ptr<CaseConstructionReporter> case_construction_reporter = caller.CreateCaseConstructionReporter();

        // if the caller doesn't create one, default to a reporter that will use Listener::OnLogDebugMessage
        if( case_construction_reporter == nullptr )
            case_construction_reporter = std::make_unique<LogDebuggingCaseConstructionReporter>(*this);

        lookup = m_caseConstructionReporters.try_emplace(
            caller_id,
            std::move(case_construction_reporter)
        ).first;
    }

    return lookup->second.get();
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::UnsupportedContentType(
    const std::string& dictionary_name, const std::string& content_type_names)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->UnsupportedContentType(dictionary_name, content_type_names);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::BadRecordType(
    const Case& data_case, const std::string& record_type, const std::string& case_line)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->BadRecordType(data_case, record_type, case_line);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::TooManyRecordOccurrences(
    const Case& data_case, const std::string& record_name, const size_t maximum_occurrences)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->TooManyRecordOccurrences(data_case, record_name, maximum_occurrences);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::BlankRecordAdded(
    const std::string& key, const std::string& record_name)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->BlankRecordAdded(key, record_name);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::DuplicateUuid(const Case& data_case)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->DuplicateUuid(data_case);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::BinaryDataIOError(
    const Case& data_case, const bool read_error, const std::string& error)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->BinaryDataIOError(data_case, read_error, error);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::ContentTypeConversionError(
    const Case& data_case, const std::string& item_name,
    const ContentType input_content_type, const ContentType output_content_type)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->ContentTypeConversionError(data_case, item_name, input_content_type, output_content_type);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::OnIssueMessage(
    const MessageType message_type, const int message_number, va_list parg)
{
    CaseConstructionReporter* const case_construction_reporter = GetCurrentCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->OnIssueMessage(message_type, message_number, parg);
}


void ActionInvoker::CallerWrappingCaseConstructionReporter::LogDebugMessage(const std::string& message) const
{
    ASSERT(*m_currentCaller != nullptr);

    m_runtime.IterateOverListeners(*(*m_currentCaller),
        [&](Listener& listener)
        {
            listener.OnLogDebugMessage(message);
        });
}
