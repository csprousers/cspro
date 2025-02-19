#include "stdafx.h"
#include "WrapperRepository.h"
#include "WrapperRepositoryIterators.h"
#include <zDataO/CaseIterator.h>


// --------------------------------------------------------------------------
// WrapperRepository
// --------------------------------------------------------------------------

WrapperRepository::WrapperRepository(cs::non_null_shared_or_raw_ptr<DataRepository> repository)
    :   DataRepository(repository->GetRepositoryType(), repository->GetSharedCaseAccess(), repository->GetRepositoryAccess()),
        m_repository(std::move(repository))
{
}


void WrapperRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    m_repository->Open(m_connectionString, open_flag);
}


DataRepository& WrapperRepository::GetRealRepository()
{
    return m_repository->GetRealRepository();
}


ISyncableDataRepository* WrapperRepository::GetSyncableDataRepository()
{
    return m_repository->GetSyncableDataRepository();
}


void WrapperRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_repository->ModifyCaseAccess(case_access);
    m_caseAccess = std::move(case_access); // so that non-virtual getter works
}


void WrapperRepository::ToggleReadWriteMode()
{
    m_repository->ToggleReadWriteMode();
}


void WrapperRepository::Close()
{
    m_repository->Close();
}


void WrapperRepository::DeleteRepository()
{
    m_repository->DeleteRepository();
}


bool WrapperRepository::ContainsCase(const std::string& key)
{
    return m_repository->ContainsCase(key);
}


void WrapperRepository::PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository)
{
    return m_repository->PopulateCaseIdentifiers(key, uuid, position_in_repository);
}


DataRepositoryUniqueCaseIdentifer WrapperRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    return m_repository->GetUniqueCaseIdentifer(case_key);
}


std::optional<CaseKey> WrapperRepository::FindCaseKey(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order,
                                                      const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    return m_repository->FindCaseKey(iteration_method, iteration_order, start_parameters);
}


void WrapperRepository::ReadCase(Case& data_case, const std::string& key)
{
    m_repository->ReadCase(data_case, key);
}


void WrapperRepository::ReadCase(Case& data_case, const double position_in_repository)
{
    m_repository->ReadCase(data_case, position_in_repository);
}


void WrapperRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    m_repository->ReadCaseByUuid(data_case, uuid);
}


void WrapperRepository::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    m_repository->WriteCase(data_case, write_case_parameter);
}


void WrapperRepository::DeleteCase(const double position_in_repository, const bool deleted/* = true*/)
{
    m_repository->DeleteCase(position_in_repository, deleted);
}


void WrapperRepository::DeleteCase(const std::string& key)
{
    m_repository->DeleteCase(key);
}


size_t WrapperRepository::GetNumberCases()
{
    return m_repository->GetNumberCases();
}


size_t WrapperRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    return m_repository->GetNumberCases(case_status, start_parameters);
}


std::unique_ptr<CaseIterator> WrapperRepository::CreateIterator(const CaseIterationContent iteration_content, const CaseIterationCaseStatus case_status,
                                                                const std::optional<CaseIterationMethod> iteration_method, const std::optional<CaseIterationOrder> iteration_order,
                                                                const CaseIteratorParameters* const start_parameters/* = nullptr*/, const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    return m_repository->CreateIterator(iteration_content, case_status, iteration_method, iteration_order, start_parameters, offset, limit);
}


void WrapperRepository::StartTransaction()
{
    m_repository->StartTransaction();
}


void WrapperRepository::EndTransaction()
{
    m_repository->EndTransaction();
}


// --------------------------------------------------------------------------
// WrapperRepositoryCaseIterator
// --------------------------------------------------------------------------

WrapperRepositoryCaseIterator::WrapperRepositoryCaseIterator(std::unique_ptr<CaseIterator> case_iterator)
    :   m_caseIterator(std::move(case_iterator))
{
    ASSERT(m_caseIterator != nullptr);
}


bool WrapperRepositoryCaseIterator::NextCaseKey(CaseKey& case_key)
{
    return m_caseIterator->NextCaseKey(case_key);
}


bool WrapperRepositoryCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    return m_caseIterator->NextCaseSummary(case_summary);
}


bool WrapperRepositoryCaseIterator::NextCase(Case& data_case)
{
    return m_caseIterator->NextCase(data_case);
}


int WrapperRepositoryCaseIterator::GetPercentRead() const
{
    return m_caseIterator->GetPercentRead();
}
