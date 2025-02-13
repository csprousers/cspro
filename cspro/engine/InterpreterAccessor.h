#pragma once

#include <zLogicO/Symbol.h>
#include <zAppO/FieldStatus.h>

class Case;
class MessageFile;
class PFF;
struct sqlite3;


struct InterpreterExecuteResult
{
    std::variant<double, std::wstring> result;
    bool program_control_executed;
};


// the InterpreterAccessor class can be used to access the interpreter from projects
// that may not depend on the engine, which is why the entry points are all virtual

class InterpreterAccessor
{
public:
    virtual ~InterpreterAccessor() { }

    virtual const PFF& GetPff() = 0;

    virtual const MessageFile& GetUserMessageFile() = 0;

    // throws exceptions from the data repository, otherwise returns a non-null pointer
    virtual std::unique_ptr<Case> GetCase(const std::wstring& dictionary_name, const std::optional<std::wstring>& case_uuid, const std::optional<std::wstring>& case_key) = 0;

    // throws an exception if no current case exists, otherwise returns a non-null pointer
    virtual std::unique_ptr<Case> GetCurrentCase(const std::wstring& dictionary_name) = 0;

    // returns null when one cannot be created (e.g., for a non-entry application)
    virtual std::unique_ptr<FieldStatusRetriever> CreateFieldStatusRetriever() = 0;

    // throws exceptions on compilation errors
    virtual InterpreterExecuteResult RunEvaluateLogic(const std::wstring& logic, bool& cancel_flag) = 0;
    virtual InterpreterExecuteResult RunInvoke(const StringNoCase& function_name, const JsonNode<wchar_t>& json_arguments, bool& cancel_flag) = 0;

    // throws exceptions
    virtual std::wstring GetSymbolJson(const std::wstring& symbol_name_and_potential_subscript, Symbol::SymbolJsonOutput symbol_json_output, const JsonNode<wchar_t>* serialization_options_node) = 0;
    virtual void UpdateSymbolValueFromJson(const std::wstring& symbol_name_and_potential_subscript, const JsonNode<wchar_t>& json_node) = 0;

    // throws exceptions
    virtual std::wstring LocalhostCreateMappingForBinarySymbol(const std::wstring& symbol_name_and_potential_subscript, std::optional<std::wstring> content_type_override, bool evaluate_immediately) = 0;

    // throws an exception if the dictionary does not exist, or if it does not have a SQLite database associated with it
    virtual sqlite3& GetSqliteDbForDictionary(const std::wstring& dictionary_name) = 0;

    // throws an exception on error
    virtual void RegisterSqlCallbackFunctions(sqlite3* db) = 0;
};
