#pragma once

#include <zJavaScript/Executor.h>
#include <mutex>

#pragma warning(push, 0)
#pragma warning(disable:4100)
#pragma warning(disable:4244)
#include <external/QuickJS/quickjs.h>
#pragma warning(pop)


struct JavaScript::QuickJSAccess
{
    static constexpr std::string_view UnnamedScriptFilename_sv = "<script>";

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
    Exception CreateException();
    [[noreturn]] void ThrowException() { throw CreateException(); }

    JSValue NewError(const std::exception& exception, cs::cref_optional<std::string> name, cs::cref_optional<std::string> cause);

    static JSValue NewBoolean(bool value) { return value ? JS_TRUE: JS_FALSE; }

    JSValue NewDouble(double value) { return JS_NewFloat64(ctx, value); }

    JSValue NewString(std::string_view text_sv)                         { return JS_NewStringLen(ctx, text_sv.data(), text_sv.length()); }
    static JSValue NewString(JSContext* ctx_, std::string_view text_sv) { return JS_NewStringLen(ctx_, text_sv.data(), text_sv.length()); }

    JSAtom NewAtom(std::string_view text_sv) { return JS_NewAtomLen(ctx, text_sv.data(), text_sv.size()); }

    std::string GetString(JSValue js_value, bool trim_string = false);
    std::string GetString(JSAtom js_atom);

    Bytecode ObjectToBytecode(JSValue js_object);
    JSValue BytecodeToObject(const Bytecode& bytecode);

    // static methods
    static Executor& GetExecutorFromContext(JSContext* ctx);

    static int InterruptHandler(JSRuntime* rt, void* opaque);

    static char* ModuleLoaderNameNormalizer(JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque);
    static JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name, void* opaque);

    static JSValue PrintEvaluator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
};
