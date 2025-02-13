#pragma once

#include <zJavaScript/Executor.h>
#include <mutex>

#pragma warning(push, 0)
#pragma warning(disable:4100)
#include <external/QuickJS/quickjs.h>
#pragma warning(pop)


struct JavaScript::QuickJSAccess
{
    static constexpr std::string_view UnnamedScriptFilename = "<script>";

    // variables
    Executor* executor;
    JSRuntime* rt;
    JSContext* ctx;
    JSValueConst js_console_object;

    // static variables (the context map)
    static std::mutex context_map_mutex;
    static std::vector<std::tuple<JSContext*, Executor*>> context_map;
    static std::set<JSRuntime*> runtime_interrupt_requests;

    // methods
    [[noreturn]] void ThrowException();

    static JSValue NewBoolean(bool value) { return value ? JS_TRUE: JS_FALSE; }

    JSValue NewDouble(double value) { return JS_NewFloat64(ctx, value); }

    JSValue NewString(std::string_view text_sv) { return JS_NewStringLen(ctx, text_sv.data(), text_sv.length()); }
    JSValue NewString(wstring_view text_sv)     { return NewString(UTF8Convert::WideToUTF8(text_sv)); }

    template<typename T = std::string>
    T GetString(JSValue js_value, bool trim_string = false);

    ByteCode ObjectToByteCode(JSValue js_object);
    JSValue ByteCodeToObject(const ByteCode& byte_code);

    // static methods
    static Executor& GetExecutorFromContext(JSContext* ctx);

    static int InterruptHandler(JSRuntime* rt, void* opaque);

    static char* ModuleLoaderNameNormalizer(JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque);
    static JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name, void* opaque);

    static JSValue PrintEvaluator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
};
