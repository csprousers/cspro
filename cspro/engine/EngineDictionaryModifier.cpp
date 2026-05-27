#include "StandardSystemIncludes.h"
#include "ExpresC_Include.h"
#include "EngineDictionaryModifier.h"
#include <zEngineO/EngineDictionary.h>
#include <zDataO/DataRepositoryUniqueCaseIdentifer.h>


// --------------------------------------------------------------------------
// EngineDictionaryModifier_Base
// --------------------------------------------------------------------------

class EngineDictionaryModifier_Base : public EngineDictionaryModifier
{
protected:
    EngineDictionaryModifier_Base(CIntDriver& interpreter);

public:
    void PrepareForModifications() override;
    void FinishedWithModifications() override;

protected:
    virtual bool IsInputDictionary() = 0;
    virtual DataRepository& GetDataRepository() = 0;
    virtual void LoadCaseBinaryData() = 0;
    virtual void ResetCaseIterators() = 0;

protected:
    CIntDriver& m_interpreter;

private:
    void GetCurrentCaseData();
    void RefreshCurrentCaseData();

private:
    // details about the case being entered
    std::unique_ptr<WriteCaseParameter> m_writeCaseParameter;
    std::unique_ptr<DataRepositoryUniqueCaseIdentifer> m_uniqueCaseIdentifier;
};


EngineDictionaryModifier_Base::EngineDictionaryModifier_Base(CIntDriver& interpreter)
    :   m_interpreter(interpreter)
{
}


void EngineDictionaryModifier_Base::PrepareForModifications()
{
    if( IsInputDictionary() )
        GetCurrentCaseData();

    LoadCaseBinaryData();

    ResetCaseIterators();
}


void EngineDictionaryModifier_Base::GetCurrentCaseData()
{
    ASSERT(m_writeCaseParameter == nullptr || m_uniqueCaseIdentifier == nullptr);
    ASSERT(IsInputDictionary());

    const WriteCaseParameter* source_write_case_parameter = m_interpreter.m_pEngineDriver->GetWriteCaseParameter();

    if( source_write_case_parameter == nullptr )
        return;

    m_writeCaseParameter = CreateCopyOfPointerValue(source_write_case_parameter);

    ASSERT(!m_writeCaseParameter->GetKey().empty() || m_writeCaseParameter->IsInsertParameter());
    ASSERT(m_writeCaseParameter->GetPositionInRepository() != -1);

    DataRepository& data_repository = GetDataRepository();

    // because insertion parameters only have the position, query for the key
    if( m_writeCaseParameter->IsInsertParameter() )
    {
        std::string key;
        std::string uuid;
        double position_in_repository = m_writeCaseParameter->GetPositionInRepository();
        data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

        ASSERT(!key.empty() && position_in_repository == m_writeCaseParameter->GetPositionInRepository());
        m_writeCaseParameter->SetKey(key);
    }

    m_uniqueCaseIdentifier = std::make_unique<DataRepositoryUniqueCaseIdentifer>(
        data_repository.GetUniqueCaseIdentifer(*m_writeCaseParameter)
    );
}


void EngineDictionaryModifier_Base::FinishedWithModifications()
{
    if( m_writeCaseParameter != nullptr )
        RefreshCurrentCaseData();
}


void EngineDictionaryModifier_Base::RefreshCurrentCaseData()
{
    ASSERT(m_writeCaseParameter != nullptr && m_uniqueCaseIdentifier != nullptr);
    std::unique_ptr<WriteCaseParameter> write_case_parameter = std::move(m_writeCaseParameter);
    std::unique_ptr<const DataRepositoryUniqueCaseIdentifer> unique_case_identifier = std::move(m_uniqueCaseIdentifier);

    const WriteCaseParameter* const source_write_case_parameter = m_interpreter.m_pEngineDriver->GetWriteCaseParameter();

    if( source_write_case_parameter == nullptr ||
        source_write_case_parameter->GetPositionInRepository() != write_case_parameter->GetPositionInRepository() )
    {
        ASSERT(false);
        return;
    }

    ASSERT(write_case_parameter->IsModifyParameter() == source_write_case_parameter->IsModifyParameter());
    ASSERT(( write_case_parameter->GetKey() == source_write_case_parameter->GetKey() ) ||
           ( source_write_case_parameter->GetKey().empty() && source_write_case_parameter->IsInsertParameter() ));

    DataRepository& data_repository = GetDataRepository();

    try
    {
        std::string key;
        std::string uuid;
        double position_in_repository = unique_case_identifier->GetPosition(data_repository);
        data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

        // if the case's key and position did not change, nothing needs to happen
        if( key == write_case_parameter->GetKey() &&
            position_in_repository == write_case_parameter->GetPositionInRepository() )
        {
            return;
        }

        // if it changed, but the case still exists, simply update the parameter
        else
        {
            write_case_parameter->SetKey(key);
            write_case_parameter->SetPositionInRepository(position_in_repository);
            m_interpreter.m_pEngineDriver->SetWriteCaseParameter(std::move(*write_case_parameter));
            return;
        }
    }
    catch( const DataRepositoryException::CaseNotFound& ) { }

    // if this is an insertion and the case was not matched (e.g., a deleted case in a text file),
    // try to search for the closest case to the original position
    if( write_case_parameter->IsInsertParameter() )
    {
        const CaseIteratorParameters start_parameters(
            CaseIterationStartType::LessThanEquals,
            write_case_parameter->GetPositionInRepository(),
            std::nullopt
        );

        const std::optional<CaseKey> case_key = data_repository.FindCaseKey(
            CaseIterationMethod::SequentialOrder,
            CaseIterationOrder::Descending,
            &start_parameters
        );

        if( case_key.has_value() )
        {
            m_interpreter.m_pEngineDriver->SetWriteCaseParameter(
                WriteCaseParameter::CreateInsertParameter(case_key->GetPositionInRepository())
            );

            return;
        }
    }

    // if the case cannot be reconciled in any way, clear the parameter
    m_interpreter.m_pEngineDriver->ClearWriteCaseParameter();
}



// --------------------------------------------------------------------------
// EngineDictionaryModifier_DICT
// --------------------------------------------------------------------------

class EngineDictionaryModifier_DICT : public EngineDictionaryModifier_Base
{
public:
    EngineDictionaryModifier_DICT(CIntDriver& interpreter, DICT& dict);

protected:
    bool IsInputDictionary() override;
    DataRepository& GetDataRepository() override;
    void LoadCaseBinaryData() override;
    void ResetCaseIterators() override;

private:
    DICT& m_dict;
    DICX& m_dicx;
};


EngineDictionaryModifier_DICT::EngineDictionaryModifier_DICT(CIntDriver& interpreter, DICT& dict)
    :   EngineDictionaryModifier_Base(interpreter),
        m_dict(dict),
        m_dicx(*m_dict.GetDicX())
{
}


bool EngineDictionaryModifier_DICT::IsInputDictionary()
{
    return ( m_dict.GetSubType() == SymbolSubType::Input );
}


DataRepository& EngineDictionaryModifier_DICT::GetDataRepository()
{
    return m_dicx.GetDataRepository();
}


void EngineDictionaryModifier_DICT::LoadCaseBinaryData()
{
    m_interpreter.m_pEngineDriver->LoadAllBinaryDataFromRepository(&m_dicx);
}


void EngineDictionaryModifier_DICT::ResetCaseIterators()
{
    m_dicx.ClearLastSearchedKey();
    m_dicx.StopCaseIterator();
}



// --------------------------------------------------------------------------
// EngineDictionaryModifier_ED: ENGINECR_TODO this class has not been tested
// --------------------------------------------------------------------------

class EngineDictionaryModifier_ED : public EngineDictionaryModifier_Base
{
public:
    EngineDictionaryModifier_ED(CIntDriver& interpreter, EngineDictionary& engine_dictionary);

protected:
    bool IsInputDictionary() override;
    DataRepository& GetDataRepository() override;
    void LoadCaseBinaryData() override;
    void ResetCaseIterators() override;

private:
    EngineDictionary& m_engineDictionary;
    EngineDataRepository& m_engineDataRepository;
};


EngineDictionaryModifier_ED::EngineDictionaryModifier_ED(CIntDriver& interpreter, EngineDictionary& engine_dictionary)
    :   EngineDictionaryModifier_Base(interpreter),
        m_engineDictionary(engine_dictionary),
        m_engineDataRepository(m_engineDictionary.GetEngineDataRepository())
{
}


bool EngineDictionaryModifier_ED::IsInputDictionary()
{
    return ( m_engineDictionary.GetSubType() == SymbolSubType::Input );
}


DataRepository& EngineDictionaryModifier_ED::GetDataRepository()
{
    return m_engineDataRepository.GetDataRepository();
}


void EngineDictionaryModifier_ED::LoadCaseBinaryData()
{
    m_interpreter.m_pEngineDriver->LoadAllBinaryDataFromRepository(m_engineDataRepository);
}


void EngineDictionaryModifier_ED::ResetCaseIterators()
{
    m_engineDataRepository.ClearLastSearchedKey();
    m_engineDataRepository.StopCaseIterator();
}



// --------------------------------------------------------------------------
// EngineDictionaryModifier
// --------------------------------------------------------------------------

std::unique_ptr<EngineDictionaryModifier> EngineDictionaryModifier::Create(CIntDriver& interpreter, DICT& dict)
{
    return std::make_unique<EngineDictionaryModifier_DICT>(interpreter, dict);
}


std::unique_ptr<EngineDictionaryModifier> EngineDictionaryModifier::Create(CIntDriver& interpreter, EngineDictionary& engine_dictionary)
{
    return std::make_unique<EngineDictionaryModifier_ED>(interpreter, engine_dictionary);
}
