#include "stdafx.h"
#include "DataRepository.h"
#include "CaseIterator.h"
#include "EncryptedSQLiteRepository.h"
#include "ExportWriterRepository.h"
#include "JsonRepository.h"
#include "MemoryRepository.h"
#include "NullRepository.h"
#include "SQLiteRepository.h"
#include "TextRepository.h"


DataRepository::DataRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
    :   m_type(type),
        m_caseAccess(std::move(case_access)),
        m_accessType(access_type)
{
}


DataRepository::~DataRepository()
{
}


void DataRepository::Open(const ConnectionString& connection_string, DataRepositoryOpenFlag open_flag)
{
    m_connectionString = connection_string;
    Open(open_flag);
}


CString DataRepository::GetName(DataRepositoryNameType name_type) const
{
    return WS2CS(m_connectionString.GetName(name_type));
}


std::unique_ptr<DataRepository> DataRepository::Create(std::shared_ptr<const CaseAccess> case_access,
                                                       const ConnectionString& connection_string,
                                                       DataRepositoryAccess access_type)
{
    ASSERT(case_access != nullptr && case_access->IsInitialized());

    DataRepositoryType type = connection_string.GetType();

    if( type == DataRepositoryType::Null )
    {
        return std::make_unique<NullRepository>(std::move(case_access), access_type);
    }

    else if( type == DataRepositoryType::Text )
    {
        return std::make_unique<TextRepository>(std::move(case_access), access_type);
    }

    else if( type == DataRepositoryType::SQLite )
    {
        return std::make_unique<SQLiteRepository>(std::move(case_access), access_type, GetDeviceId());
    }

    else if( type == DataRepositoryType::EncryptedSQLite )
    {
        return std::make_unique<EncryptedSQLiteRepository>(std::move(case_access), access_type, GetDeviceId());
    }

    else if( type == DataRepositoryType::Memory )
    {
        return std::make_unique<MemoryRepository>(std::move(case_access), access_type);
    }

    else if( type == DataRepositoryType::Json )
    {
        return std::make_unique<JsonRepository>(std::move(case_access), access_type);
    }

    else if( DataRepositoryHelpers::IsTypeExportWriter(type) )
    {
        return std::make_unique<ExportWriterRepository>(type, std::move(case_access), access_type);
    }

    else
    {
        throw DataRepositoryException::IOError(_T("Invalid repository type"));
    }
}


std::unique_ptr<DataRepository> DataRepository::CreateAndOpen(std::shared_ptr<const CaseAccess> case_access,
                                                              const ConnectionString& connection_string,
                                                              DataRepositoryAccess access_type, DataRepositoryOpenFlag open_flag)
{
    std::unique_ptr<DataRepository> repository = Create(std::move(case_access), connection_string, access_type);

    repository->Open(connection_string, open_flag);

    return repository;
}


void DataRepository::DeleteCase(const CString& key)
{
    CString modifiable_key = key;
    CString uuid;
    double position_in_repository;
    PopulateCaseIdentifiers(modifiable_key, uuid, position_in_repository);
    DeleteCase(position_in_repository, true);    
}


std::unique_ptr<CaseIterator> DataRepository::CreateCaseIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order)
{
    return CreateIterator(CaseIterationContent::Case, CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order);
}


std::unique_ptr<CaseIterator> DataRepository::CreateCaseKeyIterator(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order)
{
    return CreateIterator(CaseIterationContent::CaseKey, CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order);
}



// CR_TODO remove below
void DataRepository::ReadCasetainer(Case* pCasetainer, CString key)
{
    pCasetainer->m_recalculatePre74Case = true;
    ReadCase(*pCasetainer, key);
}

void DataRepository::ReadCasetainer(Case* pCasetainer, double position_in_repository)
{
    pCasetainer->m_recalculatePre74Case = true;
    ReadCase(*pCasetainer, position_in_repository);
}

void DataRepository::WriteCasetainer(Case* pCasetainer, WriteCaseParameter* write_case_parameter)
{
    pCasetainer->ApplyPre74_Case(pCasetainer->GetPre74_Case());
    WriteCase(*pCasetainer, write_case_parameter);
}


bool CaseIterator::NextCasetainer(Case& data_case) // CR_TODO remove
{
    data_case.m_recalculatePre74Case = true;
    return NextCase(data_case);
}
