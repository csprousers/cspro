#include "stdafx.h"
#include "NullRepository.h"
#include "NullRepositoryIterators.h"


NullRepository::NullRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
    :   DataRepository(DataRepositoryType::Null, std::move(case_access), access_type)
{
}


NullRepository::~NullRepository()
{
}


void NullRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);
}


void NullRepository::Open(DataRepositoryOpenFlag /*open_flag*/)
{
}


void NullRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);

    m_accessType = ( m_accessType == DataRepositoryAccess::ReadOnly ) ? DataRepositoryAccess::ReadWrite :
                                                                        DataRepositoryAccess::ReadOnly;
}


void NullRepository::Close()
{
}


void NullRepository::DeleteRepository()
{
}


bool NullRepository::ContainsCase(const std::string& /*key*/)
{
    return false;
}


void NullRepository::PopulateCaseIdentifiers(std::string& /*key*/, std::string& /*uuid*/, double& /*position_in_repository*/)
{
    throw DataRepositoryException::CaseNotFound();
}


DataRepositoryUniqueCaseIdentifer NullRepository::GetUniqueCaseIdentifer(const CaseKey& /*case_key*/)
{
    throw DataRepositoryException::CaseNotFound();
}


std::optional<CaseKey> NullRepository::FindCaseKey(CaseIterationMethod /*iteration_method*/, CaseIterationOrder /*iteration_order*/,
                                                   const CaseIteratorParameters* /*start_parameters = nullptr*/)
{
    return std::nullopt;
}


void NullRepository::ReadCase(Case& /*data_case*/, const std::string& /*key*/)
{
    throw DataRepositoryException::CaseNotFound();
}


void NullRepository::ReadCase(Case& /*data_case*/, double /*position_in_repository*/)
{
    throw DataRepositoryException::CaseNotFound();
}


void NullRepository::ReadCaseByUuid(Case& /*data_case*/, const std::string& /*uuid*/)
{
    throw DataRepositoryException::CaseNotFound();
}


void NullRepository::WriteCase(Case& /*data_case*/, WriteCaseParameter* /*write_case_parameter = nullptr*/)
{
}


void NullRepository::DeleteCase(double /*position_in_repository*/, bool /*deleted = true*/)
{
    throw DataRepositoryException::CaseNotFound();
}


size_t NullRepository::GetNumberCases()
{
    return 0;
}


size_t NullRepository::GetNumberCases(CaseIterationCaseStatus /*case_status*/, const CaseIteratorParameters* /*start_parameters = nullptr*/)
{
    return 0;
}


std::unique_ptr<CaseIterator> NullRepository::CreateIterator(CaseIterationContent /*iteration_content*/,
                                                             const CaseIteratorSettings& /*iterator_settings*/,
                                                             size_t /*offset = 0*/, size_t /*limit = SIZE_MAX*/)
{
    return std::make_unique<NullRepositoryCaseIterator>();
}
