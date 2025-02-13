#include "stdafx.h"
#include "EngineDataRepository.h"
#include "EngineDictionary.h"
#include <zDataO/DataRepository.h>
#include <zMessageO/SystemMessageIssuer.h>


EngineDataRepository::EngineDataRepository(const EngineDictionary& engine_dictionary)
    :   m_engineDictionary(engine_dictionary),
        m_needsIndex(false),
        m_cannotHaveIndex(false),
        m_isWriteable(false),
        m_hasDynamicFileManagement(false),
        m_usesSync(false)
{
}


EngineDataRepository::~EngineDataRepository()
{
    CloseDataRepository();
}


std::unique_ptr<EngineDataRepository> EngineDataRepository::CloneInInitialState() const
{
    ASSERT(m_engineDictionary.IsDataRepositoryObject());

    auto engine_data_repository = std::unique_ptr<EngineDataRepository>(new EngineDataRepository(m_engineDictionary));

    engine_data_repository->m_needsIndex = m_needsIndex;
    engine_data_repository->m_cannotHaveIndex = m_cannotHaveIndex;
    engine_data_repository->m_isWriteable = m_isWriteable;
    engine_data_repository->m_hasDynamicFileManagement = m_hasDynamicFileManagement;
    engine_data_repository->m_usesSync = m_usesSync;

    engine_data_repository->InitializeRuntime(m_systemMessageIssuer);

    return engine_data_repository;
}


void EngineDataRepository::ApplyPermissions(const EngineDataRepository& rhs_engine_data_repository)
{
    m_needsIndex |= rhs_engine_data_repository.m_needsIndex;
    m_cannotHaveIndex |= rhs_engine_data_repository.m_cannotHaveIndex;
    m_isWriteable |= rhs_engine_data_repository.m_isWriteable;
    m_hasDynamicFileManagement |= rhs_engine_data_repository.m_hasDynamicFileManagement;
    m_usesSync |= rhs_engine_data_repository.m_usesSync;
}


void EngineDataRepository::serialize(Serializer& ar)
{
    ar & m_needsIndex
       & m_cannotHaveIndex
       & m_isWriteable
       & m_hasDynamicFileManagement
       & m_usesSync;
}


void EngineDataRepository::InitializeRuntime(std::shared_ptr<SystemMessageIssuer> system_message_issuer)
{
    ASSERT(system_message_issuer != nullptr);
    m_systemMessageIssuer = system_message_issuer;
}


DataRepository& EngineDataRepository::GetDataRepository()
{
    if( m_dataRepository == nullptr )
    {
        // make sure that a data repository always exists
        SetDataRepository(DataRepository::CreateAndOpen(
            m_engineDictionary.GetSharedCaseAccess(),
            ConnectionString::CreateNullRepositoryConnectionString(),
            DataRepositoryAccess::ReadWrite, DataRepositoryOpenFlag::CreateNew));
    }

    return *m_dataRepository;
}


const DataRepository& EngineDataRepository::GetDataRepository() const
{
    return const_cast<EngineDataRepository*>(this)->GetDataRepository();
}


void EngineDataRepository::SetDataRepository(std::shared_ptr<DataRepository> data_repository)
{
    ASSERT(data_repository != nullptr);
    m_dataRepository = std::move(data_repository);
}


void EngineDataRepository::CloseDataRepository()
{
    if( m_dataRepository == nullptr )
        return;

    m_lastClosedConnectionString = m_dataRepository->GetConnectionString();

    ClearLastSearchedKey();
    StopCaseIterator();
    m_lastLoadedCaseKey.reset();

    try
    {
        std::shared_ptr<DataRepository> data_repository = std::move(m_dataRepository);
        data_repository->Close();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }
}


void EngineDataRepository::ReadCase(EngineCase& engine_case, const CString& key)
{
    m_dataRepository->ReadCase(engine_case.GetCase(), key);
    m_lastLoadedCaseKey = engine_case.CalculateInitialCaseKey();
}


void EngineDataRepository::ReadCase(EngineCase& engine_case, double position_in_repository)
{
    m_dataRepository->ReadCase(engine_case.GetCase(), position_in_repository);
    m_lastLoadedCaseKey = engine_case.CalculateInitialCaseKey();
}


DictionaryAccessParameters EngineDataRepository::GetDictionaryAccessParameters(int dictionary_access) const
{
    // the first bit of a byte indicates if the value is set; bytes by order: method || order || status
    DictionaryAccessParameters dictionary_access_parameters = m_dictionaryAccessParameters;

    if( ( dictionary_access & 0x800000 ) != 0 )
        dictionary_access_parameters.case_iteration_method = (CaseIterationMethod)( ( dictionary_access >> 16 ) & 0x7F );

    if( ( dictionary_access & 0x8000 ) != 0 )
        dictionary_access_parameters.case_iteration_order = (CaseIterationOrder)( ( dictionary_access >> 8 ) & 0x7F );

    if( ( dictionary_access & 0x80 ) != 0 )
        dictionary_access_parameters.case_iteration_status = (CaseIterationCaseStatus)( dictionary_access & 0x7F );

    return dictionary_access_parameters;
}


void EngineDataRepository::SetDictionaryAccessParameters(std::variant<int, DictionaryAccessParameters> dictionary_access_or_parameters)
{
    m_dictionaryAccessParameters = std::holds_alternative<int>(dictionary_access_or_parameters) ?
        GetDictionaryAccessParameters(std::get<int>(dictionary_access_or_parameters)) :
        std::get<DictionaryAccessParameters>(dictionary_access_or_parameters);
}


void EngineDataRepository::CreateCaseIterator(CaseIteratorStyle case_iterator_style, const std::optional<CaseKey>& starting_key/* = std::nullopt*/,
                                              int dictionary_access/* = 0*/, std::optional<CString> key_prefix/* = std::nullopt*/,
                                              CaseIterationContent iteration_content/* = CaseIterationContent::Case*/)
{
    StopCaseIterator();

    // if the case key isn't defined, the iterator will start from the beginning of the file
    CaseIterationStartType iteration_start_type = CaseIterationStartType::GreaterThan;
    const CaseKey* case_key = nullptr;

    if( case_iterator_style == CaseIteratorStyle::FromCurrentPosition )
    {
        // if a case has been loaded, create an iterator from it
        if( m_lastLoadedCaseKey.has_value() )
            case_key = &(*m_lastLoadedCaseKey);
    }

    else if( case_iterator_style == CaseIteratorStyle::FromLastSearchedCaseKey )
    {
        ASSERT(IsLastSearchedCaseKeyDefined());
        iteration_start_type = CaseIterationStartType::GreaterThanEquals;
        case_key = &(*m_lastSearchedCaseKey);
    }
    
    else if( case_iterator_style == CaseIteratorStyle::FromNextKey )
    {
        ASSERT(starting_key.has_value());
        case_key = &(*starting_key);
    }

    DictionaryAccessParameters dictionary_access_parameters = GetDictionaryAccessParameters(dictionary_access);
    std::unique_ptr<CaseIteratorParameters> start_parameters;
    
    if( case_key != nullptr || key_prefix.has_value() )
    {
        // flip the order for a descending iterator
        if( dictionary_access_parameters.case_iteration_order == CaseIterationOrder::Descending )
        {
            if( iteration_start_type == CaseIterationStartType::GreaterThan )
            {
                iteration_start_type = CaseIterationStartType::LessThan;
            }

            else if( iteration_start_type == CaseIterationStartType::GreaterThanEquals )
            {
                iteration_start_type = CaseIterationStartType::LessThanEquals;
            }
        }

        if( dictionary_access_parameters.case_iteration_method == CaseIterationMethod::KeyOrder )
        {
            start_parameters = std::make_unique<CaseIteratorParameters>(iteration_start_type, ( case_key != nullptr ) ? case_key->GetKey() : CString(), key_prefix);
        }

        else
        {
            start_parameters = std::make_unique<CaseIteratorParameters>(iteration_start_type, ( case_key != nullptr ) ? case_key->GetPositionInRepository() : -1, key_prefix);
        }
    }

    m_caseIterator = GetDataRepository().CreateIterator(iteration_content, dictionary_access_parameters.case_iteration_status,
        dictionary_access_parameters.case_iteration_method, dictionary_access_parameters.case_iteration_order, start_parameters.get());
}


bool EngineDataRepository::StepCaseIterator(EngineCase& engine_case)
{
    ASSERT(IsCaseIteratorActive());

    if( m_caseIterator->NextCase(engine_case.GetCase()) )
    {
        m_lastLoadedCaseKey = engine_case.CalculateInitialCaseKey();
        return true;
    }

    return false;
}


void EngineDataRepository::StopCaseIterator()
{
    try
    {
        m_caseIterator.reset();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }
}
