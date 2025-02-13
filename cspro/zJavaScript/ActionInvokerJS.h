#pragma once

#include <zJavaScript/Executor.h>
#include <zAction/ActionInvoker.h>


struct JavaScript::Executor::ActionInvokerJS
{
    // variables
    std::shared_ptr<ActionInvoker::Runtime> action_invoker_runtime;
    std::vector<std::unique_ptr<std::string>> cached_text_used_for_QuickJS_pointers;
    std::vector<ActionInvoker::Action> actions;
    JSClassID CS_class_id;

    struct ClassDetails
    {
        const JSClassID class_id;
        const char* name;
        std::vector<JSCFunctionListEntry> functions;
    };

    std::vector<ClassDetails> class_details;

    // adds the text to cached_text_used_for_QuickJS_pointers
    const char* AddToQuickJSTextCache(wstring_view text_sv);

    // adds the CS object to the global object
    void AddToGlobalObject(JSContext* ctx, JSValue js_global_obj);

    // the runtime functions
    static JSValue RunActionInvoker(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
    static JSValue RunActionInvokerAsync(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
};
