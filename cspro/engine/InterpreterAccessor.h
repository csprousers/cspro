#pragma once

#include <zLogicO/Symbol.h>
#include <zAppO/FieldStatus.h>
#include <zEngineO/EngineValue.h>

class CancelFlag;
class Case;
class DataRepository;
class EngineDictionaryModifier;
class LogicInterpreter;
class MessageFile;
namespace Paradata { class ParadataDriver; }
class PFF;
struct sqlite3;
class UserFunction;
class UserFunctionArgumentEvaluator;


struct InterpreterExecuteResult
{
    Engine::Value result;
    bool program_control_executed;
};


// --------------------------------------------------------------------------
// InterpreterAccessor can be used to access the interpreter from projects
// that may not depend on the engine, which is why the entry points are all
// virtual.
// --------------------------------------------------------------------------

class InterpreterAccessor
{
public:
    virtual ~InterpreterAccessor() { }

    virtual LogicInterpreter& GetInterpreter() = 0;

    virtual const PFF& GetPff() = 0;

    virtual const MessageFile& GetUserMessageFile() = 0;

    // Throws an exception if the dictionary does not exist.
    virtual std::shared_ptr<const CDataDict> GetDictionary(std::string_view dictionary_name_sv) = 0;

    // Throws an exception if the dictionary does not exist.
    virtual DataRepository& GetDataRepository(std::string_view dictionary_name_sv, bool check_level_is_valid_for_data_access) = 0;

    // Throws an exception if no current case exists, otherwise returns a non-null pointer.
    virtual std::unique_ptr<Case> GetCurrentCase(std::string_view dictionary_name_sv) = 0;

    // Throws an exception if the dictionary does not exist.
    virtual std::unique_ptr<EngineDictionaryModifier> CreateEngineDictionaryModifier(std::string_view dictionary_name_sv) = 0;

    // Returns null when one cannot be created (e.g., for a non-entry application).
    virtual std::unique_ptr<FieldStatusRetriever> CreateFieldStatusRetriever() = 0;

    // Throws exceptions on compilation errors.
    virtual InterpreterExecuteResult RunEvaluateLogic(SharableString logic, CancelFlag& cancel_flag) = 0;
    virtual InterpreterExecuteResult RunInvoke(std::string_view function_name_sv, const JsonNode& json_arguments, CancelFlag& cancel_flag) = 0;

    // Executes the user-defined function with the provided arguments.
    virtual InterpreterExecuteResult CallUserFunction(UserFunction& user_function, UserFunctionArgumentEvaluator& argument_evaluator) = 0;

    // Converts an engine value to either a number or string.
    virtual std::variant<double, SharableString> CreateVariantFromEngineValue(Engine::Value value) = 0;

    // Throws exceptions.
    virtual std::string GetSymbolJson(const std::string& symbol_name_and_potential_subscript, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode* serialization_options_node) = 0;
    virtual void SetSymbolValueFromJson(const std::string& symbol_name_and_potential_subscript, const JsonNode& json_node) = 0;

    // Throws exceptions.
    virtual std::string LocalhostCreateMappingForBinarySymbol(const std::string& symbol_name_and_potential_subscript, std::optional<std::string> content_type_override, bool evaluate_immediately) = 0;

    // Throws an exception if the dictionary does not exist, or if it does not have a SQLite database associated with it.
    virtual sqlite3& GetSqliteDbForDictionary(std::string_view dictionary_name_sv) = 0;

    // Throws an exception on error.
    virtual void RegisterSqlCallbackFunctions(sqlite3* db) = 0;

    // Returns null if paradata is not in use.
    virtual Paradata::ParadataDriver* GetParadataDriver() = 0;
};
