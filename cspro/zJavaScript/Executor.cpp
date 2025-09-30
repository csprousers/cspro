#include "stdafx.h"
#include "Executor.h"
#include "ActionInvokerJS.h"
#include "Atom.h"
#include "ValueInternal.h"
#include "VariablePropertyNameEvaluator.h"
#include <zToolsO/Hash.h>
#include <zToolsO/UniqueId.h>
#include <zUtilO/FileExtensions.h>


JavaScript::Executor::Executor(std::string root_directory/* = std::string()*/)
    :   m_qjs(new QuickJSAccess),
        m_rootDirectory(std::move(root_directory)),
        m_printer(std::make_unique<DefaultPrinter>()),
        m_actionInvokerCallerId(UniqueId::CreateInt()),
        m_cancelFlag(nullptr)
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
        const std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
        QuickJSAccess::context_map.emplace_back(m_qjs->ctx, this);
    }

    // add the print and console.log functions
    const GlobalObjectValue js_global_obj(m_qjs);

    JS_SetPropertyStr(m_qjs->ctx, *js_global_obj, "print", JS_NewCFunction(m_qjs->ctx, QuickJSAccess::PrintEvaluator, "print", 1));

    m_qjs->js_console_object = JS_NewObject(m_qjs->ctx);
    JS_SetPropertyStr(m_qjs->ctx, m_qjs->js_console_object, "log", JS_NewCFunction(m_qjs->ctx, QuickJSAccess::PrintEvaluator, "print", 1));
    JS_SetPropertyStr(m_qjs->ctx, *js_global_obj, "console", m_qjs->js_console_object);

    // potentially add the CS object
    if( m_actionInvokerJS != nullptr )
        m_actionInvokerJS->AddToGlobalObject(m_qjs->ctx, *js_global_obj);

    ASSERT(m_wrappedModuleFunctions.empty());
}


void JavaScript::Executor::RemoveContext()
{
    // remove the entry from the context map
    {
        const std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
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
    // remove any pending cancelation requests
    {
        const std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
        QuickJSAccess::runtime_interrupt_requests.erase(m_qjs->rt);
    }

    // reset the context
    RemoveContext();
    AddContext();

    // set the default printer
    m_printer = std::make_unique<DefaultPrinter>();
}


std::string JavaScript::Executor::EvaluateScript(const std::string& script, const ModuleType module_type/* = ModuleType::Autodetect*/,
                                                 const std::string& file_path/* = std::string()*/, const int line_number/* = 1*/)
{
    const int flags = GetFlagFromModuleType(script, module_type, file_path);
    return EvaluateScript<std::string>(script, file_path, line_number, flags);
}


JavaScript::Bytecode JavaScript::Executor::CompileScript(const std::string& script, const ModuleType module_type/* = ModuleType::Autodetect*/,
                                                         const std::string& file_path/* = std::string()*/, const int line_number/* = 1*/)
{
    const int flags = JS_EVAL_FLAG_COMPILE_ONLY | GetFlagFromModuleType(script, module_type, file_path);
    return CompileScript<Bytecode>(script, file_path, line_number, flags);
}


void JavaScript::Executor::CompileScriptOnly(const std::string& script, const ModuleType module_type/* = ModuleType::Autodetect*/,
                                             const std::string& file_path/* = std::string()*/, const int line_number/* = 1*/)
{
    const int flags = JS_EVAL_FLAG_COMPILE_ONLY | GetFlagFromModuleType(script, module_type, file_path);
    CompileScript<void>(script, file_path, line_number, flags);
}


std::string JavaScript::Executor::EvaluateFile(const std::string& file_path, const ModuleType module_type/* = ModuleType::Autodetect*/)
{
    return EvaluateScript(FileIO::ReadText(file_path), module_type, file_path);
}


JavaScript::Bytecode JavaScript::Executor::CompileFile(const std::string& file_path, const ModuleType module_type/* = ModuleType::Autodetect*/)
{
    return CompileScript(FileIO::ReadText(file_path), module_type, file_path);
}


template<typename JSValueWrapper>
void JavaScript::Executor::ProcessPostEvaluationResult(const JSValueWrapper& js_result)
{
    std::optional<Exception> exception;

    // only throw the exception after processing any pending jobs, otherwise QuickJS will
    // be in a bad state with jobs pending
    if( JS_IsException(js_result) )
        exception = m_qjs->CreateException();

    // execute any pending jobs (which should be async tasks)
    JSContext* pending_job_ctx;
    int pending_job_error;

    while( ( pending_job_error = JS_ExecutePendingJob(m_qjs->rt, &pending_job_ctx) ) != 0 )
    {
        if( pending_job_error < 0 && !exception.has_value() )
            exception = m_qjs->CreateException();
    }

    if( exception.has_value() )
        throw *exception;
}


std::string JavaScript::Executor::EvaluateBytecode(const Bytecode& bytecode)
{
    const JSValue js_object = m_qjs->BytecodeToObject(bytecode);

    const Value js_result(m_qjs, JS_EvalFunction(m_qjs->ctx, js_object));

    ProcessPostEvaluationResult(*js_result);

    return js_result.ToString();
}


void JavaScript::Executor::CancelEvaluation()
{
    const std::lock_guard<std::mutex> lock(QuickJSAccess::context_map_mutex);
    QuickJSAccess::runtime_interrupt_requests.insert(m_qjs->rt);
}


std::vector<std::string> JavaScript::Executor::LoadModule(const std::string& file_path)
{
    const std::string script = FileIO::ReadText(file_path);

    const Value js_result = EvaluateScript<Value>(script, file_path, 1, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    JSModuleDef* const js_module_def = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(*js_result));

    // get a list of exported names
    const int export_entry_count = csjs_get_export_entry_count(js_module_def);
    std::vector<std::string> exported_names;

    if( export_entry_count > 0 )
    {
        auto js_export_entry_names = std::make_unique_for_overwrite<JSAtom[]>(export_entry_count);
        csjs_get_export_entry_names(js_module_def, js_export_entry_names.get());

        for( int i = 0; i < export_entry_count; ++i )
            exported_names.emplace_back(m_qjs->GetString(js_export_entry_names[i]));
    }

    return exported_names;
}


std::vector<std::tuple<std::string, std::string>> JavaScript::Executor::GetGlobalFunctionDefinitions()
{
    std::vector<std::tuple<std::string, std::string>> function_definitions;

    const GlobalObjectValue js_global_obj(m_qjs);

    uint32_t properties_count;
    JSPropertyEnum* js_properties;

    if( JS_GetOwnPropertyNames(m_qjs->ctx, &js_properties, &properties_count, *js_global_obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0 )
        m_qjs->ThrowException();

    for( uint32_t i = 0; i < properties_count; ++i )
    {
        JSPropertyEnum* const js_property = js_properties + i;
        const Value js_property_value(m_qjs, JS_GetProperty(m_qjs->ctx, *js_global_obj, js_property->atom));

        if( JS_IsFunction(m_qjs->ctx, *js_property_value) )
        {
            std::string name = m_qjs->GetString(js_property->atom);

            if( name != "print" )
                function_definitions.emplace_back(std::move(name), js_property_value.ToString());
        }
    }

    js_free_prop_enum(m_qjs->ctx, js_properties, properties_count);

    return function_definitions;
}


void JavaScript::Executor::SetModuleLoaderHelper(std::shared_ptr<ModuleLoaderHelper> module_loader_helper)
{
    ASSERT(m_moduleLoaderHelper == nullptr && module_loader_helper != nullptr);
    m_moduleLoaderHelper = std::move(module_loader_helper);
}


void JavaScript::Executor::SetModuleLoaderErrorTracker(std::shared_ptr<std::vector<std::tuple<std::string, std::string>>> module_loading_errors)
{
    ASSERT(m_moduleLoadingErrors == nullptr && module_loading_errors != nullptr);
    m_moduleLoadingErrors = std::move(module_loading_errors);
}


std::shared_ptr<JavaScript::Printer> JavaScript::Executor::SetPrinter(std::shared_ptr<Printer> printer)
{
    ASSERT(printer != nullptr);
    std::swap(m_printer, printer);
    return printer;
}


std::string JavaScript::Executor::GetRelativeFilePath(std::string file_path)
{
    // base the file path off the root directory (so that full paths are not in the compiled bytecode)
    if( !m_rootDirectory.empty() )
    {
        if( m_fakeFilePathInRootDirectory.empty() )
            m_fakeFilePathInRootDirectory = Path::Combine(m_rootDirectory, "g");

        file_path = GetRelativePathForDisplay(m_fakeFilePathInRootDirectory, file_path);
    }

    // use forward slashes for the path
    return Path::MakeToForwardSlash(file_path);
}


int JavaScript::Executor::GetFlagFromModuleType(const std::string& script, const ModuleType module_type, const std::string& file_path)
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

        if( Path::ExtensionMatches(file_path, FileExtensions::JavaScriptModule) ||
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
T JavaScript::Executor::EvaluateScript(const std::string& script, const std::string& file_path, const int line_number, const int flags)
{
    const std::string evaluated_file_path = !file_path.empty() ? GetRelativeFilePath(file_path) :
                                                                 std::string(QuickJSAccess::UnnamedScriptFilename_sv);

    // evaluate the script
    JSEvalOptions options =
    {
        JS_EVAL_OPTIONS_VERSION,
        flags,
        evaluated_file_path.c_str(),
        line_number
    };

    Value js_result(m_qjs, JS_Eval2(m_qjs->ctx, script.data(), script.length(), &options));
    ProcessPostEvaluationResult(*js_result);

    if constexpr(std::is_same_v<T, Value>)
    {
        return js_result;
    }

    else
    {
        return js_result.ToString();
    }
}


template<typename T>
T JavaScript::Executor::CompileScript(const std::string& script, const std::string& file_path, const int line_number, const int flags)
{
    ASSERT(( flags & JS_EVAL_FLAG_COMPILE_ONLY ) != 0);

    const Value js_result = EvaluateScript<Value>(script, file_path, line_number, flags);

    if constexpr(std::is_same_v<T, Bytecode>)
    {
        return m_qjs->ObjectToBytecode(*js_result);
    }
}


template<>
double JavaScript::Executor::ConvertValue(const Value& value)
{
    double converted_value;

    if( JS_ToFloat64(m_qjs->ctx, &converted_value, value.GetValue()) == -1 )
        m_qjs->ThrowException();

    if( std::isnan(converted_value) )
        throw Exception(FormatText("A value of type '%s' could not be converted to a CSPro numeric.", value.GetType()));

    return converted_value;
}


template<>
std::string JavaScript::Executor::ConvertValue(const Value& value)
{
    const JSValue& js_value = value.GetValue();

    if( !JS_IsString(js_value) )
    {
        // allow numbers and booleans to be converted to their string equivalent
        if( JS_IsNumber(js_value) || JS_IsBool(js_value) )
        {
            // fine
        }

        else
        {
            ASSERT(JS_IsUndefined(js_value) ||
                   JS_IsNull(js_value) ||
                   JS_IsObject(js_value));

            throw Exception(FormatText("A value of type '%s' could not be converted to a CSPro string.", value.GetType()));
        }
    }

    return value.ToString();
}


JavaScript::Value JavaScript::Executor::CreateEngineValue(const double value)
{
    return ( !IsSpecial(value) ) ? Value(*m_qjs, value) :
           ( value == NOTAPPL )  ? Value::Null() :
                                   CreateValue(std::string_view(SpecialValues::ValueToString(value)));
}


template<>
ZJAVASCRIPT_API double JavaScript::Executor::ConvertEngineValue(const Value& value)
{
    const JSValue& js_value = value.GetValue();

    if( !JS_IsNumber(js_value) )
    {
        if( JS_IsNull(js_value) )
            return NOTAPPL;

        if( JS_IsString(js_value) )
        {
            const double* const special_value = SpecialValues::StringIsSpecial<const double*>(value.ToString());

            if( special_value != nullptr )
                return *special_value;
        }
    }

    return ConvertValue<double>(value);
}


template<>
ZJAVASCRIPT_API SharableString JavaScript::Executor::ConvertEngineValue(const Value& value)
{
    if( JS_IsNull(value.GetValue()) )
        return SharableString();

    return ConvertValue<std::string>(value);
}


auto JavaScript::Executor::GetJSValueArray(size_t number_values, const Value* values)
{
    if( number_values == 0 )
        return std::unique_ptr<JSValue[]>();

    ASSERT(values != nullptr);

    auto js_values = std::make_unique_for_overwrite<JSValue[]>(number_values);
    JSValue* js_value_itr = js_values.get();

    for( ; number_values > 0; --number_values )
    {
        *js_value_itr = values->GetValue();
        ++js_value_itr;
        ++values;
    }

    return js_values;
}


JavaScript::Value JavaScript::Executor::CreateArray(const size_t size, const Value* const data)
{
    Value js_array(m_qjs, JS_NewArray(m_qjs->ctx));

    for( size_t i = 0; i < size; ++i )
        JS_DefinePropertyValueUint32(m_qjs->ctx, *js_array, i, data[i].Duplicate(), JS_PROP_C_W_E);

    return js_array;
}


uint32_t JavaScript::Executor::GetArrayLength(const Value& array_value)
{
    if( !JS_IsArray(array_value.GetValue()) )
        throw Exception(FormatText("A value of type '%s' is not an array.", array_value.GetType()));

    const Value js_length(m_qjs, JS_GetPropertyStr(m_qjs->ctx, array_value.GetValue(), "length"));
    uint32_t length;

    if( !JS_IsNumber(js_length.GetValue()) || JS_ToUint32(m_qjs->ctx, &length, js_length.GetValue()) == -1 )
        throw Exception("The array length is unknown.");

    return length;
}


JavaScript::Value JavaScript::Executor::GetArrayElement(const Value& array_value, const uint32_t index)
{
    ASSERT(array_value.IsArray() && index < GetArrayLength(array_value));

    Value js_element(m_qjs, JS_GetPropertyUint32(m_qjs->ctx, array_value.GetValue(), index));

    if( JS_IsException(js_element.GetValue()) )
        m_qjs->ThrowException();

    return js_element;
}


JavaScript::Value JavaScript::Executor::CreateObject()
{
    return Value(m_qjs, JS_NewObject(m_qjs->ctx));
}


template<typename AtomT>
JavaScript::Value JavaScript::Executor::GetObjectPropertyValueWorker(const Value& object_value, const AtomT& name_atom)
{
    ASSERT(object_value.IsObject());

    Value js_property_value(m_qjs, JS_GetProperty(m_qjs->ctx, *object_value, name_atom));

    if( JS_IsException(*js_property_value) )
        m_qjs->ThrowException();

    return js_property_value;
}


template<typename T>
std::vector<T> JavaScript::Executor::GetObjectPropertyNamesWorker(const Value& object_value)
{
    ASSERT(object_value.IsObject());

    uint32_t properties_count;
    JSPropertyEnum* js_properties;

    if( JS_GetOwnPropertyNames(m_qjs->ctx, &js_properties, &properties_count, object_value.GetValue(), JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0 )
        m_qjs->ThrowException();

    std::vector<T> property_names_and_potentially_values;
    property_names_and_potentially_values.reserve(properties_count);

    JSPropertyEnum* js_property_itr = js_properties;
    const JSPropertyEnum* const js_property_end = js_property_itr + properties_count;

    for( ; js_property_itr != js_property_end; ++js_property_itr )
    {
        if constexpr(std::is_same_v<T, std::string>)
        {
            property_names_and_potentially_values.emplace_back(m_qjs->GetString(js_property_itr->atom));
        }

        else
        {
            property_names_and_potentially_values.emplace_back(m_qjs->GetString(js_property_itr->atom),
                                                               GetObjectPropertyValueWorker(object_value, js_property_itr->atom));
        }
    }

    js_free_prop_enum(m_qjs->ctx, js_properties, properties_count);

    return property_names_and_potentially_values;
}


std::vector<std::string> JavaScript::Executor::GetObjectPropertyNames(const Value& object_value)
{
    return GetObjectPropertyNamesWorker<std::string>(object_value);
}


JavaScript::Value JavaScript::Executor::GetObjectPropertyValue(const Value& object_value, const std::string_view name_sv)
{
    const Atom js_name(*m_qjs, name_sv);

    return GetObjectPropertyValueWorker(object_value, *js_name);
}


std::vector<std::tuple<std::string, JavaScript::Value>> JavaScript::Executor::GetObjectPropertyNamesAndValues(const Value& object_value)
{
    return GetObjectPropertyNamesWorker<std::tuple<std::string, JavaScript::Value>>(object_value);
}


void JavaScript::Executor::SetObjectProperty(Value& object_value, const std::string_view name_sv, Value property_value)
{
    ASSERT(object_value.IsObject());

    const Atom js_name(*m_qjs, name_sv);

    if( JS_SetProperty(m_qjs->ctx, *object_value, *js_name, property_value.Release()) == -1 )
        m_qjs->ThrowException();
}


struct JavaScript::Executor::CallbackFunctionExecutor
{
    static JSValue Run(JSContext* const ctx, const JSValueConst /*this_val*/, const int argc, JSValueConst* const argv, int magic)
    {
        JavaScript::Executor& executor = JavaScript::QuickJSAccess::GetExecutorFromContext(ctx);
        ASSERT(static_cast<size_t>(magic) < executor.m_callbackFunctions.size());

        auto js_arguments = std::make_unique_for_overwrite<Value[]>(argc);

        for( int i = 0; i < argc; ++i )
        {
            new(js_arguments.get() + i) Value(executor.m_qjs, static_cast<const JSValue&>(argv[i]));
        }

        try
        {
            return executor.m_callbackFunctions[magic](argc, js_arguments.get()).Release();
        }

        catch( const std::exception& exception )
        {
            return JS_Throw(ctx, executor.m_qjs->NewError(exception, std::nullopt, std::nullopt));
        }
    }
};


JavaScript::Value JavaScript::Executor::CreateFunction(std::function<Value(int argc, const Value* argv)> callback_function)
{
    ASSERT(callback_function);

    const int callback_function_index = m_callbackFunctions.size();
    m_callbackFunctions.emplace_back(std::move(callback_function));

    return Value(m_qjs, JS_NewCFunctionMagic(m_qjs->ctx, CallbackFunctionExecutor::Run, nullptr, 1,
                                             JS_CFUNC_generic_magic, callback_function_index));
}


std::string JavaScript::Executor::GetJsonForValue(const Value& value)
{
    const Value js_json(m_qjs, JS_JSONStringify(m_qjs->ctx, value.GetValue(), JS_UNDEFINED, JS_UNDEFINED));

    if( JS_IsException(*js_json) )
        m_qjs->ThrowException();

    return js_json.ToString();
}


JavaScript::Value JavaScript::Executor::CreateValueFromJson(const std::string& json_text)
{
    Value js_value(m_qjs, JS_ParseJSON(m_qjs->ctx, json_text.c_str(), json_text.length(), ""));

    if( JS_IsException(*js_value) )
        m_qjs->ThrowException();

    return js_value;
}


bool JavaScript::Executor::HasPropertyValue(const std::string& name) noexcept
{
    try
    {
        GetPropertyValue(name);
        return true;
    }

    catch(...)
    {
        return false;
    }
}


JavaScript::Value JavaScript::Executor::GetPropertyValue(const std::string& name)
{
    VariablePropertyNameEvaluator::ForGetting name_evaluator(*m_qjs, name);

    if( JS_IsUndefined(name_evaluator.GetValue().GetValue()) )
        name_evaluator.ThrowException();

    return name_evaluator.ReleaseValue();
}


void JavaScript::Executor::SetPropertyValue(const std::string& name, Value value)
{
    const VariablePropertyNameEvaluator::ForSetting name_evaluator(*m_qjs, name);

    if( JS_SetProperty(m_qjs->ctx,
                       name_evaluator.GetObjectValue().GetValue(),
                       name_evaluator.GetLastParsedNameAtom(),
                       value.Release()) == -1 )
    {
        m_qjs->ThrowException();
    }
}


JavaScript::Value JavaScript::Executor::InvokeFunction(const std::string& function_name,
                                                       const size_t number_function_arguments, const Value* const function_arguments)
{
    std::optional<VariablePropertyNameEvaluator::ForGetting> name_evaluator;

    try
    {
        name_evaluator.emplace(*m_qjs, function_name);

        if( !JS_IsFunction(m_qjs->ctx, name_evaluator->GetValue().GetValue()) )
            name_evaluator.reset();
    }
    catch(...) { }

    if( !name_evaluator.has_value() )
        throw Exception(FormatText("No function named '%s' found.", function_name.c_str()));

    const std::unique_ptr<JSValue[]> js_function_arguments = GetJSValueArray(number_function_arguments, function_arguments);

    Value js_result(m_qjs, JS_Call(m_qjs->ctx,
                                   name_evaluator->GetValue().GetValue(),
                                   name_evaluator->IsObjectValueGlobalObject() ? JS_UNDEFINED : name_evaluator->GetObjectValue().GetValue(),
                                   number_function_arguments, js_function_arguments.get()));

    ProcessPostEvaluationResult(*js_result);

    return js_result;
}


std::string JavaScript::Executor::ExecuteFunctionWorker(const std::string& module_file_path, const std::string& function_name,
                                                        const size_t number_function_arguments, const Value* const function_arguments)
{
    const std::string* function_name_to_execute = &function_name;

    // when using a module, wrap the function in a globally-accessible function
    if( !module_file_path.empty() )
    {
        size_t hash_value = 0;
        Hash::Combine(hash_value, SO::ToUpper(module_file_path));
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
            std::string wrapped_function_name = FormatText("cspro_f%d", static_cast<int>(m_wrappedModuleFunctions.size()));
            std::string function_parameters;

            for( size_t i = 0; i < number_function_arguments; ++i )
                SO::AppendWithSeparator(function_parameters, FormatText("a%d", static_cast<int>(i)), ',');

            const std::string script = FormatText("import { %s } from \"%s\";\n"
                                                  "globalThis.%s = function(%s) { return %s(%s); };",
                                                  function_name.c_str(),
                                                  Encoders::ToEscapedString(module_file_path).c_str(),
                                                  wrapped_function_name.c_str(),
                                                  function_parameters.c_str(),
                                                  function_name.c_str(),
                                                  function_parameters.c_str());

            EvaluateScript<Value>(script, SO::Empty_string, 1, JS_EVAL_TYPE_MODULE);

            function_name_to_execute = &m_wrappedModuleFunctions.try_emplace(hash_value, std::move(wrapped_function_name)).first->second;
        }
    }

    const Value js_result = InvokeFunction(*function_name_to_execute, number_function_arguments, function_arguments);

    return js_result.ToString();
}
