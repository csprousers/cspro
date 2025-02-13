#pragma once

#include <zJavaScript/zJavaScript.h>
#include <zJavaScript/Definitions.h>
#include <zJavaScript/Overrides.h>
#include <zJavaScript/Value.h>

namespace Logic { struct FunctionDetails; enum class FunctionNamespace : int; }


namespace JavaScript
{
    // JavaScript::Exception exceptions are thrown on JavaScript errors
    // FileIO::Exception exceptions are thrown when there is an error reading files

    class ZJAVASCRIPT_API Executor
    {
        friend struct QuickJSAccess;

    public:
        Executor(std::wstring root_directory = std::wstring());
        ~Executor();

        // creates a new context, sets the default printer, and cancels any any pending cancellation requests
        void Reset();

        // evaluates the script
        std::string EvaluateScript(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                                   const std::wstring& filename = std::wstring(), int line_number = 1);

        // compiles the script, returning the bytecode for later evaluation
        ByteCode CompileScript(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                               const std::wstring& filename = std::wstring(), int line_number = 1);

        // compiles the script
        void CompileScriptOnly(const std::string& script, ModuleType module_type = ModuleType::Autodetect,
                               const std::wstring& filename = std::wstring(), int line_number = 1);

        // loads the script from the file and evaluates it
        std::string EvaluateFile(const std::wstring& filename, ModuleType module_type = ModuleType::Autodetect);

        // loads the script from the file and compiles it
        ByteCode CompileFile(const std::wstring& filename, ModuleType module_type = ModuleType::Autodetect);

        // evaluates bytecode
        std::string EvaluateByteCode(const ByteCode& byte_code);

        // cancels any script evaluation in process
        void CancelEvaluation();

        // returns the names of exported functions (and classes)
        std::vector<std::string> LoadModule(const std::wstring& filename);

        // returns the names and the function definitions in the global object
        std::vector<std::tuple<std::string, std::string>> GetGlobalFunctionDefinitions();

        // sets a module loader helper
        void SetModuleLoaderHelper(std::shared_ptr<ModuleLoaderHelper> module_loader_helper);

        // overrides the default print behavior, returning the previous printer
        std::shared_ptr<Printer> SetPrinter(std::shared_ptr<Printer> printer);

        // adds the CS object to the global object with the defined functions
        void UseActionInvoker(const std::vector<const Logic::FunctionDetails*>& functions, const std::map<Logic::FunctionNamespace, const TCHAR*>& namespace_names);

        // creates a JavaScript::Value (a wrapper around JSValue) from the specified value
        template<typename T>
        Value CreateValue(const T& value) { return Value(*m_qjs, value); }

        // executes the function, converting any arguments to JavaScript values using the CreateValue method
        template<typename... Args>
        std::string ExecuteFunction(const std::wstring& module_filename, const std::wstring& function_name, Args const&... arguments);

    private:
        void AddContext();
        void RemoveContext();

        std::wstring GetRelativeFilename(std::wstring filename);

        static int GetFlagFromModuleType(const std::string& script, ModuleType module_type, const std::wstring& filename);

        // EvaluateScript will throw an exception on error;
        // otherwise, the calling function must free the result if a JSValue object is returned
        template<typename T>
        T EvaluateScript(const std::string& script, const std::wstring& filename, int line_number, int flags);

        template<typename T>
        T CompileScript(const std::string& script, const std::wstring& filename, int line_number, int flags);

        template<typename T, typename... Args>
        void ParseFunctionArguments(Value* function_arguments, const T& argument1, Args const&... arguments);

        std::string ExecuteFunctionWorker(const std::wstring& module_filename, const std::wstring& function_name,
                                          size_t number_function_arguments, const Value* function_arguments);

        template<typename JSValueWrapper>
        void ProcessPostEvaluationResult(const JSValueWrapper& js_result);

    private:
        std::unique_ptr<QuickJSAccess> m_qjs;

        std::wstring m_rootDirectory;
        std::wstring m_fakeFilenameInRootDirectory;

        std::shared_ptr<Printer> m_printer;
        std::shared_ptr<ModuleLoaderHelper> m_moduleLoaderHelper;

        std::map<size_t, std::wstring> m_wrappedModuleFunctions;

        class ActionInvokerJSCaller;
        struct ActionInvokerJS;
        std::unique_ptr<ActionInvokerJS> m_actionInvokerJS;

        std::shared_ptr<bool> m_currentCancelationFlag;
   };
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T, typename... Args>
void JavaScript::Executor::ParseFunctionArguments(Value* function_arguments, const T& argument1, Args const&... arguments)
{
    ASSERT(function_arguments != nullptr);
    *function_arguments = CreateValue(argument1);

    if constexpr(sizeof...(Args) != 0)
    {
        ParseFunctionArguments(function_arguments + 1, arguments...);
    }
}


template<typename... Args>
std::string JavaScript::Executor::ExecuteFunction(const std::wstring& module_filename, const std::wstring& function_name, Args const&... arguments)
{
    if constexpr(sizeof...(Args) == 0)
    {
        return ExecuteFunctionWorker(module_filename, function_name, 0, nullptr);
    }

    else
    {
        Value function_arguments[sizeof...(Args)];
        ParseFunctionArguments(function_arguments, arguments...);

        return ExecuteFunctionWorker(module_filename, function_name, _countof(function_arguments), function_arguments);
    }
}
