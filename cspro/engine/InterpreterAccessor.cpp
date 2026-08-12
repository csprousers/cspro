#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "InterpreterAccessor.h"
#include "EngineDictionaryModifier.h"
#include "EngineExecutor.h"
#include "ParadataDriver.h"
#include <zEngineO/BinarySymbol.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/UserFunction.h>
#include <zMessageO/MessageManager.h>
#include <zCaseO/Case.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zBridgeO/NPff.h>


// --------------------------------------------------------------------------
// EngineInterpreterAccessor
// --------------------------------------------------------------------------

class EngineInterpreterAccessor : public InterpreterAccessor
{
public:
    EngineInterpreterAccessor(CIntDriver& interpreter);

    LogicInterpreter& GetInterpreter() override;

    const PFF& GetPff() override;

    const MessageFile& GetUserMessageFile() override;

    std::shared_ptr<const CDataDict> GetDictionary(std::string_view dictionary_name_sv) override;

    DataRepository& GetDataRepository(std::string_view dictionary_name_sv, bool check_level_is_valid_for_data_access) override;

    std::unique_ptr<Case> GetCurrentCase(std::string_view dictionary_name_sv) override;

    std::unique_ptr<EngineDictionaryModifier> CreateEngineDictionaryModifier(std::string_view dictionary_name_sv) override;

    std::unique_ptr<FieldStatusRetriever> CreateFieldStatusRetriever() override;

    InterpreterExecuteResult RunEvaluateLogic(SharableString logic, CancelFlag& cancel_flag) override;
    InterpreterExecuteResult RunInvoke(std::string_view function_name_sv, const JsonNode& json_arguments, CancelFlag& cancel_flag) override;

    InterpreterExecuteResult CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& argument_evaluator) override;

    std::variant<double, SharableString> CreateVariantFromEngineValue(Engine::Value value) override;

    std::string GetSymbolJson(const std::string& symbol_name_and_potential_subscript, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode* serialization_options_node) override;
    void SetSymbolValueFromJson(const std::string& symbol_name_and_potential_subscript, const JsonNode& json_node) override;

    std::string LocalhostCreateMappingForBinarySymbol(const std::string& symbol_name_and_potential_subscript, std::optional<std::string> content_type_override, bool evaluate_immediately) override;

    sqlite3& GetSqliteDbForDictionary(std::string_view dictionary_name_sv) override;

    void RegisterSqlCallbackFunctions(sqlite3* db) override;

    Paradata::ParadataDriver* GetParadataDriver() override;

private:
    Symbol& GetEvaluatedSymbolFromSymbolName(const std::string& symbol_name_and_potential_subscript);

    DICT& GetDictionary(std::string_view dictionary_name_sv, bool check_level_is_valid_for_data_access);

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


LogicInterpreter& EngineInterpreterAccessor::GetInterpreter()
{
    return m_interpreter;
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


std::shared_ptr<const CDataDict> EngineInterpreterAccessor::GetDictionary(const std::string_view dictionary_name_sv)
{
    DICT& dictionary = GetDictionary(dictionary_name_sv, false);
    return dictionary.GetSharedDictionary();
}


DataRepository& EngineInterpreterAccessor::GetDataRepository(const std::string_view dictionary_name_sv,
                                                             const bool check_level_is_valid_for_data_access)
{
    DICT& dictionary = GetDictionary(dictionary_name_sv, check_level_is_valid_for_data_access);
    return dictionary.GetDicX()->GetDataRepository();
}


std::unique_ptr<Case> EngineInterpreterAccessor::GetCurrentCase(const std::string_view dictionary_name_sv)
{
    DICT& dictionary = GetDictionary(dictionary_name_sv, true);

    std::unique_ptr<Case> data_case = dictionary.GetCaseAccess()->CreateCase(true);

    m_pEngineDriver->PrepareCaseFromEngineForQuestionnaireViewer(&dictionary, *data_case);

    return data_case;
}


std::unique_ptr<EngineDictionaryModifier> EngineInterpreterAccessor::CreateEngineDictionaryModifier(const std::string_view dictionary_name_sv)
{
    try
    {
        Symbol& symbol = m_interpreter.GetSymbolFromSymbolName(dictionary_name_sv);

        if( symbol.IsA(SymbolType::Pre80Dictionary) )
            return EngineDictionaryModifier::Create(m_interpreter, assert_cast<DICT&>(symbol));

        if( symbol.IsA(SymbolType::Dictionary) )
            return EngineDictionaryModifier::Create(m_interpreter, assert_cast<EngineDictionary&>(symbol));
    }
    catch(...) { }

    throw CSProException("No dictionary named '%s' exists.", std::string(dictionary_name_sv).c_str());
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


InterpreterExecuteResult EngineInterpreterAccessor::RunEvaluateLogic(SharableString logic, CancelFlag& cancel_flag)
{
    return m_interpreter.EvaluateLogic(std::move(logic), cancel_flag);
}


InterpreterExecuteResult EngineInterpreterAccessor::RunInvoke(const std::string_view function_name_sv, const JsonNode& json_arguments, CancelFlag& cancel_flag)
{
    return m_interpreter.RunInvoke(function_name_sv, json_arguments, &cancel_flag);
}


InterpreterExecuteResult EngineInterpreterAccessor::CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& argument_evaluator)
{
    return m_interpreter.Execute([&]() { return m_interpreter.CallUserFunction(user_function, argument_evaluator); });
}


std::variant<double, SharableString> EngineInterpreterAccessor::CreateVariantFromEngineValue(Engine::Value value)
{
    if( value.is<double>() )
        return value.get<double>();

    ASSERT(value.is<SharableString>());
    return std::move(value).as<SharableString>();
}


std::string EngineInterpreterAccessor::GetSymbolJson(const std::string& symbol_name_and_potential_subscript, const Symbol::SymbolJsonOutput symbol_json_output, const JsonNode* const serialization_options_node)
{
    const Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);
    return m_interpreter.GetSymbolJson(symbol, symbol_json_output, serialization_options_node);
}


void EngineInterpreterAccessor::SetSymbolValueFromJson(const std::string& symbol_name_and_potential_subscript, const JsonNode& json_node)
{
    Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);
    m_interpreter.SetSymbolValueFromJson(symbol, json_node);
}


std::string EngineInterpreterAccessor::LocalhostCreateMappingForBinarySymbol(const std::string& symbol_name_and_potential_subscript,
                                                                             std::optional<std::string> content_type_override, const bool evaluate_immediately)
{
    const Symbol& symbol = GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);

    if( !BinarySymbol::IsBinarySymbol(symbol) )
        throw CSProException("The symbol '%s' is not a binary symbol that can be mapped.", symbol.GetName().c_str());

    return m_interpreter.LocalhostCreateMappingForBinarySymbol(assert_cast<const BinarySymbol&>(symbol), std::move(content_type_override), evaluate_immediately);
}


sqlite3& EngineInterpreterAccessor::GetSqliteDbForDictionary(const std::string_view dictionary_name_sv)
{
    DICT& dictionary = GetDictionary(dictionary_name_sv, false);
    DICX* pDicX = dictionary.GetDicX();
    sqlite3* db = DataRepositoryHelpers::GetSqliteDatabase(pDicX->GetDataRepository());

    if( db != nullptr )
        return *db;

    if( pDicX->GetDataRepository().GetRepositoryType() == DataRepositoryType::Text )
        throw CSProException("Only text files that use an index have an associated SQLite database.");

    throw CSProException("There is no SQLite database associated with the dictionary '%s'.", std::string(dictionary_name_sv).c_str());
}


void EngineInterpreterAccessor::RegisterSqlCallbackFunctions(sqlite3* const db)
{
    m_interpreter.RegisterSqlCallbackFunctions(db);
}


Paradata::ParadataDriver* EngineInterpreterAccessor::GetParadataDriver()
{
    return m_interpreter.m_paradataDriver.get();
}


Symbol& EngineInterpreterAccessor::GetEvaluatedSymbolFromSymbolName(const std::string& symbol_name_and_potential_subscript)
{
    auto [base_symbol, wrapped_symbol] = m_interpreter.GetEvaluatedSymbolFromSymbolName(symbol_name_and_potential_subscript);

    return ( wrapped_symbol != nullptr ) ? *wrapped_symbol :
                                           *base_symbol;
}


DICT& EngineInterpreterAccessor::GetDictionary(const std::string_view dictionary_name_sv, const bool check_level_is_valid_for_data_access)
{
    DICT* pDicT = nullptr;

    try
    {
        Symbol& symbol = m_interpreter.GetSymbolFromSymbolName(dictionary_name_sv, SymbolType::Pre80Dictionary);
        ASSERT(!symbol.IsA(SymbolType::Dictionary)); // ENGINECR_TODO implement for non-DICT

        if( symbol.IsA(SymbolType::Pre80Dictionary) )
            pDicT = assert_cast<DICT*>(&symbol);
    }
    catch(...) { }

    if( pDicT == nullptr )
        throw CSProException("No dictionary named '%s' exists.", std::string(dictionary_name_sv).c_str());

    // make sure the case is currently available
    if( check_level_is_valid_for_data_access )
        m_interpreter.EnsureDataIsAccessible(*pDicT);

    return *pDicT;
}



// --------------------------------------------------------------------------
// CEngineDriver::CreateInterpreterAccessor
// --------------------------------------------------------------------------

std::unique_ptr<InterpreterAccessor> CEngineDriver::CreateInterpreterAccessor()
{
    return ( m_pIntDriver != nullptr ) ? std::make_unique<EngineInterpreterAccessor>(*m_pIntDriver) :
                                         nullptr;
}
