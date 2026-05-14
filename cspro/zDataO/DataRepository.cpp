#include "stdafx.h"
#include "DataRepository.h"
#include "CaseIterator.h"
#include "CSWebRepository.h"
#include "EncryptedSQLiteRepository.h"
#include "ExportWriterRepository.h"
#include "JsonRepository.h"
#include "MemoryRepository.h"
#include "NullRepository.h"
#include "SQLiteRepository.h"
#include "TextRepository.h"


DataRepository::DataRepository(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
    :   m_type(type),
        m_caseAccess(std::move(case_access)),
        m_accessType(access_type)
{
    ASSERT(m_caseAccess != nullptr);
}


void DataRepository::Open(const ConnectionString& connection_string, const DataRepositoryOpenFlag open_flag)
{
    m_connectionString = connection_string;
    Open(open_flag);
}


std::string DataRepository::GetName(const DataRepositoryNameType name_type) const
{
    return m_connectionString.GetName(name_type);
}


std::unique_ptr<DataRepository> DataRepository::Create(std::shared_ptr<const CaseAccess> case_access,
                                                       const ConnectionString& connection_string,
                                                       const DataRepositoryAccess access_type)
{
    ASSERT(case_access != nullptr && case_access->IsInitialized());

    const DataRepositoryType type = connection_string.GetType();

    switch( type )
    {
        case DataRepositoryType::Null:            return std::make_unique<NullRepository>(std::move(case_access), access_type);
        case DataRepositoryType::Text:            return std::make_unique<TextRepository>(std::move(case_access), access_type);
        case DataRepositoryType::SQLite:          return std::make_unique<SQLiteRepository>(std::move(case_access), access_type, GetDeviceId());
        case DataRepositoryType::EncryptedSQLite: return std::make_unique<EncryptedSQLiteRepository>(std::move(case_access), access_type, GetDeviceId());
        case DataRepositoryType::Memory:          return std::make_unique<MemoryRepository>(std::move(case_access), access_type);
        case DataRepositoryType::Json:            return std::make_unique<JsonRepository>(std::move(case_access), access_type);
        case DataRepositoryType::CSWeb:           return std::make_unique<CSWebRepository>(std::move(case_access), access_type);

        default:
        {
            if( DataRepositoryHelpers::IsTypeExportWriter(type) )
                return std::make_unique<ExportWriterRepository>(type, std::move(case_access), access_type);

            throw DataRepositoryException::IOError("Invalid repository type");
        }
    }
}


std::unique_ptr<DataRepository> DataRepository::CreateAndOpen(std::shared_ptr<const CaseAccess> case_access,
                                                              const ConnectionString& connection_string,
                                                              const DataRepositoryAccess access_type, const DataRepositoryOpenFlag open_flag)
{
    std::unique_ptr<DataRepository> repository = Create(std::move(case_access), connection_string, access_type);

    repository->Open(connection_string, open_flag);

    return repository;
}


void DataRepository::DeleteCase(const std::string& key)
{
    std::string modifiable_key = key;
    std::string uuid;
    double position_in_repository;
    PopulateCaseIdentifiers(modifiable_key, uuid, position_in_repository);
    DeleteCase(position_in_repository, true);
}


std::unique_ptr<CaseIterator> DataRepository::CreateCaseIterator(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order)
{
    const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order);

    return CreateIterator(CaseIterationContent::Case, iterator_settings);
}


std::unique_ptr<CaseIterator> DataRepository::CreateCaseKeyIterator(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order)
{
    const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order);

    return CreateIterator(CaseIterationContent::CaseKey, iterator_settings);
}



// --------------------------------------------------------------------------
// CR_TODO remove all below...
// --------------------------------------------------------------------------

void DataRepository::ReadCasetainer(Case& casetainer, const std::string& key)
{
    casetainer.m_recalculatePre74Case = true;
    ReadCase(casetainer, key);
}


void DataRepository::ReadCasetainer(Case& casetainer, const double position_in_repository)
{
    casetainer.m_recalculatePre74Case = true;
    ReadCase(casetainer, position_in_repository);
}


void DataRepository::WriteCasetainer(Case& casetainer, const WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    casetainer.ApplyPre74_Case(casetainer.GetPre74_Case());
    WriteCase(casetainer, write_case_parameter);
}


bool DataRepository::NextCasetainer(CaseIterator& case_iterator, Case& data_case)
{
    data_case.m_recalculatePre74Case = true;
    return case_iterator.NextCase(data_case);
}
