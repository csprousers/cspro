#include "stdafx.h"
#include "ActionInvoker.h"
#include "NameProcessors.h"
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/ExecutionStack.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/Messages.h>
#include <zHtml/VirtualFileMapping.h>


ActionInvoker::Runtime::Runtime()
    :   m_registeredAccessTokensForExternalCallers(std::make_unique<std::set<std::wstring>>()),
        m_listeners(std::make_shared<std::vector<Listener*>>())
{
}


ActionInvoker::Runtime::~Runtime()
{
}


void ActionInvoker::Runtime::DisableAccessTokenCheckForExternalCallers()
{
    m_registeredAccessTokensForExternalCallers.reset();
}


void ActionInvoker::Runtime::RegisterAccessToken(std::wstring access_token)
{
    if( m_registeredAccessTokensForExternalCallers != nullptr )
        m_registeredAccessTokensForExternalCallers->insert(std::move(access_token));
}


void ActionInvoker::Runtime::CheckAccessToken(const std::wstring* access_token, Caller& caller)
{
    constexpr const char* NoValidAccessTokenMessage = "The application settings require that a valid access token is provided before using the Action Invoker.";
    constexpr const char* UserDidNotAllowMessage    = "The user denied access to the Action Invoker without a valid access token.";

    ASSERT(caller.IsExternalCaller());

    // if the need for access tokens has been disabled, there is nothing to check
    if( m_registeredAccessTokensForExternalCallers == nullptr )
        return;

    // if the user has indicated that access tokens are or are not required, use that setting
    const std::optional<bool> user_override_access_token_requirement = caller.GetUserOverrodeAccessTokenRequirement();

    if( user_override_access_token_requirement.has_value() )
    {
        if( *user_override_access_token_requirement )
            return;

        throw CSProException(UserDidNotAllowMessage);
    }

    // check if the access token has been registered, or if the caller allows this specific access token
    if( access_token != nullptr && ( m_registeredAccessTokensForExternalCallers->count(*access_token) == 1 ||
                                     caller.IsAccessTokenValid(*access_token) ) )
    {
        return;
    }

    // get the application's logic settings for additional checks;
    // if no application exists, we will prompt to allow access
    const Application* application = GetApplication(false);
    bool prompt_to_allow_access;

    if( application == nullptr )
    {
        prompt_to_allow_access = true;
    }

    else
    {
        const LogicSettings& logic_settings = application->GetLogicSettings();

        // see if there are any additional access tokens defined in the logic settings
        if( access_token != nullptr  )
        {
            bool access_token_found = false;

            for( const std::wstring& logic_settings_access_token : logic_settings.GetActionInvokerAccessTokens() )
            {
                if( m_registeredAccessTokensForExternalCallers->insert(logic_settings_access_token).second &&
                    *access_token == logic_settings_access_token )
                {
                    access_token_found = true;
                }
            }

            if( access_token_found )
                return;
        }

        // throw an exception if the logic settings require a valid access token
        if( logic_settings.GetActionInvokerAccessFromExternalCaller() == LogicSettings::ActionInvokerAccessFromExternalCaller::RequireAccessToken )
            throw CSProException(NoValidAccessTokenMessage);

        // if the settings allow for it, prompt the user to allow this action
        prompt_to_allow_access = ( logic_settings.GetActionInvokerAccessFromExternalCaller() == LogicSettings::ActionInvokerAccessFromExternalCaller::PromptIfNoValidAccessToken );
    }

    if( prompt_to_allow_access )
    {
        const std::wstring user_prompt_message = MGF::GetMessageText(MGF::CS_access_without_token_prompt_9208,
                                                                     _T("A web page or program is attempting to access CSPro functionality, which can include access to data or to files on your device. Do you want to allow this? You should only allow this if you trust this source."));
        int result;

#ifdef WIN_DESKTOP
        result = AfxMessageBox(user_prompt_message.c_str(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
#else
        result = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T("Allow Access?"), user_prompt_message, MB_YESNO);
#endif
        if( result != IDYES )
        {
            caller.SetUserOverrodeAccessTokenRequirement(false);
            throw CSProException(UserDidNotAllowMessage);
        }
    }

    // at this point, the user allowed for this action, or the settings don't require access tokens, so indicate
    // to the caller that actions are allowed for this external caller
    ASSERT(prompt_to_allow_access || application->GetLogicSettings().GetActionInvokerAccessFromExternalCaller() == LogicSettings::ActionInvokerAccessFromExternalCaller::AlwaysAllow);

    caller.SetUserOverrodeAccessTokenRequirement(true);
}


ActionInvoker::ListenerHolder ActionInvoker::Runtime::RegisterListener(std::shared_ptr<Listener> listener)
{
    return ListenerHolder(m_listeners, std::move(listener));
}


ActionInvoker::Result ActionInvoker::Runtime::ProcessExecute(const std::wstring& json_arguments, Caller& caller)
{
    const JsonNode<wchar_t> json_node = ParseJson(json_arguments, nullptr);
    const Action action = GetActionFromJson(json_node);

    return RunFunction(action, json_node, caller);
}


ActionInvoker::Result ActionInvoker::Runtime::ProcessAction(const Action action, const std::optional<std::wstring>& json_arguments, Caller& caller)
{
    if( json_arguments.has_value() )
    {
        return RunFunction(action, ParseJson(*json_arguments, &action), caller);
    }

    else
    {
        static const JsonNode<wchar_t> no_arguments_json_node = Json::Parse(_T("{ }"));

        return RunFunction(action, no_arguments_json_node, caller);
    }
}


JsonNode<wchar_t> ActionInvoker::Runtime::ParseJson(const std::wstring& json_arguments, const Action* const action)
{
    try
    {
        return Json::Parse(json_arguments);
    }

    catch( const JsonParseException& exception )
    {
        IssueError(MGF::CS_json_argument_error_9205, GetActionName(action).c_str(), exception.GetErrorMessage().c_str());
    }
}


ActionInvoker::Action ActionInvoker::Runtime::GetActionFromJson(const JsonNode<wchar_t>& json_node)
{
    if( !json_node.Contains(JK::action) )
        IssueError(MGF::CS_action_missing_9201);

    return GetActionFromText(json_node.Get<wstring_view>(JK::action), *this);
}


ActionInvoker::Result ActionInvoker::Runtime::RunFunction(const Action action, const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const auto& lookup = m_functions.find(action);

    if( lookup == m_functions.cend() )
        IssueError(MGF::CS_action_invalid_9202, GetActionName(action).c_str());

    try
    {
        ASSERT(!caller.GetCancelFlag());

        return (this->*lookup->second)(json_node, caller);
    }

    catch( const JsonParseException& exception )
    {
        // the JSON arguments parsed without error, but the arguments did not
        IssueError(MGF::CS_json_argument_error_9205, GetActionName(action).c_str(), exception.GetErrorMessage().c_str());
    }

    catch( const CSProException& exception )
    {
        // add the action name to the error message
        throw ExceptionWithActionName(exception, GetActionName(action).c_str());
    }
}


ActionInvoker::Result ActionInvoker::Runtime::execute(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const Action action = GetActionFromJson(json_node);

    return RunFunction(action, json_node.GetOrEmpty(JK::arguments), caller);
}


ActionInvoker::Result ActionInvoker::Runtime::registerAccessToken(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    if( caller.IsExternalCaller() )
        throw CSProException("You cannot register access tokens while executing code in an external, untrusted, environment.");

    std::wstring access_token = json_node.Get<std::wstring>(JK::accessToken);

    if( SO::IsWhitespace(access_token) )
        throw CSProException("An access token cannot be blank.");

    RegisterAccessToken(std::move(access_token));

    return Result::Undefined();
}


InterpreterAccessor& ActionInvoker::Runtime::GetInterpreterAccessor()
{
    // get the interpreter if null, or if the execution stack has changed
    if( std::get<1>(m_interpreterAccessor) == nullptr || std::get<0>(m_interpreterAccessor) != ExecutionStack::GetStateId() )
        m_interpreterAccessor = std::make_tuple(ExecutionStack::GetStateId(), ObjectTransporter::GetInterpreterAccessor());

    ASSERT(std::get<1>(m_interpreterAccessor) != nullptr);
    return *std::get<1>(m_interpreterAccessor);
}
