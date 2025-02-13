#include "stdafx.h"
#include "EngineDictionary.h"
#include <zLogicO/KeywordTable.h>
#include <zDataO/DataRepository.h>


// --------------------------------------------------------------------------
// EngineDictionary
// --------------------------------------------------------------------------

EngineDictionary::EngineDictionary(std::wstring dictionary_name, EngineData& engine_data)
    :   Symbol(std::move(dictionary_name), SymbolType::Dictionary),
        m_engineData(engine_data),
        m_contents(Contents::Dictionary)
{
}


EngineDictionary::EngineDictionary(Contents contents, std::wstring dictionary_name, std::shared_ptr<const CDataDict> dictionary,
                                   std::shared_ptr<CaseAccess> case_access, EngineData& engine_data)
    :   EngineDictionary(std::move(dictionary_name), engine_data)
{
    ASSERT(dictionary != nullptr && case_access != nullptr);

    m_contents = contents;
    m_dictionary = std::move(dictionary);
    m_caseAccess = std::move(case_access);

    if( HasEngineCase() )
        m_engineCase = std::unique_ptr<EngineCase>(new EngineCase(*this, engine_data));

    if( HasEngineDataRepository() )
        m_engineDataRepository = std::unique_ptr<EngineDataRepository>(new EngineDataRepository(*this));
}


EngineDictionary::EngineDictionary(const EngineDictionary& engine_dictionary)
    :   Symbol(engine_dictionary),
        m_engineData(engine_dictionary.m_engineData),
        m_contents(engine_dictionary.m_contents),
        m_dictionary(engine_dictionary.m_dictionary),
        m_caseAccess(engine_dictionary.m_caseAccess),
        m_resetOverride(engine_dictionary.m_resetOverride)

{
    ASSERT(m_caseAccess->IsInitialized() && !IsDictionaryObject());

    if( IsCaseObject() )
    {
        m_engineCase = engine_dictionary.m_engineCase->CloneInInitialState();
    }

    else
    {
        ASSERT(IsDataRepositoryObject());
        m_engineDataRepository = engine_dictionary.m_engineDataRepository->CloneInInitialState();
    }
}


std::unique_ptr<Symbol> EngineDictionary::CloneInInitialState() const
{
    return std::unique_ptr<EngineDictionary>(new EngineDictionary(*this));
}


EngineDictionary::~EngineDictionary()
{
}


Symbol* EngineDictionary::FindChildSymbol(const std::wstring& symbol_name) const
{
    return HasEngineCase() ? m_engineCase->FindChildSymbol(symbol_name) : nullptr;
}


void EngineDictionary::Reset()
{
    if( IsCaseObject() )
    {
        m_engineCase->Reset();
    }

    else if( IsDataRepositoryObject() )
    {
        ASSERT(m_resetOverride != nullptr);
        (*m_resetOverride)(*m_engineDataRepository);
        m_engineDataRepository->CloseDataRepository();
    }

    else
    {
        ASSERT(IsDictionaryObject());
    }
}


void EngineDictionary::serialize_subclass(Serializer& ar)
{
    ar.SerializeEnum(m_contents);

    // only serialize the case access in the main dictionary object
    if( IsDictionaryObject() )
    {
        ar & *m_caseAccess;
    }

    // otherwise link to the dictionary (that belongs to the dictionary object)
    else
    {
        if( ar.IsSaving() )
        {
            ar.Write<int>(m_dictionary->GetSymbol());
        }

        else
        {
            EngineDictionary& dictionary_object_engine_dictionary = assert_cast<EngineDictionary&>(m_engineData.symbol_table.GetAt(ar.Read<int>()));
            m_dictionary = dictionary_object_engine_dictionary.m_dictionary;
            m_caseAccess = dictionary_object_engine_dictionary.m_caseAccess;
        }
    }

    ASSERT(m_dictionary != nullptr && m_caseAccess != nullptr);


    // serialize the EngineCase and EngineDataRepository objects
    if( HasEngineCase() )
    {
        if( m_engineCase == nullptr )
        {
            ASSERT(ar.IsLoading());
            m_engineCase = std::unique_ptr<EngineCase>(new EngineCase(*this, m_engineData));
        }

        m_engineCase->serialize(ar);
    }

    if( HasEngineDataRepository() )
    {
        if( m_engineDataRepository == nullptr )
        {
            ASSERT(ar.IsLoading());
            m_engineDataRepository = std::unique_ptr<EngineDataRepository>(new EngineDataRepository(*this));
        }

        m_engineDataRepository->serialize(ar);
    }
}


void EngineDictionary::InitializeRuntime(std::shared_ptr<SystemMessageIssuer> system_message_issuer,
                                         std::shared_ptr<CaseConstructionReporter> case_construction_reporter_override,
                                         std::shared_ptr<std::function<void(EngineDataRepository&)>> reset_override)
{
    ASSERT(m_caseAccess->IsInitialized());
    ASSERT(IsDataRepositoryObject() == ( reset_override != nullptr ));

    if( HasEngineCase() )
        m_engineCase->InitializeRuntime(std::move(case_construction_reporter_override));

    if( HasEngineDataRepository() )
    {
        m_engineDataRepository->InitializeRuntime(std::move(system_message_issuer));
        m_resetOverride = std::move(reset_override);
    }
}



// --------------------------------------------------------------------------
// JSON serialization
// --------------------------------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(EngineDictionary::Contents,
    { EngineDictionary::Contents::Dictionary,    _T("Dictionary") },
    { EngineDictionary::Contents::Case,           Logic::KeywordTable::GetKeywordName(TokenCode::TOKKWCASE) },
    { EngineDictionary::Contents::DataRepository, Logic::KeywordTable::GetKeywordName(TokenCode::TOKKWDATASOURCE) })


void EngineDictionary::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_dictionary != nullptr);

    json_writer.Write(JK::subtype, m_contents);

    if( IsDictionaryObject() )
        json_writer.Write(JK::dictionaryType, GetSubType());

    json_writer.Write(JK::dictionary, *m_dictionary);
}


void EngineDictionary::WriteValueToJson(JsonWriter& json_writer) const
{
    if( IsDictionaryObject() )
        json_writer.BeginObject();

    if( HasEngineCase() )
    {
        ASSERT(m_engineCase != nullptr);

        if( IsDictionaryObject() )
        {
            // ENGINECR_TODO call CIntDriver::IsDataAccessible to make sure the case data is available (see CSymbolDict::WriteValueToJson)
            json_writer.Key(JK::case_);
        }

        json_writer.Write(m_engineCase->GetCase());
    }

    if( HasEngineDataRepository() )
    {
        ASSERT(m_engineDataRepository != nullptr);

        if( IsDictionaryObject() )
            json_writer.Key(JK::dataSource);

        const DataRepository& data_repository = m_engineDataRepository->GetDataRepository();

        json_writer.BeginObject()
                   .Write(JK::type, ToString(data_repository.GetRepositoryType()))
                   .Write(JK::connectionString, data_repository.GetConnectionString())
                   .EndObject();
    }

    if( IsDictionaryObject() )
        json_writer.EndObject();
}
