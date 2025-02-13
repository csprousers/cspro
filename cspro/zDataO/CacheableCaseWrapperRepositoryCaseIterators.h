#pragma once

#include <zDataO/WrapperRepositoryIterators.h>


class CacheableCaseWrapperRepositoryFirstPassCaseIterator : public WrapperRepositoryCaseIterator
{
public:
    CacheableCaseWrapperRepositoryFirstPassCaseIterator(CacheableCaseWrapperRepository* cacheable_case_wrapper_repository,
        size_t iteration_hash_value, std::shared_ptr<CaseIterator> case_iterator)
        :   WrapperRepositoryCaseIterator(case_iterator),
            m_cacheableCaseWrapperRepository(cacheable_case_wrapper_repository),
            m_iterationHashValue(iteration_hash_value)
    {
    }

    bool NextCase(Case& data_case) override
    {
        bool case_read = WrapperRepositoryCaseIterator::NextCase(data_case);

        if( case_read )
            m_cachedCases.push_back(m_cacheableCaseWrapperRepository->CacheCase(data_case, false));

        // when all cases have been read, the iteration order can be saved for future use
        else
            m_cacheableCaseWrapperRepository->m_cachedIterations.emplace(m_iterationHashValue, m_cachedCases);

        return case_read;
    }

private:
    CacheableCaseWrapperRepository* m_cacheableCaseWrapperRepository;
    size_t m_iterationHashValue;
    std::vector<std::shared_ptr<Case>> m_cachedCases;
};


class CacheableCaseWrapperRepositorySecondPassCaseIterator : public CaseIterator
{
public:
    CacheableCaseWrapperRepositorySecondPassCaseIterator(const std::vector<std::shared_ptr<Case>>& cases)
        :   m_cases(cases),
            m_iterator(m_cases.cbegin())
    {
        m_percentMultiplier = 100.0 / std::max(cases.size(), (size_t)1);
    }

private:
    template<typename T>
    bool Next(T& case_object)
    {
        if( m_iterator != m_cases.cend() )
        {
            case_object = *(*m_iterator);
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
        size_t cases_read = m_iterator - m_cases.cbegin();
        return (int)( cases_read * m_percentMultiplier );
    }

protected:
    const std::vector<std::shared_ptr<Case>>& m_cases;
    std::vector<std::shared_ptr<Case>>::const_iterator m_iterator;
    double m_percentMultiplier;
};
