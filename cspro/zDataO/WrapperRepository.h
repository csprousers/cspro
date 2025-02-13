#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>
#include <zDataO/WrapperRepositoryIterators.h>


class WrapperRepository : public DataRepository
{
protected:
    WrapperRepository(std::shared_ptr<DataRepository> repository, DataRepositoryAccess access_type)
        :   DataRepository(repository->GetRepositoryType(), repository->GetSharedCaseAccess(), access_type),
            m_repository(std::move(repository))
    {
    }

public:
    const DataRepository& GetRealRepository() const override
    {
        return m_repository->GetRealRepository();
    }

    DataRepository& GetRealRepository() override
    {
        return m_repository->GetRealRepository();
    }

    const ISyncableDataRepository* GetSyncableDataRepository() const override
    {
        return m_repository->GetSyncableDataRepository();
    }

    ISyncableDataRepository* GetSyncableDataRepository() override
    {
        return m_repository->GetSyncableDataRepository();
    }

    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override
    {
        m_repository->ModifyCaseAccess(case_access);
        m_caseAccess = std::move(case_access); // so that non-virtual getter works
    }

    void Close() override
    {
        m_repository->Close();
    }

    void DeleteRepository() override
    {
        m_repository->DeleteRepository();
    }

    bool ContainsCase(const CString& key) const override
    {
        return m_repository->ContainsCase(key);
    }

    void PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const override
    {
        return m_repository->PopulateCaseIdentifiers(key, uuid, position_in_repository);
    }

    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
	    const CaseIteratorParameters* start_parameters = nullptr) const override
    {
        return m_repository->FindCaseKey(iteration_method, iteration_order, start_parameters);
    }

    void ReadCase(Case& data_case, const CString& key) override
    {
        m_repository->ReadCase(data_case, key);
    }

    void ReadCase(Case& data_case, double position_in_repository) override
    {
        m_repository->ReadCase(data_case, position_in_repository);
    }

    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override
    {
        m_repository->WriteCase(data_case, write_case_parameter);
    }

    void DeleteCase(double position_in_repository, bool deleted = true) override
    {
        m_repository->DeleteCase(position_in_repository, deleted);
    }

    size_t GetNumberCases() const override
    {
        return m_repository->GetNumberCases();
    }

    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) const override
    {
        return m_repository->GetNumberCases(case_status, start_parameters);
    }

    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
        std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
        const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override
    {
        return std::make_unique<WrapperRepositoryCaseIterator>(
            m_repository->CreateIterator(iteration_content, case_status, iteration_method, iteration_order, start_parameters, offset, limit));
    }

    void StartTransaction() override
    {
        m_repository->StartTransaction();
    }

    void EndTransaction() override
    {
        m_repository->EndTransaction();
    }

protected:
    void Open(DataRepositoryOpenFlag open_flag) override
    {
        m_repository->Open(m_connectionString, open_flag);
    }

protected:
    std::shared_ptr<DataRepository> m_repository;
};
