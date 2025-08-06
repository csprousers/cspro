#pragma once

#include <zDataO/CaseIteratorSettings.h>

class Case;
class CaseKey;
class CaseSummary;


class CaseIterator
{
protected:
    CaseIterator() { }

public:
    virtual ~CaseIterator() { }

    static constexpr bool RequiresCaseNote() { return !OnWindowsDesktop(); }

    // Loads the next CaseKey if one exists or returns false if at the end of the cases.
    virtual bool NextCaseKey(CaseKey& case_key) = 0;

    // Loads the next CaseSummary if one exists or returns false if at the end of the cases.
    virtual bool NextCaseSummary(CaseSummary& case_summary) = 0;

    // Loads the next Case if one exists or returns false if at the end of the cases.
    virtual bool NextCase(Case& data_case) = 0;

    // Returns the percent of the cases in the repository that have been read. This
    // method will only be called when reading cases and when not using any kind of filter.
    virtual int GetPercentRead() const = 0;
};
