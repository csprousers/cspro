#pragma once

#include <zEngineO/zEngineO.h>
#include <zJavaScript/Definitions.h>

class InterpreterAccessor;


class ZENGINEO_API EngineJavaScriptProcessor
{
public:
    EngineJavaScriptProcessor(EngineData& engine_data);
    ~EngineJavaScriptProcessor();

    // Returns the JavaScript executor.
    JavaScript::Executor& GetExecutor() { return *m_executor; }

    // Compiles the script and stores the bytecode for later evaluation using the bytecode index.
    size_t CompileScript(SharableString script, JavaScript::ModuleType module_type);

    // Compiles the script from the code file and stores the bytecode for later evaluation during application startup.
    void CompileCodeFile(const CodeFile& code_file);

    // Evaluates the stored bytecode.
    std::string EvaluateBytecode(size_t bytecode_index);

    // Evaluates the bytecode marked for evaluation during application startup.
    void EvaluateApplicationStartupBytecode();

    // Evaluates the script for the JS.eval logic function.
    std::string EvaluateScript(const std::string& script, bool& exception_is_from_compilation);

    // Invokes the function with optionally supplied arguments for the JS.invoke logic function.
    JavaScript::Value InvokeFunction(const std::string& function_name,
                                     size_t number_function_arguments, const JavaScript::Value* js_function_arguments);

    // Returns whether a value associated with the given variable or property is defined (not undefined).
    bool HasPropertyValue(const std::string& name) noexcept;

    // Returns the evaluated value for the variable or property with the supplied name.
    JavaScript::Value GetValue(const std::string& name);

    // Sets the value of the variable or property using the supplied value.
    void SetValue(const std::string& name, JavaScript::Value js_value);

    // Returns the JSON for the variable or property with the supplied name.
    std::string GetValueJson(const std::string& name);

    // Sets the value of the variable or property using the supplied JSON.
    void SetValueFromJson(const std::string& name, const std::string& json_text, bool& exception_is_from_json_parsing);

    // A list of the symbol types that can be used as part of a call to JS.invoke.
    static constexpr SymbolType SymbolTypesAllowedAsArguments[]
    {
        SymbolType::Array,
        SymbolType::HashMap,
        SymbolType::List,
        SymbolType::UserFunction,
        SymbolType::WorkString,
        SymbolType::WorkVariable
    };

    static bool IsSymbolTypeAllowedAsArgument(SymbolType symbol_type);

    // Routines to create JavaScript values from engine values.
    // Numeric values are serialized as null for NOTAPPL and as a string for other special values.
    JavaScript::Value CreateValue(double value);
    JavaScript::Value CreateValue(std::string_view value_sv);
    JavaScript::Value CreateValue(const std::string& value);
    JavaScript::Value CreateValue(const SharableString& value);
    JavaScript::Value CreateValue(const std::variant<double, SharableString>& value);
    JavaScript::Value CreateValue(const Symbol& symbol);
    JavaScript::Value CreateValue(const UserFunction& user_function);

    // Routines to convert JavaScript values into engine values.
    double ConvertNumeric(const JavaScript::Value& js_value);
    SharableString ConvertString(const JavaScript::Value& js_value);
    void ConvertSymbol(const JavaScript::Value& js_value, Symbol& symbol);

    // Serializes the bytecode for compiled applications.
    void serialize(Serializer& ar);

private:
    size_t CompileScript(SharableString script, JavaScript::ModuleType module_type, const std::string& file_path,
                         bool evaluate_at_application_startup);

    struct BytecodeData;
    std::string EvaluateBytecode(const BytecodeData& bytecode_data);

    // Adds a module loader helper that returns bytecode from the compiled application rather than from the disk.
    class CompiledApplicationModuleLoaderHelper;
    void AddCompiledApplicationModuleLoaderHelper();

    // Hooks the print/console.log functions to log text to the listing file as warnings.
    // Based on the application settings, the Action Invoker is optionally added.
    enum class ExecutorEvaluationState { None, PrinterOnly, FullRuntimeAccess };
    class Printer;
    void EnsureExecutorEvaluationState(ExecutorEvaluationState state);

    // Executes a logic function, converting the JavaScript arguments to CSPro equivalents.
    class ArgumentEvaluator;
    JavaScript::Value ExecuteLogicUserFunction(InterpreterAccessor& interpreter_accessor, int user_function_symbol_index,
                                               size_t argc, const JavaScript::Value* js_argv);

private:
    EngineData& m_engineData;
    std::unique_ptr<JavaScript::Executor> m_executor;
    ExecutorEvaluationState m_executorEvaluationState;

    struct BytecodeData
    {
        std::string file_path;
        SharableString script;
        JavaScript::ModuleType module_type;
        bool evaluate_at_application_startup;
        JavaScript::Bytecode bytecode;
    };

    std::vector<BytecodeData> m_bytecodeData;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool EngineJavaScriptProcessor::IsSymbolTypeAllowedAsArgument(const SymbolType symbol_type)
{
    const auto& lookup = std::find(std::cbegin(SymbolTypesAllowedAsArguments), std::cend(SymbolTypesAllowedAsArguments), symbol_type);
    return ( lookup != std::cend(EngineJavaScriptProcessor::SymbolTypesAllowedAsArguments) );
}
