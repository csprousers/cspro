#pragma once

#include <zDataO/CaseIterator.h>


class NullRepositoryCaseIterator : public CaseIterator
{
public:
    NullRepositoryCaseIterator()
    {
    }

    bool NextCaseKey(CaseKey& /*case_key*/) override
    {
        return false;
    }

    bool NextCaseSummary(CaseSummary& /*case_summary*/) override
    {
        return false;
    }

    bool NextCase(Case& /*data_case*/) override
    {
        return false;
    }

    int GetPercentRead() const override
    {
        return 100;
    }
};
