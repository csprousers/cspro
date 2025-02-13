#include "stdafx.h"
#include "Executor.h"
#include "ActionInvokerJS.h"
#include "ValueGetter.h"
#include <zToolsO/Hash.h>
#include <zUtilO/FileExtensions.h>


JavaScript::Executor::Executor(std::wstring root_directory)
    :   m_qjs(new QuickJSAccess),
        m_rootDirectory(std::move(root_directory)),
        m_printer(std::make_shared<DefaultPrinter>())
{
    ASSERT(m_rootDirectory.empty() || PortableFunctions::FileIsDirectory(m_rootDirectory));

    m_qjs->executor = this;
    m_qjs->rt = JS_NewRuntime();

    // add the module loader
    JS_SetModuleLoaderFunc(m_qjs->rt, QuickJSAccess::ModuleLoaderNameNormalizer, QuickJSAccess::ModuleLoader, nullptr);

    // set the interrupt handler
    JS_SetInterruptHandler(m_qjs->rt, QuickJSAccess::InterruptHandler, nullptr);

    // set up the initial context
    AddContext();
}


JavaScript::Executor::~Executor()
{
    RemoveContext();

    JS_FreeRuntime(m_qjs->rt);
}


void JavaScript::Executor::AddContext()
{
    m_qjs->ctx = JS_NewContext(m_qjs->rt);

    // add an entry to the context map
    {
        std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
        QuickJSAccess::context_map.emplace_back(m_qjs->ctx, this);
    }

    // add the print and console.log functions
    JSValue js_global_obj = JS_GetGlobalObject(m_qjs->ctx);

    JS_SetPropertyStr(m_qjs->ctx, js_global_obj, "print", JS_NewCFunction(m_qjs->ctx, QuickJSAccess::PrintEvaluator, "print", 1));

    m_qjs->js_console_object = JS_NewObject(m_qjs->ctx);
    JS_SetPropertyStr(m_qjs->ctx, m_qjs->js_console_object, "log", JS_NewCFunction(m_qjs->ctx, QuickJSAccess::PrintEvaluator, "print", 1));
    JS_SetPropertyStr(m_qjs->ctx, js_global_obj, "console", m_qjs->js_console_object);

    // potentially add the CS object
    if( m_actionInvokerJS != nullptr )
        m_actionInvokerJS->AddToGlobalObject(m_qjs->ctx, js_global_obj);

    JS_FreeValue(m_qjs->ctx, js_global_obj);

    ASSERT(m_wrappedModuleFunctions.empty());
}


void JavaScript::Executor::RemoveContext()
{
    // remove the entry from the context map
    {
        std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
        const auto& context_search = std::find_if(QuickJSAccess::context_map.cbegin(), QuickJSAccess::context_map.cend(),
                                                  [&](const auto& cm) { return ( std::get<1>(cm) == this ); });
        ASSERT(context_search != QuickJSAccess::context_map.cend());
        QuickJSAccess::context_map.erase(context_search);
    }

    ASSERT(!JS_IsJobPending(m_qjs->rt));

    JS_FreeContext(m_qjs->ctx);

    m_wrappedModuleFunctions.clear();
}


void JavaScript::Executor::Reset()
{
    // remove any pending cancellation requests
    {
        std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
        QuickJSAccess::runtime_interrupt_requests.erase(m_qjs->rt);
    }

    // reset the context
    RemoveContext();
    AddContext();

    // set the default printer
    m_printer = std::make_shared<DefaultPrinter>();
}


std::string JavaScript::Executor::EvaluateScript(const std::string& script, ModuleType module_type/* = ModuleType::Autodetect*/,
                                                 const std::wstring& filename/* = std::wstring()*/, int line_number/* = 1*/)
{
    int flags = GetFlagFromModuleType(script, module_type, filename);
    return EvaluateScript<std::string>(script, filename, line_number, flags);
}


JavaScript::ByteCode JavaScript::Executor::CompileScript(const std::string& script, ModuleType module_type/* = ModuleType::Autodetect*/,
                                                         const std::wstring& filename/* = std::wstring()*/, int line_number/* = 1*/)
{
    int flags = JS_EVAL_FLAG_COMPILE_ONLY | GetFlagFromModuleType(script, module_type, filename);
    return CompileScript<ByteCode>(script, filename, line_number, flags);
}


void JavaScript::Executor::CompileScriptOnly(const std::string& script, ModuleType module_type/* = ModuleType::Autodetect*/,
                                             const std::wstring& filename/* = std::wstring()*/, int line_number/* = 1*/)
{
    int flags = JS_EVAL_FLAG_COMPILE_ONLY | GetFlagFromModuleType(script, module_type, filename);
    CompileScript<bool>(script, filename, line_number, flags);
}


std::string JavaScript::Executor::EvaluateFile(const std::wstring& filename, ModuleType module_type/* = ModuleType::Autodetect*/)
{
    return EvaluateScript(FileIO::ReadText<std::string>(filename), module_type, filename);
}


JavaScript::ByteCode JavaScript::Executor::CompileFile(const std::wstring& filename, ModuleType module_type/* = ModuleType::Autodetect*/)
{
    return CompileScript(FileIO::ReadText<std::string>(filename), module_type, filename);
}


template<typename JSValueWrapper>
void JavaScript::Executor::ProcessPostEvaluationResult(const JSValueWrapper& js_result)
{
    if( JS_IsException(js_result) )
        m_qjs->ThrowException();

    // execute any pending jobs (which should be async tasks)
    JSContext* pending_job_ctx;
    int pending_job_error;

    while( ( pending_job_error = JS_ExecutePendingJob(m_qjs->rt, &pending_job_ctx) ) != 0 )
    {
        if( pending_job_error < 0 )
            m_qjs->ThrowException();
    }
}


std::string JavaScript::Executor::EvaluateByteCode(const ByteCode& byte_code)
{
    JSValue js_object = m_qjs->ByteCodeToObject(byte_code);

    JSValue js_result = JS_EvalFunction(m_qjs->ctx, js_object);

    ProcessPostEvaluationResult(js_result);

    std::string result = m_qjs->GetString(js_result);

    JS_FreeValue(m_qjs->ctx, js_result);

    return result;
}


void JavaScript::Executor::CancelEvaluation()
{
    std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
    QuickJSAccess::runtime_interrupt_requests.insert(m_qjs->rt);

    // set the current cancelation flag to true
    if( m_currentCancelationFlag != nullptr )
    {
        *m_currentCancelationFlag = true;
        m_currentCancelationFlag.reset();
    }
}


std::vector<std::string> JavaScript::Executor::LoadModule(const std::wstring& filename)
{
    std::string script = FileIO::ReadText<std::string>(filename);

    JSValue js_result = EvaluateScript<JSValue>(script, filename, 1, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    JSModuleDef* js_module_def = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(js_result));

    // get a list of exported names
    int export_entry_count = csjs_get_export_entry_count(js_module_def);
    std::vector<std::string> exported_names;

    if( export_entry_count > 0 )
    {
        auto js_export_entry_names = std::make_unique<JSAtom[]>(export_entry_count);
        csjs_get_export_entry_names(js_module_def, js_export_entry_names.get());

        for( int i = 0; i < export_entry_count; ++i )
        {
            const char* name = JS_AtomToCString(m_qjs->ctx, js_export_entry_names[i]);
            exported_names.emplace_back(name);
            JS_FreeCString(m_qjs->ctx, name);
        }
    }

    JS_FreeValue(m_qjs->ctx, js_result);

    return exported_names;
}


std::vector<std::tuple<std::string, std::string>> JavaScript::Executor::GetGlobalFunctionDefinitions()
{
    std::vector<std::tuple<std::string, std::string>> function_definitions;

    JSValue js_global_obj = JS_GetGlobalObject(m_qjs->ctx);

    uint32_t properties_count;
    JSPropertyEnum* js_properties;

    if( JS_GetOwnPropertyNames(m_qjs->ctx, &js_properties, &properties_count, js_global_obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0 )
    {
        JS_FreeValue(m_qjs->ctx, js_global_obj);
        m_qjs->ThrowException();
    }

    for( uint32_t i = 0; i < properties_count; ++i )
    {
        JSPropertyEnum* js_property = js_properties + i;
        JSValue js_property_value = JS_GetProperty(m_qjs->ctx, js_global_obj, js_property->atom);

        if( JS_IsFunction(m_qjs->ctx, js_property_value) )
        {
            const char* name = JS_AtomToCString(m_qjs->ctx, js_property->atom);

            if( strcmp(name, "print") != 0 )
                function_definitions.emplace_back(name, m_qjs->GetString(js_property_value));

            JS_FreeCString(m_qjs->ctx, name);
        }

        JS_FreeValue(m_qjs->ctx, js_property_value);

        JS_FreeAtom(m_qjs->ctx, js_property->atom);
    }

    js_free(m_qjs->ctx, js_properties);

    JS_FreeValue(m_qjs->ctx, js_global_obj);

    return function_definitions;
}


void JavaScript::Executor::SetModuleLoaderHelper(std::shared_ptr<ModuleLoaderHelper> module_loader_helper)
{
    m_moduleLoaderHelper = std::move(module_loader_helper);
}


std::shared_ptr<JavaScript::Printer> JavaScript::Executor::SetPrinter(std::shared_ptr<Printer> printer)
{
    ASSERT(printer != nullptr);
    std::swap(m_printer, printer);
    return printer;
}


std::wstring JavaScript::Executor::GetRelativeFilename(std::wstring filename)
{
    // base the filename off the root directory (so that full paths are not in the compiled byte code)
    if( !m_rootDirectory.empty() )
    {
        if( m_fakeFilenameInRootDirectory.empty() )
            m_fakeFilenameInRootDirectory = PortableFunctions::PathAppendToPath(m_rootDirectory, _T("g"));

        filename = GetRelativeFNameForDisplay(m_fakeFilenameInRootDirectory, filename);
    }

    // use forward slashes for the path
    return PortableFunctions::PathToForwardSlash(filename);
}


int JavaScript::Executor::GetFlagFromModuleType(const std::string& script, ModuleType module_type, const std::wstring& filename)
{
    if( module_type == ModuleType::Global )
    {
        return JS_EVAL_TYPE_GLOBAL;
    }

    else if( module_type == ModuleType::Module )
    {
        return JS_EVAL_TYPE_MODULE;
    }

    else
    {
        ASSERT(module_type == ModuleType::Autodetect);

        if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(filename), FileExtensions::JavaScriptModule) ||
            JS_DetectModule(script.c_str(), script.length()) )
        {
            return JS_EVAL_TYPE_MODULE;
        }

        else
        {
            return JS_EVAL_TYPE_GLOBAL;
        }
    }
}


template<typename T>
T JavaScript::Executor::EvaluateScript(const std::string& script, const std::wstring& filename, int line_number, int flags)
{
    std::string utf8_filename = !filename.empty() ? UTF8Convert::WideToUTF8(GetRelativeFilename(filename)) :
                                                    std::string(QuickJSAccess::UnnamedScriptFilename);

    // evaluate the script
    JSValue js_result = JS_Eval2(m_qjs->ctx, script.data(), script.length(), utf8_filename.c_str(), flags, line_number);

    ProcessPostEvaluationResult(js_result);

    if constexpr(std::is_same_v<T, JSValue>)
    {
        return js_result;
    }

    else
    {
        std::string result = m_qjs->GetString(js_result);
        JS_FreeValue(m_qjs->ctx, js_result);
        return result;
    }
}


template<typename T>
T JavaScript::Executor::CompileScript(const std::string& script, const std::wstring& filename, int line_number, int flags)
{
    ASSERT(( flags & JS_EVAL_FLAG_COMPILE_ONLY ) != 0);

    JSValue js_result = EvaluateScript<JSValue>(script, filename, line_number, flags);

    if constexpr(std::is_same_v<T, ByteCode>)
    {
        ByteCode byte_code = m_qjs->ObjectToByteCode(js_result);

        JS_FreeValue(m_qjs->ctx, js_result);

        return byte_code;
    }

    else
    {
        JS_FreeValue(m_qjs->ctx, js_result);

        return true;
    }
}


std::string JavaScript::Executor::ExecuteFunctionWorker(const std::wstring& module_filename, const std::wstring& function_name,
                                                        size_t number_function_arguments, const Value* function_arguments)
{
    const std::wstring* function_name_to_execute = &function_name;

    // when using a module, wrap the function in a globally-accessible function
    if( !module_filename.empty() )
    {
        size_t hash_value = 0;
        Hash::Combine(hash_value, SO::ToUpper(module_filename));
        Hash::Combine(hash_value, function_name);
        Hash::Combine(hash_value, number_function_arguments);

        // the function may have already been wrapped
        const auto& lookup = m_wrappedModuleFunctions.find(hash_value);

        if( lookup != m_wrappedModuleFunctions.cend() )
        {
            function_name_to_execute = &lookup->second;
        }

        // if not, wrap it
        else
        {
            std::wstring wrapped_function_name = FormatTextCS2WS(_T("cspro_f%d"), m_wrappedModuleFunctions.size());
            std::wstring function_parameters;

            for( size_t i = 0; i < number_function_arguments; ++i )
                SO::AppendFormat(function_parameters, _T("%sa%d"), ( i == 0 ) ? _T("") : _T(","), i);

            const std::wstring script = FormatTextCS2WS(_T("import { %s } from \"%s\";\n")
                                                        _T("globalThis.%s = function(%s) { return %s(%s); };"),
                                                        function_name.c_str(),
                                                        Encoders::ToEscapedString(module_filename).c_str(),
                                                        wrapped_function_name.c_str(),
                                                        function_parameters.c_str(),
                                                        function_name.c_str(),
                                                        function_parameters.c_str());

            JSValue js_result = EvaluateScript<JSValue>(UTF8Convert::WideToUTF8(script), std::wstring(), 1, JS_EVAL_TYPE_MODULE);

            if( JS_IsException(js_result) )
                m_qjs->ThrowException();

            JS_FreeValue(m_qjs->ctx, js_result);

            function_name_to_execute = &m_wrappedModuleFunctions.try_emplace(hash_value, std::move(wrapped_function_name)).first->second;
        }
    }

    JSValue js_global_obj = JS_GetGlobalObject(m_qjs->ctx);
    JSValue js_function_property = JS_GetPropertyStr(m_qjs->ctx, js_global_obj, UTF8Convert::WideToUTF8(*function_name_to_execute).c_str());

    if( JS_IsUndefined(js_function_property) )
    {
        JS_FreeValue(m_qjs->ctx, js_global_obj);
        throw Exception(UTF8Convert::WideToUTF8(FormatText(_T("No function named '%s' found."), function_name.c_str())).c_str());
    }

    std::unique_ptr<JSValue[]> js_function_arguments;

    if( number_function_arguments != 0 )
    {
        ASSERT(function_arguments != nullptr);

        js_function_arguments = std::make_unique_for_overwrite<JSValue[]>(number_function_arguments);

        for( size_t i = 0; i < number_function_arguments; ++i )
            js_function_arguments[i] = function_arguments[i].GetValue();
    }

    JSValue js_function_result = JS_Call(m_qjs->ctx, js_function_property, js_global_obj, number_function_arguments, js_function_arguments.get());
    std::optional<std::string> result;

    if( !JS_IsException(js_function_result) )
    {
        result.emplace(m_qjs->GetString(js_function_result));
        JS_FreeValue(m_qjs->ctx, js_function_result);
    }

    JS_FreeValue(m_qjs->ctx, js_function_property);
    JS_FreeValue(m_qjs->ctx, js_global_obj);

    if( !result.has_value() )
        m_qjs->ThrowException();

    return *result;
}
