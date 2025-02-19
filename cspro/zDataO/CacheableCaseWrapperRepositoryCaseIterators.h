#pragma once

#include <zDataO/WrapperRepositoryIterators.h>


class CCWR_FirstPassCaseIterator : public WrapperRepositoryCaseIterator
{
public:
    CCWR_FirstPassCaseIterator(CacheableCaseWrapperRepository& cacheable_case_wrapper_repository,
                               size_t iteration_hash_value, std::unique_ptr<CaseIterator> case_iterator);

    bool NextCase(Case& data_case) override;

private:
    CacheableCaseWrapperRepository& m_cacheableCaseWrapperRepository;
    const size_t m_iterationHashValue;
    std::vector<std::shared_ptr<Case>> m_cachedCases;
};


class CCWR_SecondPassCaseIterator : public CaseIterator
{
public:
    CCWR_SecondPassCaseIterator(const std::vector<std::shared_ptr<Case>>& cases);

    bool NextCaseKey(CaseKey& case_key) override;
    bool NextCaseSummary(CaseSummary& case_summary) override;
    bool NextCase(Case& data_case) override;
    int GetPercentRead() const override;

private:
    template<typename T>
    bool Next(T& case_object);

private:
    const std::vector<std::shared_ptr<Case>>& m_cases;
    std::vector<std::shared_ptr<Case>>::const_iterator m_iterator;
    const double m_percentMultiplier;
};
