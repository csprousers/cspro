#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/WrapperRepository.h>


class ZDATAO_API CacheableCaseWrapperRepository : public WrapperRepository
{
    friend class CCWR_FirstPassCaseIterator;

private:
    CacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository);

public:
    static std::shared_ptr<DataRepository> CreateCacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository);

    // methods that can modify the cases are overridden
    ISyncableDataRepository* GetSyncableDataRepository() override;
    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;
    void ReadCase(Case& data_case, const std::string& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void ReadCaseByUuid(Case& data_case, const std::string& uuid) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    void DeleteCase(const std::string& key) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                 std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                                 const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

private:
    void ClearCachedIterations();
    void ClearCachedCases(bool reuse_cases);
    void ClearCachedCase(const Case& data_case);
    std::shared_ptr<Case> CacheCase(Case& data_case, bool cache_using_key);

    template<typename MapT, typename LookupT>
    void DeleteCaseWorker(MapT& cases_map, const LookupT& lookup_value);

private:
    const bool m_positionsInRepositoryChangeOnModification;

    std::vector<std::shared_ptr<Case>> m_unusedCasesPool;
    std::map<std::string, std::shared_ptr<Case>> m_casesByKey;
    std::map<double, std::shared_ptr<Case>> m_casesByPosition;

    std::map<size_t, const std::vector<std::shared_ptr<Case>>> m_cachedIterations;
};
