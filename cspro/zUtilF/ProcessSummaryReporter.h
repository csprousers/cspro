#pragma once

#include <zUtilO/ProcessSummary.h>

class CancelFlag;


class ProcessSummaryReporter
{
public:
    virtual ~ProcessSummaryReporter() { }

    virtual void Initialize(InterfaceString title, std::shared_ptr<ProcessSummary> process_summary, CancelFlag* cancel_flag) = 0;

    virtual void SetSource(InterfaceString source_text) = 0;

    virtual void SetKey(const std::string& case_key) = 0;
};
