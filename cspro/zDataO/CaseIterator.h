#pragma once
#include <zDataO/zDataO.h> // CR_TODO remove
class CaseKey;
class CaseSummary;
class Case;

class /* CR_TODO remove */ ZDATAO_API CaseIterator
{
protected:
    CaseIterator()
    {
    }

public:
    virtual ~CaseIterator() { }

    static bool RequiresCaseNote()
    {
#ifdef WIN_DESKTOP
        return false;
#else
        return true;
#endif
    }

    /// <summary>
    /// Loads the next CaseKey if one exists or returns false if at the end of the cases.
    /// </summary>
    virtual bool NextCaseKey(CaseKey& case_key) = 0;

    /// <summary>
    /// Loads the next CaseSummary if one exists or returns false if at the end of the cases.
    /// </summary>
    virtual bool NextCaseSummary(CaseSummary& case_summary) = 0;

    /// <summary>
    /// Loads the next Case if one exists or returns false if at the end of the cases.
    /// </summary>
    virtual bool NextCase(Case& data_case) = 0;

    /// <summary>
    /// Returns the percent of the cases in the repository that have been read. This
    /// method will only be called when reading cases and when not using any kind of filter.
    /// </summary>
    virtual int GetPercentRead() const = 0;
    
    bool NextCasetainer(Case& data_case); // CR_TODO remove
};
