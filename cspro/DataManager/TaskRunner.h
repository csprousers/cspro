#pragma once

#include <zUtilF/LoggingListBox.h>


class TaskRunner
{
public:
    virtual ~TaskRunner() { }

    virtual std::shared_ptr<CaseConstructionReporter> GetCaseConstructionReporter(const CaseAccess& case_access) = 0;

    virtual void SetTitle(const std::string& title) = 0;

    virtual void LogText(SharableString text = SharableString()) = 0;

    template<typename... Args>
    void LogText(const char* formatter, Args const&... args);

    virtual void UpdateProgress(int percent) = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void TaskRunner::LogText(const char* const formatter, Args const&... args)
{
    LogText(FormatText(formatter, args...));
}
