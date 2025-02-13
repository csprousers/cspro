#pragma once

#include <zUtilO/ProcessSummary.h>


class ProcessSummaryReporter
{
public:
    virtual ~ProcessSummaryReporter() { }

    virtual void Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary, bool* cancel_flag) = 0;

    virtual void SetSource(const CString& source_text) = 0;

    virtual void SetKey(const CString& case_key) = 0;
};
