#include "stdafx.h"
#include "ActionInvokerJS.h"
#include <zToolsO/ObjectTransporter.h>
#include <zToolsO/RaiiHelpers.h>
#include <zLogicO/FunctionTable.h>


// --------------------------------------------------------------------------
// JavaScript::Executor
// --------------------------------------------------------------------------

void JavaScript::Executor::UseActionInvoker(const std::vector<const Logic::FunctionDetails*>& functions, const std::map<Logic::FunctionNamespace, const TCHAR*>& namespace_names)
{
    // return if it has already been set up
    if( m_actionInvokerJS != nullptr )
        return;

    m_actionInvokerJS = std::make_unique<ActionInvokerJS>();

    struct ActionInvokerSetUpError { };

    try
    {
        m_actionInvokerJS->action_invoker_runtime = ObjectTransporter::GetActionInvokerRuntime();

        if( m_actionInvokerJS->action_invoker_runtime == nullptr )
            throw ActionInvokerSetUpError();

        // a routine to create a class
        std::map<Logic::FunctionNamespace, JSClassID> function_namespace_to_class_id_map;

        auto create_class = [&](const Logic::FunctionNamespace function_namespace)
        {
            ASSERT(function_namespace_to_class_id_map.find(function_namespace) == function_namespace_to_class_id_map.cend());

            JSClassID class_id = 0;
            JS_NewClassID(&class_id);

            const auto& name_lookup = namespace_names.find(function_namespace);

            if( name_lookup == namespace_names.cend() )
                throw ActionInvokerSetUpError();

            const char* const class_name = m_actionInvokerJS->AddToQuickJSTextCache(name_lookup->second);

            const JSClassDef class_definition = { class_name };

            if( JS_NewClass(m_qjs->rt, class_id, &class_definition) == -1 )
                throw ActionInvokerSetUpError();

            function_namespace_to_class_id_map.try_emplace(function_namespace, class_id);

            m_actionInvokerJS->class_details.emplace_back(ActionInvokerJS::ClassDetails { class_id, class_name, { } });

            return class_id;
        };

        // set up the CS class
        m_actionInvokerJS->CS_class_id = create_class(Logic::FunctionNamespace::CS);

        // set up classes for the namespaces and associate functions with their classes
        for( const Logic::FunctionDetails& function_details : VI_V(functions) )
        {
            ASSERT(std::holds_alternative<Logic::FunctionNamespace>(function_details.function_domain));
            const Logic::FunctionNamespace function_namespace = std::get<Logic::FunctionNamespace>(function_details.function_domain);

            // get or create a JavaScript class corresponding to this namespace
            const auto& function_namespace_lookup = function_namespace_to_class_id_map.find(function_namespace);

            const JSClassID class_id = ( function_namespace_lookup != function_namespace_to_class_id_map.cend() ) ? function_namespace_lookup->second :
                                                                                                                    create_class(function_namespace);

            // the function's magic code will correspond to the action's index in the actions vector
            const size_t action_index = m_actionInvokerJS->actions.size();
            m_actionInvokerJS->actions.emplace_back(static_cast<ActionInvoker::Action>(function_details.number_arguments));

            auto class_lookup = std::find_if(m_actionInvokerJS->class_details.begin(), m_actionInvokerJS->class_details.end(),
                                             [&](const ActionInvokerJS::ClassDetails& class_details) { return ( class_details.class_id == class_id ); });
            ASSERT(class_lookup != m_actionInvokerJS->class_details.end());

            // create synchronous and asynchronous versions of the function
            auto create_function = [&](const wstring_view action_name_sv, const auto& execution_function)
            {
                JSCFunctionListEntry& function = class_lookup->functions.emplace_back();

                function.name = m_actionInvokerJS->AddToQuickJSTextCache(action_name_sv);

                function.prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
                function.def_type = JS_DEF_CFUNC;

                function.magic = static_cast<int16_t>(action_index);

                function.u.func.length = 1;
                function.u.func.cproto = JS_CFUNC_generic_magic;
                function.u.func.cfunc.generic_magic = execution_function;
            };

            create_function(function_details.name, ActionInvokerJS::RunActionInvoker);
            create_function(std::wstring(function_details.name).append(ActionInvoker::AsyncActionSuffix_sv), ActionInvokerJS::RunActionInvokerAsync);
        }

        // now instantiate each class in the global object
        const JSValue js_global_obj = JS_GetGlobalObject(m_qjs->ctx);
        m_actionInvokerJS->AddToGlobalObject(m_qjs->ctx, js_global_obj);
        JS_FreeValue(m_qjs->ctx, js_global_obj);
    }

    catch( const ActionInvokerSetUpError& )
    {
        ASSERT(false);
        m_actionInvokerJS.reset();
    }
}



// --------------------------------------------------------------------------
// JavaScript::Executor::ActionInvokerJSCaller
// --------------------------------------------------------------------------

class JavaScript::Executor::ActionInvokerJSCaller : public ActionInvoker::Caller
{
public:
    ActionInvokerJSCaller(Executor& executor)
        :   m_executor(executor)
    {
    }

    bool& GetCancelFlag() override;

    std::wstring GetRootDirectory() override { return m_executor.m_rootDirectory; }

private:
    struct CancelFlagData
    {
        std::shared_ptr<bool> cancel_flag;
        RAII::SetValueAndRestoreOnDestruction<std::shared_ptr<bool>> cancel_flag_holder;
    };

    Executor& m_executor;
    std::unique_ptr<CancelFlagData> m_cancelFlagData;
};


bool& JavaScript::Executor::ActionInvokerJSCaller::GetCancelFlag()
{
    // only create a cancelation flag on demand
    if( m_cancelFlagData == nullptr )
    {
        auto cancel_flag = std::make_shared<bool>(false);
        m_cancelFlagData = std::make_unique<CancelFlagData>(CancelFlagData
            {
                cancel_flag,
                RAII::SetValueAndRestoreOnDestruction<std::shared_ptr<bool>>(m_executor.m_currentCancelationFlag, cancel_flag)
            });
    }

    return *m_cancelFlagData->cancel_flag;
}



// --------------------------------------------------------------------------
// JavaScript::Executor::ActionInvokerJS
// --------------------------------------------------------------------------

const char* JavaScript::Executor::ActionInvokerJS::AddToQuickJSTextCache(const wstring_view text_sv)
{
    return cached_text_used_for_QuickJS_pointers.emplace_back(std::make_unique<std::string>(UTF8Convert::WideToUTF8(text_sv)))->c_str();
}


void JavaScript::Executor::ActionInvokerJS::AddToGlobalObject(JSContext* ctx, const JSValue js_global_obj)
{
    auto create_object = [&](const ClassDetails& class_details, const JSValue js_parent_object)
    {
        const JSValue js_prototype = JS_NewObject(ctx);
        JS_SetClassProto(ctx, class_details.class_id, js_prototype);

        // create the object
        const JSValue js_object = JS_NewObjectClass(ctx, class_details.class_id);

        // apply the functions to the object
        JS_SetPropertyFunctionList(ctx, js_object, class_details.functions.data(), class_details.functions.size());

        // add it the global object or the previously-created CS object
        JS_DefinePropertyValueStr(ctx, js_parent_object, class_details.name, js_object, 0);

        return js_object;
    };

    // add the CS object to the global object
    ASSERT(!class_details.empty() && class_details.front().class_id == CS_class_id);
    const JSValue js_CS_object = create_object(class_details.front(), js_global_obj);

    // add the namespace objects to the CS object
    for( auto class_details_itr = class_details.cbegin() + 1; class_details_itr != class_details.cend(); ++class_details_itr )
        create_object(*class_details_itr, js_CS_object);
}


JSValue JavaScript::Executor::ActionInvokerJS::RunActionInvoker(JSContext* ctx, const JSValueConst /*this_val*/, const int argc, JSValueConst* argv, const int magic)
{
    Executor& executor = QuickJSAccess::GetExecutorFromContext(ctx);
    ASSERT(executor.m_actionInvokerJS != nullptr && static_cast<size_t>(magic) < executor.m_actionInvokerJS->actions.size());

    ActionInvoker::Runtime& action_invoker_runtime = *executor.m_actionInvokerJS->action_invoker_runtime;
    const ActionInvoker::Action action = executor.m_actionInvokerJS->actions[magic];

    try
    {
        std::optional<std::wstring> json_arguments;

        // CS[.namespace].actionName({ arguments_object })
        if( argc > 0 )
        {
            // allow the arguments to be specified as an object that will be stringified...
            const JSValue js_stringified_argument = JS_JSONStringify(executor.m_qjs->ctx, argv[0], JS_NULL, JS_NULL);

            if( JS_IsException(js_stringified_argument) )
                throw js_stringified_argument;

            json_arguments = executor.m_qjs->GetString<std::wstring>(js_stringified_argument);

            JS_FreeValue(executor.m_qjs->ctx, js_stringified_argument);
        }

        // run the action and return the result as an object (if defined)
        ActionInvokerJSCaller caller(executor);
        const ActionInvoker::Result result = action_invoker_runtime.ProcessAction(action, json_arguments, caller);

        switch( result.GetType() )
        {
            case ActionInvoker::Result::Type::Undefined:
            {
                return JS_UNDEFINED;
            }

            case ActionInvoker::Result::Type::Bool:
            {
                return QuickJSAccess::NewBoolean(result.GetNumericResult() != 0);
            }

            case ActionInvoker::Result::Type::Number:
            {
                return executor.m_qjs->NewDouble(result.GetNumericResult());
            }

            case ActionInvoker::Result::Type::String:
            {
                return executor.m_qjs->NewString(result.GetStringResult());
            }

            default:
            {
                ASSERT(result.GetType() == ActionInvoker::Result::Type::JsonText);
                const std::string result_utf8 = UTF8Convert::WideToUTF8(result.GetStringResult());
            
                return JS_ParseJSON(executor.m_qjs->ctx, result_utf8.c_str(), result_utf8.length(), nullptr);
            }
        }
    }

    catch( const JSValue& js_value_exception )
    {
        ASSERT(JS_IsException(js_value_exception));
        return js_value_exception;
    }

    catch( const ActionInvoker::ExceptionWithActionName& exception )
    {
        return JS_Throw(ctx, executor.m_qjs->NewString(exception.GetErrorMessageWithActionName()));
    }

    catch( const CSProException& exception )
    {
        return JS_Throw(ctx, executor.m_qjs->NewString(ActionInvoker::ExceptionWithActionName::GetErrorMessageFromCSProException(exception)));
    }
}


JSValue JavaScript::Executor::ActionInvokerJS::RunActionInvokerAsync(JSContext* ctx, const JSValueConst this_val, const int argc, JSValueConst* argv, const int magic)
{
    JSValue js_resolve_reject_functions[2];
    JSValue js_promise = JS_NewPromiseCapability(ctx, js_resolve_reject_functions);

    if( JS_IsException(js_promise) )
        return js_promise;

    // for now, run asynchronous calls in the same thread
    JSValue js_result = RunActionInvoker(ctx, this_val, argc, argv, magic);

    // reject
    if( JS_IsException(js_result) )
    {
        JSValue js_exception = JS_GetException(ctx);
        JS_Call(ctx, js_resolve_reject_functions[1], JS_UNDEFINED, 1, &js_exception);
        JS_FreeValue(ctx, js_exception);
    }

    // resolve
    else
    {
        JS_Call(ctx, js_resolve_reject_functions[0], JS_UNDEFINED, 1, &js_result);
    }

    JS_FreeValue(ctx, js_result);
    JS_FreeValue(ctx, js_resolve_reject_functions[0]);
    JS_FreeValue(ctx, js_resolve_reject_functions[1]);

    return js_promise;
}
