#include "stdafx.h"
#include "QuickJSAccess.h"
#include "ValueInternal.h"
#include <zUtilO/UWM.h>
#include <regex>


std::mutex JavaScript::QuickJSAccess::context_map_mutex;
std::vector<std::tuple<JSContext*, JavaScript::Executor*>> JavaScript::QuickJSAccess::context_map;
std::set<JSRuntime*> JavaScript::QuickJSAccess::runtime_interrupt_requests;


JavaScript::Exception JavaScript::QuickJSAccess::CreateException()
{
    const Value js_exception(this, JS_GetException(ctx));

    std::string exception_message = GetString(*js_exception, true);

    if( exception_message.empty() )
        exception_message = "[Unknown JavaScript Error]";

    const size_t original_exception_message_length = exception_message.length();
    std::optional<std::tuple<std::string, int>> file_path_and_line_number;

    if( JS_IsError(ctx, *js_exception) )
    {
        const Value js_stack(this, JS_GetPropertyStr(ctx, *js_exception, "stack"));

        if( !JS_IsUndefined(*js_stack) )
        {
            const std::string stack_text = GetString(*js_stack, true);

            if( !stack_text.empty() )
            {
                exception_message.push_back(' ');
                exception_message.append(stack_text);

                // the stack text should look something like: at <eval> (test.js:2)
                //                                            at test.js:2

                for( const char* const regex_text : { R"(^\s*at.*\((.+):(\d+)\)$)",
                                                      R"(^\s*at\s+(.+):(\d+)$)" } )
                {
                    const std::regex regex(regex_text);
                    std::cmatch matches;

                    if( std::regex_match(stack_text.c_str(), matches, regex) )
                    {
                        std::string file_path = matches.str(1);

                        if( file_path == UnnamedScriptFilename_sv )
                            file_path.clear();

                        file_path_and_line_number.emplace(MakeFullPath(executor->m_rootDirectory, std::move(file_path)),
                                                          std::stoi(matches.str(2)));

                        break;
                    }
                }
            }
        }
    }

    if( file_path_and_line_number.has_value() )
    {
        return Exception(exception_message.c_str(),
                         exception_message.substr(0, original_exception_message_length),
                         std::move(std::get<0>(*file_path_and_line_number)),
                         std::get<1>(*file_path_and_line_number));
    }

    else
    {
        return Exception(exception_message);
    }
}


JSValue JavaScript::QuickJSAccess::NewError(const std::exception& exception, const cs::cref_optional<std::string> name, const cs::cref_optional<std::string> cause)
{
    JSValue js_error = JS_NewError(ctx);

    auto set_property = [&](const char* const property, const std::string_view value_sv)
    {
        JS_DefinePropertyValueStr(ctx, js_error, property, NewString(value_sv), JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    };

    if( name.has_value() && !name->empty() )
    {
        ASSERT(!name->empty());
        set_property("name", *name);
    }

    set_property("message", exception.what());

    if( cause.has_value() && !cause->empty() )
        set_property("cause", *cause);

    return js_error;
}


std::string JavaScript::QuickJSAccess::GetString(const JSValue js_value, const bool trim_string/* = false*/)
{
    const char* const text = JS_ToCString(ctx, js_value);

    if( text == nullptr )
        return std::string();

    std::string str = text;

    JS_FreeCString(ctx, text);

    if( trim_string )
        SO::MakeTrim(str);

    return str;
}


std::string JavaScript::QuickJSAccess::GetString(const JSAtom js_atom)
{
    const char* const text = JS_AtomToCString(ctx, js_atom);

    if( text == nullptr )
        return ReturnProgrammingError(std::string());

    std::string str = text;

    JS_FreeCString(ctx, text);

    return str;
}


JavaScript::Bytecode JavaScript::QuickJSAccess::ObjectToBytecode(const JSValue js_object)
{
    size_t js_bytecode_size;
    uint8_t* js_bytecode = JS_WriteObject(ctx, &js_bytecode_size, js_object, JS_WRITE_OBJ_BYTECODE);

    Bytecode bytecode;
    bytecode.insert(bytecode.end(), js_bytecode, js_bytecode + js_bytecode_size);

    js_free(ctx, js_bytecode);

    return bytecode;
}


JSValue JavaScript::QuickJSAccess::BytecodeToObject(const Bytecode& bytecode)
{
    // js_object only has to be freed if the module cannot be resolved
    JSValue js_object = JS_ReadObject(ctx, bytecode.data(), bytecode.size(), JS_READ_OBJ_BYTECODE);

    if( JS_IsException(js_object) )
        ThrowException();

    if( JS_VALUE_GET_TAG(js_object) == JS_TAG_MODULE )
    {
        if( JS_ResolveModule(ctx, js_object) < 0 )
        {
            JS_FreeValue(ctx, js_object);
            ThrowException();
        }
    }

    return js_object;
}


JavaScript::Executor& JavaScript::QuickJSAccess::GetExecutorFromContext(JSContext* const ctx)
{
    const std::lock_guard<std::mutex> lock(context_map_mutex);

    if( context_map.size() == 1 )
    {
        ASSERT(std::get<0>(context_map.front()) == ctx);
        return *std::get<1>(context_map.front());
    }

    const auto& context_search = std::find_if(context_map.cbegin(), context_map.cend(),
                                              [&](const auto& cm) { return ( std::get<0>(cm) == ctx); });
    ASSERT(context_search != context_map.cend());

    return *std::get<1>(*context_search);
}


int JavaScript::QuickJSAccess::InterruptHandler(JSRuntime* const rt, void* /*opaque*/)
{
    if( !runtime_interrupt_requests.empty() )
    {
        const std::lock_guard<std::mutex> lock(context_map_mutex);

        // 0 means to continue, so 1 will only be returned when the runtime was in the set
        return runtime_interrupt_requests.erase(rt);
    }

    return 0;
}


char* JavaScript::QuickJSAccess::ModuleLoaderNameNormalizer(JSContext* const ctx, const char* const module_base_name, const char* const module_name, void* /*opaque*/)
{
    Executor& executor = GetExecutorFromContext(ctx);

    std::string file_path = PortableFunctions::PathToNativeSlash(module_name);

    // case 1: the file path exists
    if( !PortableFunctions::FileIsRegular(file_path) )
    {
        // case 2: evaluate the file path based on the module's base name
        std::string wide_module_base_name = PortableFunctions::PathToNativeSlash(module_base_name);

        if( !executor.m_rootDirectory.empty() )
            wide_module_base_name = MakeFullPath(executor.m_rootDirectory, wide_module_base_name);

        std::string new_file_path_to_use = MakeFullPath(PortableFunctions::PathGetDirectory(wide_module_base_name), file_path);

        if( !PortableFunctions::FileIsRegular(new_file_path_to_use) )
        {
            // case 3: evaluate the file path based on the root directory (when set)
            if( !executor.m_rootDirectory.empty() )
            {
                std::string test_file_path = MakeFullPath(executor.m_rootDirectory, file_path);

                if( PortableFunctions::FileIsRegular(test_file_path) )
                    new_file_path_to_use = std::move(test_file_path);
            }

            // (else) case 4: return the case 2 option
        }

        file_path = std::move(new_file_path_to_use);
    }

    const std::string relative_file_path = executor.GetRelativeFilePath(std::move(file_path));
    const size_t chars_with_null_terminator = relative_file_path.length() + 1;

    char* const file_path_to_return = static_cast<char*>(js_malloc(executor.m_qjs->ctx, chars_with_null_terminator));

    memcpy(file_path_to_return, relative_file_path.c_str(), chars_with_null_terminator);

    return file_path_to_return;
}


JSModuleDef* JavaScript::QuickJSAccess::ModuleLoader(JSContext* const ctx, const char* const module_name, void* /*opaque*/)
{
    TRACE("QuickJSAccess: loading module: %s\n", module_name);

    Executor& executor = GetExecutorFromContext(ctx);

    try
    {
        // the module name will be based off the root directory, so adjust it here
        const std::string absolute_file_path = !executor.m_rootDirectory.empty() ? MakeFullPath(executor.m_rootDirectory, module_name) :
                                                                                   module_name;

        std::optional<Value> js_result;

        // load from bytecode...
        if( executor.m_moduleLoaderHelper != nullptr )
        {
            const Bytecode* const bytecode = executor.m_moduleLoaderHelper->GetBytecode(absolute_file_path);

            if( bytecode != nullptr )
                js_result.emplace(executor.m_qjs, executor.m_qjs->BytecodeToObject(*bytecode));
        }

        // ...or from the disk
        if( !js_result.has_value() )
        {
            std::string script;

            // in case the file is open in an editor, try to get the potentially modified-but-unsaved text;
            // otherwise load it from the disk
            if( WindowsDesktopMessage::Send(UWM::UtilO::GetCodeText, &absolute_file_path, &script) != 1 )
                script = FileIO::ReadText(absolute_file_path);

            js_result = executor.EvaluateScript<Value>(script, module_name, 1, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        }

        JSModuleDef* const js_module_def = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(js_result->GetValue()));

        // set the import metadata
        const Value js_meta_object(executor.m_qjs, JS_GetImportMeta(ctx, js_module_def));

        if( !JS_IsException(*js_meta_object) )
        {
            const std::string file_url = Encoders::ToFileUrl(module_name);
            JS_DefinePropertyValueStr(ctx, *js_meta_object, "url", NewString(ctx, file_url), JS_PROP_C_W_E);
        }

        return js_module_def;
    }

    catch( const CSProException& exception )
    {
        if( executor.m_moduleLoadingErrors != nullptr )
            executor.m_moduleLoadingErrors->emplace_back(module_name, exception.what());

        JS_ThrowReferenceError(ctx, "Could not load module: %s", exception.what());

        return nullptr;
    }
}


JSValue JavaScript::QuickJSAccess::PrintEvaluator(JSContext* const ctx, const JSValueConst this_val, const int argc, JSValueConst* const argv)
{
    std::string full_text;

    for( int i = 0; i < argc; ++i )
    {
        const char* const text = JS_ToCString(ctx, argv[i]);

        if( text == nullptr )
            return JS_EXCEPTION;

        SO::AppendWithSeparator(full_text, text, " ");

        JS_FreeCString(ctx, text);
    }

    Executor& executor = GetExecutorFromContext(ctx);

    if( JS_VALUE_GET_PTR(this_val) == JS_VALUE_GET_PTR(executor.m_qjs->js_console_object) )
    {
        executor.m_printer->OnConsoleLog(std::move(full_text));
    }

    else
    {
        executor.m_printer->OnPrint(std::move(full_text));
    }

    return JS_UNDEFINED;
}
