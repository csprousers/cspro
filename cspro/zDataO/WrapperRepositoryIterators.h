#pragma once

#include <zDataO/CaseIterator.h>


class WrapperRepositoryCaseIterator : public CaseIterator
{
public:
    WrapperRepositoryCaseIterator(std::shared_ptr<CaseIterator> case_iterator)
        :   m_caseIterator(case_iterator)
    {
    }

    bool NextCaseKey(CaseKey& case_key) override
    {
        return m_caseIterator->NextCaseKey(case_key);
    }

    bool NextCaseSummary(CaseSummary& case_summary) override
    {
        return m_caseIterator->NextCaseSummary(case_summary);
    }

    bool NextCase(Case& data_case) override
    {
        return m_caseIterator->NextCase(data_case);
    }

    int GetPercentRead() const override
    {
        return m_caseIterator->GetPercentRead();
    }

protected:
    std::shared_ptr<CaseIterator> m_caseIterator;
};
