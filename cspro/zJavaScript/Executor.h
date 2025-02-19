#pragma once

#include <zJavaScript/zJavaScript.h>
#include <zJavaScript/Definitions.h>
#include <zJavaScript/Overrides.h>
#include <zJavaScript/Value.h>

class CancelFlag;
namespace Logic { struct FunctionDetails; enum class FunctionNamespace : int; }
namespace JavaScript { class Executor; }


// --------------------------------------------------------------------------
// JavaScript::Executor
//
// JavaScript::Exception exceptions are thrown on JavaScript errors.
//
// FileIO::Exception exceptions are thrown when there are errors reading
// files.
// --------------------------------------------------------------------------

class ZJAVASCRIPT_API JavaScript::Executor
{
    friend struct QuickJSAccess;

public:
    Executor(std::string root_directory = std::string());
    ~Executor();

    // Creates a new context, sets the default printer, and cancels any any pending cancelation requests.
    void Reset();

    // Evaluates the script.
    std::string EvaluateScript(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                               const std::string& file_path = std::string(), int line_number = 1);

    // Compiles the script, returning the bytecode for later evaluation.
    Bytecode CompileScript(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                           const std::string& file_path = std::string(), int line_number = 1);

    // Compiles the script.
    void CompileScriptOnly(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                           const std::string& file_path = std::string(), int line_number = 1);

    // Loads the script from the file and evaluates it.
    std::string EvaluateFile(const std::string& file_path, ModuleType module_type = ModuleType::Autodetect);

    // Loads the script from the file and compiles it.
    Bytecode CompileFile(const std::string& file_path, ModuleType module_type = ModuleType::Autodetect);

    // Evaluates the bytecode.
    std::string EvaluateBytecode(const Bytecode& bytecode);

    // Returns or sets the current cancelation flag.
    CancelFlag* GetCancelFlag()                 { return m_cancelFlag; }
    void SetCancelFlag(CancelFlag* cancel_flag) { m_cancelFlag = cancel_flag; }

    // Cancels any script evaluation in process.
    void CancelEvaluation();

    // Returns the names of exported functions (and classes).
    std::vector<std::string> LoadModule(const std::string& file_path);

    // Returns the names and the function definitions in the global object.
    std::vector<std::tuple<std::string, std::string>> GetGlobalFunctionDefinitions();

    // Sets a module loader helper.
    void SetModuleLoaderHelper(std::shared_ptr<ModuleLoaderHelper> module_loader_helper);

    // Sets an object to be used to keep track of module loading errors.
    void SetModuleLoaderErrorTracker(std::shared_ptr<std::vector<std::tuple<std::string, std::string>>> module_loading_errors);

    // Overrides the default print behavior, returning the previous printer.
    std::shared_ptr<Printer> SetPrinter(std::shared_ptr<Printer> printer);

    // Adds the CS object to the global object with the defined functions. The "CS" name can be overridden.
    void UseActionInvoker(const std::vector<const Logic::FunctionDetails*>& functions, const std::map<Logic::FunctionNamespace, const char*>& namespace_names,
                          const char* object_name_override = nullptr);

    // Creates a JavaScript::Value (a wrapper around JSValue) from the specified value.
    template<typename T>
    Value CreateValue(T&& value);

    // Converts the JavaScript value, throwing exceptions on error.
    template<typename T>
    T ConvertValue(const Value& value);

    // Creates an engine-style value from the specified value.
    Value CreateEngineValue(double value);
    Value CreateEngineValue(const SharableString& value) { return CreateValue(value.GetString()); }

    // Converts the JavaScript value to an engine-style value, throwing exceptions on error.
    template<typename T>
    T ConvertEngineValue(const Value& value);

    // Creates an array from the specified values.
    Value CreateArray(size_t size, const Value* data);

    // Returns the length of an array, throwing an exception is the value is not an array.
    uint32_t GetArrayLength(const Value& array_value);

    // Returns the element of an array, throwing an exception on error.
    // It is assumed that array_value is an array.
    Value GetArrayElement(const Value& array_value, uint32_t index);

    // Creates an object.
    Value CreateObject();

    // Returns the names of an object's properties, throwing an exception on error.
    // It is assumed that object_value is an object.
    std::vector<std::string> GetObjectPropertyNames(const Value& object_value);

    // Returns the value of an object's property with the given name, throwing an exception on error.
    // It is assumed that object_value is an object.
    Value GetObjectPropertyValue(const Value& object_value, std::string_view name_sv);

    // Returns the names and values of an object's properties, throwing an exception on error.
    // It is assumed that object_value is an object.
    std::vector<std::tuple<std::string, Value>> GetObjectPropertyNamesAndValues(const Value& object_value);

    // Sets an object's property.
    void SetObjectProperty(Value& object_value, std::string_view name_sv, Value property_value);

    // Creates an unnamed function that when invoked executes the callback function.
    Value CreateFunction(std::function<Value(int argc, const Value* argv)> callback_function);

    // Returns the result of JSON.stringify called on the value.
    std::string GetJsonForValue(const Value& value);

    // Creates a value from the supplied JSON.
    Value CreateValueFromJson(const std::string& json_text);

    // Returns whether a value associated with the given variable or property is defined (not undefined).
    bool HasPropertyValue(const std::string& name) noexcept;

    // Returns the value associated with the given variable or property.
    // An exception is thrown if the value is undefined.
    Value GetPropertyValue(const std::string& name);

    // Sets the value of the given variable or property.
    void SetPropertyValue(const std::string& name, Value value);

    // Invokes the function with optionally supplied arguments.
    Value InvokeFunction(const std::string& function_name,
                         size_t number_function_arguments, const Value* function_arguments);

    // Executes the function, converting any arguments to JavaScript values using the CreateValue method.
    template<typename... Args>
    std::string ExecuteFunction(const std::string& module_file_path, const std::string& function_name, Args const&... arguments);

private:
    void AddContext();
    void RemoveContext();

    template<typename JSValueWrapper>
    void ProcessPostEvaluationResult(const JSValueWrapper& js_result);

    std::string GetRelativeFilePath(std::string file_path);

    static int GetFlagFromModuleType(const std::string& script, ModuleType module_type, const std::string& file_path);

    // EvaluateScript will throw an exception on error.
    template<typename T>
    T EvaluateScript(const std::string& script, const std::string& file_path, int line_number, int flags);

    template<typename T>
    T CompileScript(const std::string& script, const std::string& file_path, int line_number, int flags);

    // Converts an array of Value wrappers to JSValue values, returning std::unique_ptr<JSValue[]>.
    // The JSValue values are not duplicated, so the lifetime of the Value wrapper must outlive the
    // use of the JSValue values.
    static auto GetJSValueArray(size_t number_values, const Value* values);

    template<typename AtomT>
    JavaScript::Value GetObjectPropertyValueWorker(const Value& object_value, const AtomT& name_atom);

    template<typename T>
    std::vector<T> GetObjectPropertyNamesWorker(const Value& object_value);

    template<typename T, typename... Args>
    void ParseFunctionArguments(Value* function_arguments, const T& argument1, Args const&... arguments);

    std::string ExecuteFunctionWorker(const std::string& module_file_path, const std::string& function_name,
                                      size_t number_function_arguments, const Value* function_arguments);

private:
    std::unique_ptr<QuickJSAccess> m_qjs;

    std::string m_rootDirectory;
    std::string m_fakeFilePathInRootDirectory;

    std::shared_ptr<Printer> m_printer;
    std::shared_ptr<ModuleLoaderHelper> m_moduleLoaderHelper;
    std::shared_ptr<std::vector<std::tuple<std::string, std::string>>> m_moduleLoadingErrors;

    std::map<size_t, std::string> m_wrappedModuleFunctions;

    struct CallbackFunctionExecutor;
    std::vector<std::function<Value(int, const Value*)>> m_callbackFunctions;

    class ActionInvokerJSCaller;
    struct ActionInvokerJS;
    std::unique_ptr<ActionInvokerJS> m_actionInvokerJS;
    int m_actionInvokerCallerId;

    CancelFlag* m_cancelFlag;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T, typename... Args>
void JavaScript::Executor::ParseFunctionArguments(Value* const function_arguments, const T& argument1, Args const&... arguments)
{
    ASSERT(function_arguments != nullptr);
    *function_arguments = CreateValue(argument1);

    if constexpr(sizeof...(Args) != 0)
    {
        ParseFunctionArguments(function_arguments + 1, arguments...);
    }
}


template<typename... Args>
std::string JavaScript::Executor::ExecuteFunction(const std::string& module_file_path, const std::string& function_name, Args const&... arguments)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return ExecuteFunctionWorker(module_file_path, function_name, 0, nullptr);
    }

    else
    {
        Value function_arguments[sizeof...(Args)];
        ParseFunctionArguments(function_arguments, arguments...);

        return ExecuteFunctionWorker(module_file_path, function_name, _countof(function_arguments), function_arguments);
    }
}


template<typename T>
JavaScript::Value JavaScript::Executor::CreateValue(T&& value)
{
    return Value(*m_qjs, std::forward<T>(value));
}
