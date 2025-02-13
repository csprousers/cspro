#include "stdafx.h"
#include "QuickJSAccess.h"
#include <zUtilO/UWM.h>
#include <regex>


std::mutex JavaScript::QuickJSAccess::context_map_mutex;
std::vector<std::tuple<JSContext*, JavaScript::Executor*>> JavaScript::QuickJSAccess::context_map;
std::set<JSRuntime*> JavaScript::QuickJSAccess::runtime_interrupt_requests;


void JavaScript::QuickJSAccess::ThrowException()
{
    JSValue js_exception = JS_GetException(ctx);

    std::string exception_message = GetString(js_exception, true);

    if( exception_message.empty() )
        exception_message = "[Unknown JavaScript Error]";

    size_t original_exception_message_length = exception_message.length();
    std::optional<std::tuple<std::wstring, int>> filename_and_line_number;

    if( JS_IsError(ctx, js_exception) )
    {
        JSValue js_stack = JS_GetPropertyStr(ctx, js_exception, "stack");

        if( !JS_IsUndefined(js_stack) )
        {
            std::string stack_text = GetString(js_stack, true);

            if( !stack_text.empty() )
            {
                exception_message.push_back(' ');
                exception_message.append(stack_text);

                // the stack text should look something like: at <eval> (test.js:2)
                //                                            at test.js:2

                for( const char* regex_text : { R"(^\s*at.*\((.+):(\d+)\)$)",
                                                R"(^\s*at\s+(.+):(\d+)$)" } )
                {
                    std::regex regex(regex_text);
                    std::cmatch matches;

                    if( std::regex_match(stack_text.c_str(), matches, regex) )
                    {
                        std::string filename = matches.str(1);

                        if( filename == UnnamedScriptFilename )
                            filename.clear();

                        filename_and_line_number.emplace(MakeFullPath(executor->m_rootDirectory, UTF8Convert::UTF8ToWide(filename)),
                                                         std::stoi(matches.str(2)));

                        break;
                    }
                }
            }

            JS_FreeValue(ctx, js_stack);
        }
    }

    JS_FreeValue(ctx, js_exception);

    if( filename_and_line_number.has_value() )
    {
        throw Exception(exception_message.c_str(),
                        exception_message.substr(0, original_exception_message_length),
                        std::move(std::get<0>(*filename_and_line_number)),
                        std::get<1>(*filename_and_line_number));
    }

    else
    {
        throw Exception(exception_message.c_str());
    }    
}


template<typename T/* = std::string*/>
T JavaScript::QuickJSAccess::GetString(JSValue js_value, bool trim_string/* = false*/)
{
    const char* text = JS_ToCString(ctx, js_value);

    if( text == nullptr )
        return T();

    std::string str = text;

    JS_FreeCString(ctx, text);

    if( trim_string && !str.empty() && ( std::isspace(str.front()) || std::isspace(str.back()) ) )
    {
        auto first_non_whitespace = std::find_if(str.cbegin(), str.cend(),
                                                 [](char ch) { return !std::isspace(ch); });

        if( first_non_whitespace == str.cend() )
        {
            str.clear();
        }

        else
        {
            auto last_non_whitespace = std::find_if(str.crbegin(), str.crend(),
                                                    [](char ch) { return !std::isspace(ch); });
            str = std::string(&*first_non_whitespace, &*last_non_whitespace - &*first_non_whitespace + 1);
        }
    }

    if constexpr(std::is_same_v<T, std::string>)
    {
        return str;
    }

    else
    {
        return UTF8Convert::UTF8ToWide(str);
    }
}

template std::string JavaScript::QuickJSAccess::GetString(JSValue js_value, bool trim_string/* = false*/);
template std::wstring JavaScript::QuickJSAccess::GetString(JSValue js_value, bool trim_string/* = false*/);


JavaScript::ByteCode JavaScript::QuickJSAccess::ObjectToByteCode(JSValue js_object)
{
    size_t js_byte_code_size;
    uint8_t* js_byte_code = JS_WriteObject(ctx, &js_byte_code_size, js_object, JS_WRITE_OBJ_BYTECODE);

    ByteCode byte_code;
    byte_code.insert(byte_code.end(), js_byte_code, js_byte_code + js_byte_code_size);

    js_free(ctx, js_byte_code);

    return byte_code;
}


JSValue JavaScript::QuickJSAccess::ByteCodeToObject(const ByteCode& byte_code)
{
    // js_object only has to be freed if the module cannot be resolved
    JSValue js_object = JS_ReadObject(ctx, byte_code.data(), byte_code.size(), JS_READ_OBJ_BYTECODE);

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


JavaScript::Executor& JavaScript::QuickJSAccess::GetExecutorFromContext(JSContext* ctx)
{
    std::lock_guard<std::mutex> lock(context_map_mutex);

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


int JavaScript::QuickJSAccess::InterruptHandler(JSRuntime* rt, void* /*opaque*/)
{    
    if( !runtime_interrupt_requests.empty() )
    {
        std::lock_guard<std::mutex> lock(context_map_mutex);

        // 0 means to continue, so 1 will only be returned when the runtime was in the set
        return runtime_interrupt_requests.erase(rt);
    }

    return 0;
}


char* JavaScript::QuickJSAccess::ModuleLoaderNameNormalizer(JSContext* ctx, const char* module_base_name, const char* module_name, void* /*opaque*/)
{
    Executor& executor = GetExecutorFromContext(ctx);

    std::wstring filename = PortableFunctions::PathToNativeSlash(UTF8Convert::UTF8ToWide(module_name));

    // case 1: the filename exists
    if( !PortableFunctions::FileIsRegular(filename) )
    {
        // case 2: evaluate the filename based on the module's base name
        std::wstring wide_module_base_name = PortableFunctions::PathToNativeSlash(UTF8Convert::UTF8ToWide(module_base_name));

        if( !executor.m_rootDirectory.empty() )
            wide_module_base_name = MakeFullPath(executor.m_rootDirectory, wide_module_base_name);

        std::wstring new_filename_to_use = MakeFullPath(PortableFunctions::PathGetDirectory(wide_module_base_name), filename);

        if( !PortableFunctions::FileIsRegular(new_filename_to_use) )
        {
            // case 3: evaluate the filename based on the root directory (when set)
            if( !executor.m_rootDirectory.empty() )
            {
                std::wstring test_filename = MakeFullPath(executor.m_rootDirectory, filename);

                if( PortableFunctions::FileIsRegular(test_filename) )
                    new_filename_to_use = test_filename;
            }

            // (else) case 4: return the case 2 option
        }

        filename = new_filename_to_use;
    }

    const std::string utf_filename = UTF8Convert::WideToUTF8(executor.GetRelativeFilename(filename));
    const size_t chars_with_null_terminator = utf_filename.length() + 1;

    char* filename_to_return = static_cast<char*>(js_malloc(executor.m_qjs->ctx, chars_with_null_terminator));

    memcpy(filename_to_return, utf_filename.c_str(), chars_with_null_terminator);

    return filename_to_return;
}


JSModuleDef* JavaScript::QuickJSAccess::ModuleLoader(JSContext* ctx, const char* module_name, void* /*opaque*/)
{
    try
    {
        Executor& executor = GetExecutorFromContext(ctx);
        std::wstring relative_filename = UTF8Convert::UTF8ToWide(module_name);
        std::optional<JSValue> js_result;
        bool save_byte_code = false;

        // load from byte code...
        if( executor.m_moduleLoaderHelper != nullptr )
        {
            const ByteCode* byte_code = executor.m_moduleLoaderHelper->GetByteCode(relative_filename);

            if( byte_code != nullptr )
            {
                js_result = executor.m_qjs->ByteCodeToObject(*byte_code);
            }

            else
            {
                save_byte_code = executor.m_moduleLoaderHelper->NeedByteCode();
            }
        }

        // ...or from the disk
        if( !js_result.has_value() )
        {
            // the module name will be based off the root directory, so adjust it here
            std::wstring absolute_filename = relative_filename;

            if( !executor.m_rootDirectory.empty() )
                absolute_filename = MakeFullPath(executor.m_rootDirectory, relative_filename);

            std::string script;
            std::wstring wide_script;

            // in case the file is open in an editor, try to get the potentially modified-but-unsaved text
            if( WindowsDesktopMessage::Send(UWM::UtilO::GetCodeText, absolute_filename.c_str(), &wide_script) == 1 )
            {
                script = UTF8Convert::WideToUTF8(wide_script);
            }

            // otherwise load it from the disk
            else
            {
                script = FileIO::ReadText<std::string>(absolute_filename);
            }

            js_result = executor.EvaluateScript<JSValue>(script, relative_filename.c_str(), 1, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        }

        JSModuleDef* js_module_def = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(*js_result));

        // set the import metadata
        JSValue js_meta_object = JS_GetImportMeta(ctx, js_module_def);

        if( !JS_IsException(js_meta_object) )
        {
            std::string file_url = UTF8Convert::WideToUTF8(Encoders::ToFileUrl(relative_filename));
            JS_DefinePropertyValueStr(ctx, js_meta_object, "url", JS_NewStringLen(ctx, file_url.data(), file_url.length()), JS_PROP_C_W_E);

            JS_FreeValue(ctx, js_meta_object);
        }

        // save the byte code (as needed)
        if( save_byte_code )
            executor.m_moduleLoaderHelper->SetByteCode(relative_filename, executor.m_qjs->ObjectToByteCode(*js_result));

        JS_FreeValue(ctx, *js_result);

        return js_module_def;
    }

    catch( const CSProException& exception )
    {
        JS_ThrowReferenceError(ctx, "Could not load module: %s", exception.what());
        return nullptr;
    }
}


JSValue JavaScript::QuickJSAccess::PrintEvaluator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    std::string full_text;

    for( int i = 0; i < argc; ++i )
    {
        if( i != 0 )
            full_text.append(" ");

        const char* text = JS_ToCString(ctx, argv[i]);

        if( text == nullptr )
            return JS_EXCEPTION;

        full_text.append(text);

        JS_FreeCString(ctx, text);
    }

    Executor& executor = GetExecutorFromContext(ctx);

    if( JS_VALUE_GET_PTR(this_val) == JS_VALUE_GET_PTR(executor.m_qjs->js_console_object) )
    {
        executor.m_printer->OnConsoleLog(full_text);
    }

    else
    {
        executor.m_printer->OnPrint(full_text);
    }

    return JS_UNDEFINED;
}
