#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "InterpreterAccessor.h"
#include <zEngineO/BinarySymbol.h>
#include <zMessageO/MessageManager.h>
#include <zCaseO/Case.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <ZBRIDGEO/npff.h>


// --------------------------------------------------------------------------
// EngineInterpreterAccessor
// --------------------------------------------------------------------------

class EngineInterpreterAccessor : public InterpreterAccessor
{
public:
    EngineInterpreterAccessor(CIntDriver& interpreter);

    const PFF& GetPff() override;

    const MessageFile& GetUserMessageFile() override;

    std::unique_ptr<Case> GetCase(const std::wstring& dictionary_name, const std::optional<std::wstring>& case_uuid, const std::optional<std::wstring>& case_key) override;
    std::unique_ptr<Case> GetCurrentCase(const std::wstring& dictionary_name) override;

    std::unique_ptr<FieldStatusRetriever> CreateFieldStatusRetriever() override;

    InterpreterExecuteResult RunEvaluateLogic(const std::wstring& logic, bool& cancel_flag) override;
    InterpreterExecuteResult RunInvoke(const StringNoCase& function_name, const JsonNode<wchar_t>& json_arguments, bool& cancel_flag) override;

    std::wstring GetSymbolJson(const std::wstring& symbol_name_and_potential_subscript, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode<wchar_t>* serialization_options_node) override;
    void UpdateSymbolValueFromJson(const std::wstring& symbol_name_and_potential_subscript, const JsonNode<wchar_t>& json_node) override;

    std::wstring LocalhostCreateMappingForBinarySymbol(const std::wstring& symbol_name_and_potential_subscript, std::optional<std::wstring> content_type_override, bool evaluate_immediately) override;

    sqlite3& GetSqliteDbForDictionary(const std::wstring& dictionary_name) override;

    void RegisterSqlCallbackFunctions(sqlite3* db) override;

private:
    Symbol& GetEvaluatedSymbolFromSymbolName(const std::wstring& symbol_name_and_potential_subscript);

    DICT& GetDictionary(const std::wstring& dictionary_name, bool check_level_is_valid_for_data_access);

private:
    CIntDriver& m_interpreter;
    CEngineDriver* m_pEngineDriver;
};


EngineInterpreterAccessor::EngineInterpreterAccessor(CIntDriver& interpreter)
    :   m_interpreter(interpreter),
        m_pEngineDriver(m_interpreter.m_pEngineDriver)
{
    ASSERT(m_pEngineDriver != nullptr);
}


const PFF& EngineInterpreterAccessor::GetPff()
{
    ASSERT(m_pEngineDriver->m_pPifFile != nullptr);
    return *m_pEngineDriver->m_pPifFile;
}


const MessageFile& EngineInterpreterAccessor::GetUserMessageFile()
{
    return m_pEngineDriver->GetUserMessageManager().GetMessageFile();
}


std::unique_ptr<Case> EngineInterpreterAccessor::GetCase(const std::wstring& dictionary_name, const std::optional<std::wstring>& case_uuid, const std::optional<std::wstring>& case_key)
{
    ASSERT(case_uuid.has_value() || case_key.has_value());

    DICT& dictionary = GetDictionary(dictionary_name, false);

    std::unique_ptr<Case> data_case = dictionary.GetCaseAccess()->CreateCase();

    DataRepository& data_repository = dictionary.GetDicX()->GetDataRepository();

    // load the case by UUID...
    if( case_uuid.has_value() )
    {
        CString key;
        CString uuid = WS2CS(*case_uuid);
        double position_in_repository;
        data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

        data_repository.ReadCase(*data_case, position_in_repository);
    }

    // ...or by key
    else
    {
        data_repository.ReadCase(*data_case, WS2CS(*case_key));
    }

    return data_case;
}


std::unique_ptr<Case> EngineInterpreterAccessor::GetCurrentCase(const std::wstring& dictionary_name)
{
    DICT& dictionary = GetDictionary(dictionary_name, true);

    std::unique_ptr<Case> data_case = dictionary.GetCaseAccess()->CreateCase();

    m_pEngineDriver->PrepareCaseFromEngineForQuestionnaireViewer(&dictionary, *data_case);

    return data_case;
}


std::unique_ptr<FieldStatusRetriever> EngineInterpreterAccessor::CreateFieldStatusRetriever()
{
    if( Issamod != ModuleType::Entry )
        return nullptr;

    return std::make_unique<FieldStatusRetriever>(
        [interpreter = &m_interpreter](const CaseItem& case_item, const CaseItemIndex& index)
        {
            return interpreter->GetFieldStatus(case_item, index);
        });
}


InterpreterExecuteResult EngineInterpreterAccessor::RunEvaluateLogic(const std::wstring& logic, bool& /*cancel_flag*/) // CS_TODO what to do about cancel_flag? should it somehow hook into CIntDriver::m_bStopProc?
{
    return m_interpreter.EvaluateLogic(logic); 
}


InterpreterExecuteResult EngineInterpreterAccessor::RunInvoke(const StringNoCase& function_name, const JsonNode<wchar_t>& json_arguments, bool& /*cancel_flag*/) // CS_TODO see above cancel_flag message?
{
    return m_interpreter.RunInvoke(function_name, json_arguments);
}


std::wstring EngineInterpreterAccessor::GetSymbolJson(const std::wstring& symbol_name_and_potential_subscript, const Symbol::SymbolJsonOutput symbol_json_output, const JsonNode<wchar_t>* serialization_options_node)
{
    const Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);
    return m_interpreter.GetSymbolJson(symbol, symbol_json_output, serialization_options_node);
}


void EngineInterpreterAccessor::UpdateSymbolValueFromJson(const std::wstring& symbol_name_and_potential_subscript, const JsonNode<wchar_t>& json_node)
{
    Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);
    m_interpreter.UpdateSymbolValueFromJson(symbol, json_node);
}


std::wstring EngineInterpreterAccessor::LocalhostCreateMappingForBinarySymbol(const std::wstring& symbol_name_and_potential_subscript, std::optional<std::wstring> content_type_override, bool evaluate_immediately)
{
    const Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);

    if( !BinarySymbol::IsBinarySymbol(symbol) )
        throw CSProException(_T("The symbol '%s' is not a binary symbol that can be mapped."), symbol.GetName().c_str());

    return m_interpreter.LocalhostCreateMappingForBinarySymbol(assert_cast<const BinarySymbol&>(symbol), std::move(content_type_override), evaluate_immediately);
}


sqlite3& EngineInterpreterAccessor::GetSqliteDbForDictionary(const std::wstring& dictionary_name)
{
    DICT& dictionary = GetDictionary(dictionary_name, false);
    DICX* pDicX = dictionary.GetDicX();
    sqlite3* db = DataRepositoryHelpers::GetSqliteDatabase(pDicX->GetDataRepository());

    if( db != nullptr )
        return *db;

    if( pDicX->GetDataRepository().GetRepositoryType() == DataRepositoryType::Text )
        throw CSProException("Only text files that use an index have an associated SQLite database.");

    throw CSProException(_T("There is no SQLite database associated with the dictionary '%s'."), dictionary_name.c_str());
}


void EngineInterpreterAccessor::RegisterSqlCallbackFunctions(sqlite3* db)
{
    m_interpreter.RegisterSqlCallbackFunctions(db);
}


Symbol& EngineInterpreterAccessor::GetEvaluatedSymbolFromSymbolName(const std::wstring& symbol_name_and_potential_subscript)
{
    auto [base_symbol, wrapped_symbol] = m_interpreter.GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);

    return ( wrapped_symbol != nullptr ) ? *wrapped_symbol :
                                           *base_symbol;
}


DICT& EngineInterpreterAccessor::GetDictionary(const std::wstring& dictionary_name, const bool check_level_is_valid_for_data_access)
{
    DICT* pDicT = nullptr;

    try
    {
        Symbol& symbol = m_interpreter.GetSymbolFromSymbolName(dictionary_name, SymbolType::Pre80Dictionary);
        ASSERT(!symbol.IsA(SymbolType::Dictionary)); // ENGINECR_TODO implement for non-DICT

        if( symbol.IsA(SymbolType::Pre80Dictionary) )
            pDicT = assert_cast<DICT*>(&symbol);
    }
    catch(...) { }

    if( pDicT == nullptr )
        throw CSProException(_T("No dictionary named '%s' exists."), dictionary_name.c_str());

    // make sure the case is currently available
    if( check_level_is_valid_for_data_access )
        m_interpreter.EnsureDataIsAccessible(*pDicT);

    return *pDicT;
}



// --------------------------------------------------------------------------
// CEngineDriver::CreateInterpreterAccessor
// --------------------------------------------------------------------------

std::shared_ptr<InterpreterAccessor> CEngineDriver::CreateInterpreterAccessor()
{
    return ( m_pIntDriver != nullptr ) ? std::make_shared<EngineInterpreterAccessor>(*m_pIntDriver) :
                                         nullptr;
}
