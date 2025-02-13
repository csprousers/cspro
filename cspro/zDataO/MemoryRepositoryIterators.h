#pragma once

#include <zDataO/CaseIterator.h>


class MemoryRepositoryCaseIterator : public CaseIterator
{
public:
    MemoryRepositoryCaseIterator(const std::vector<std::shared_ptr<Case>>& cases, std::vector<size_t> indices)
        :   m_cases(cases),
            m_indices(std::move(indices)),
            m_iterator(m_indices.cbegin())
    {
        m_percentMultiplier = 100.0 / std::max<size_t>(cases.size(), 1);
    }

private:
    template<typename T>
    bool Next(T& case_object)
    {
        if( m_iterator != m_indices.cend() )
        {
            case_object = *m_cases[*m_iterator];
            ++m_iterator;
            return true;
        }

        return false;
    }

public:
    bool NextCaseKey(CaseKey& case_key) override
    {
        return Next(case_key);
    }

    bool NextCaseSummary(CaseSummary& case_summary) override
    {
        return Next(case_summary);
    }

    bool NextCase(Case& data_case) override
    {
        return Next(data_case);
    }

    int GetPercentRead() const override
    {
        size_t cases_read = m_iterator - m_indices.cbegin();
        return (int)( cases_read * m_percentMultiplier );
    }

protected:
    const std::vector<std::shared_ptr<Case>>& m_cases;
    const std::vector<size_t> m_indices;
    std::vector<size_t>::const_iterator m_iterator;
    double m_percentMultiplier;
};
