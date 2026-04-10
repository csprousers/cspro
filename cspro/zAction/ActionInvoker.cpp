#include "stdafx.h"
#include "ActionInvoker.h"
#include "ExceptionThrowingJsonReaderInterface.h"
#include "NameProcessors.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/UniqueId.h>
#include <zUtilO/ExecutionStack.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/Messages.h>
#include <zHtml/VirtualFileMapping.h>


ActionInvoker::Runtime::Runtime()
    :   m_exceptionThrowingJsonReaderInterface(std::make_unique<ExceptionThrowingJsonReaderInterface>()),
        m_registeredAccessTokensForExternalCallers(std::make_unique<std::set<std::string>>()),
        m_listeners(std::make_unique<std::vector<Listener*>>())
{
}


ActionInvoker::Runtime::~Runtime()
{
}


void ActionInvoker::Runtime::DisableAccessTokenCheckForExternalCallers()
{
    m_registeredAccessTokensForExternalCallers.reset();
}


void ActionInvoker::Runtime::RegisterAccessToken(std::string access_token)
{
    if( m_registeredAccessTokensForExternalCallers != nullptr )
        m_registeredAccessTokensForExternalCallers->insert(std::move(access_token));
}


void ActionInvoker::Runtime::CheckAccessToken(const std::string* const access_token, Caller& caller)
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
    const Application* const application = GetApplication(false);
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

            for( const std::string& logic_settings_access_token : logic_settings.GetActionInvokerAccessTokens() )
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
        const SharableString user_prompt_message = MGF::GetMessageText(MGF::CS_access_without_token_prompt_9208,
                                                                       "A web page or program is attempting to access CSPro functionality, which can include access to data or to files on your device. "
                                                                       "Do you want to allow this? You should only allow this if you trust this source.");
        int result;

#ifdef WIN_DESKTOP
        result = AfxMessageBox(user_prompt_message.GetString(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
#else
        result = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog("Allow Access?", user_prompt_message.GetString(), MB_YESNO);
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


ActionInvoker::Result ActionInvoker::Runtime::ProcessExecute(const std::string& json_arguments, Caller& caller)
{
    const JsonNode json_node = ParseJson(json_arguments, caller, nullptr);
    const Action action = GetActionFromJson(json_node);

    return RunFunction(action, json_node, caller);
}


ActionInvoker::Result ActionInvoker::Runtime::ProcessAction(const Action action, const SharableString& json_arguments, Caller& caller)
{
    if( json_arguments.IsSet() )
    {
        return RunFunction(action, ParseJson(json_arguments.GetString(), caller, &action), caller);
    }

    else
    {
        static const JsonNode no_arguments_json_node = Json::Parse(Json::Text::EmptyObject_sv);

        return RunFunction(action, no_arguments_json_node, caller);
    }
}


JsonNode ActionInvoker::Runtime::ParseJson(const std::string_view json_arguments_sv, Caller& caller, const Action* const action)
{
    try
    {
        // using ExceptionThrowingJsonReaderInterface will result in invalid access requests to be thrown as exceptions
        assert_cast<ExceptionThrowingJsonReaderInterface*>(m_exceptionThrowingJsonReaderInterface.get())->SetDirectory(caller.GetRootDirectory());
        return Json::Parse(json_arguments_sv, m_exceptionThrowingJsonReaderInterface.get());
    }

    catch( const JsonParseException& exception )
    {
        IssueError(MGF::CS_json_argument_error_9205, GetActionName(action).c_str(), exception.what());
    }
}


ActionInvoker::Action ActionInvoker::Runtime::GetActionFromJson(const JsonNode& json_node)
{
    if( !json_node.Contains(JK::action) )
        IssueError(MGF::CS_action_missing_9201);

    return GetActionFromText(json_node.Get<std::string_view>(JK::action), *this);
}


ActionInvoker::Result ActionInvoker::Runtime::RunFunction(const Action action, const JsonNode& json_node, Caller& caller)
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
        IssueError(MGF::CS_json_argument_error_9205, GetActionName(action).c_str(), exception.what());
    }

    catch( const ActionInvoker::Exception& )
    {
        // throw ActionInvoker::Exception exceptions directly
        throw;
    }

    catch( const CSProException& exception )
    {
        // rethrow as an ActionInvoker::Exception with the action name as the error cause
        const std::string action_name = GetActionName(action);

        throw ActionInvoker::Exception(FormatText("Error running action '%s': %s", action_name.c_str(), exception.what()),
                                       Encoders::ToJsonString(action_name),
                                       std::nullopt);
    }
}


ActionInvoker::Result ActionInvoker::Runtime::execute(const JsonNode& json_node, Caller& caller)
{
    const Action action = GetActionFromJson(json_node);

    return RunFunction(action, json_node.GetOrEmpty(JK::arguments), caller);
}


ActionInvoker::Result ActionInvoker::Runtime::registerAccessToken(const JsonNode& json_node, Caller& caller)
{
    if( caller.IsExternalCaller() )
        throw CSProException("You cannot register access tokens while executing code in an external, untrusted, environment.");

    std::string access_token = json_node.Get<std::string>(JK::accessToken);

    if( SO::IsWhitespace(access_token) )
        throw CSProException("An access token cannot be blank.");

    RegisterAccessToken(std::move(access_token));

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::throwException(const JsonNode& json_node, Caller& /*caller*/)
{
    throw ActionInvoker::Exception(json_node, true);
}


int ActionInvoker::Runtime::CreateResourceId(const Resource resource, Caller& caller)
{
    const int resource_id = UniqueId::CreateInt();
    m_resourceIdCallerMap[caller.GetCallerId()].emplace_back(resource, resource_id);
    return resource_id;
}


int ActionInvoker::Runtime::GetResourceId(const Resource resource, const JsonNode& json_node, Caller& caller, const char* const id_key,
                                          const char* const not_specified_formatter, const char* const multiple_implicit_formatter) const
{
    if( json_node.Contains(id_key) )
        return json_node.Get<int>(id_key);

    const auto& lookup = m_resourceIdCallerMap.find(caller.GetCallerId());

    if( lookup != m_resourceIdCallerMap.cend() )
    {
        std::optional<int> resource_id;

        for( const auto& [this_resource, this_resource_id] : lookup->second )
        {
            if( resource == this_resource )
            {
                if( resource_id.has_value() )
                    throw CSProException(multiple_implicit_formatter, id_key);

                resource_id = this_resource_id;
            }
        }

        if( resource_id.has_value() )
            return *resource_id;
    }

    throw CSProException(not_specified_formatter, id_key);
}


void ActionInvoker::Runtime::DestroyResourceId(const int resource_id)
{
    for( auto& [caller_id, resource_and_id] : m_resourceIdCallerMap )
    {
        const auto& resource_and_id_end = resource_and_id.end();

        for( auto resource_and_id_itr = resource_and_id.begin();
             resource_and_id_itr != resource_and_id_end;
             ++resource_and_id_itr )
        {
            if( resource_id == std::get<1>(*resource_and_id_itr) )
            {
                resource_and_id.erase(resource_and_id_itr);

                if( resource_and_id.empty() )
                    m_resourceIdCallerMap.erase(caller_id);

                return;
            }
        }
    }

    ASSERT(false);
}


InterpreterAccessor& ActionInvoker::Runtime::GetInterpreterAccessor()
{
    // get the interpreter if null, or if the execution stack has changed
    if( std::get<1>(m_interpreterAccessor) == nullptr || std::get<0>(m_interpreterAccessor) != ExecutionStack::GetStateId() )
        m_interpreterAccessor = std::make_tuple(ExecutionStack::GetStateId(), ObjectTransporter::GetInterpreterAccessor());

    ASSERT(std::get<1>(m_interpreterAccessor) != nullptr);
    return *std::get<1>(m_interpreterAccessor);
}
