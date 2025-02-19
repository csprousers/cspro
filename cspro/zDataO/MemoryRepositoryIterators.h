#pragma once

#include <zDataO/CaseIterator.h>


class MemoryRepositoryCaseIterator : public CaseIterator
{
public:
    MemoryRepositoryCaseIterator(const std::vector<std::shared_ptr<Case>>& cases, std::vector<size_t> indices);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    template<typename T>
    bool Next(T& case_object);

private:
    const std::vector<std::shared_ptr<Case>>& m_cases;
    const std::vector<size_t> m_indices;
    std::vector<size_t>::const_iterator m_iterator;
    const double m_percentMultiplier;
};
