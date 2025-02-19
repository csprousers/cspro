#pragma once

#include <zDataO/CaseIterator.h>


class WrapperRepositoryCaseIterator : public CaseIterator
{
public:
    WrapperRepositoryCaseIterator(std::unique_ptr<CaseIterator> case_iterator);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    std::unique_ptr<CaseIterator> m_caseIterator;
};
