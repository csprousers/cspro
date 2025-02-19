#include <engine/StandardSystemIncludes.h>
#include "gov_census_cspro_engine_EngineInterface_jni.h"
#include "AndroidApplicationInterface.h"
#include "JNIHelpers.h"
#include <zToolsO/Encryption.h>
#include <zToolsO/ExceptionHolder.h>
#include <zToolsO/UniqueId.h>
#include <zUtilO/ExecutionStack.h>
#include <zAction/JsonExecutor.h>
#include <zAction/Listener.h>
#include <zAction/WebController.h>


// --------------------------------------------------------------------------
// AndroidActionInvokerListener
// --------------------------------------------------------------------------

class AndroidActionInvokerListener : public ActionInvoker::Listener
{
public:
    AndroidActionInvokerListener(JNIEnv* pEnv, jobject jListener);

    std::tuple<JNIEnv*, jobject> GetJNIEnvAndListener() { return { m_pEnv, m_jListener }; }

    // Listener overrides
    SharableString OnGetDisplayOptions(ActionInvoker::Caller& caller) override;
    std::optional<bool> OnSetDisplayOptions(const JsonNode& json_node, ActionInvoker::Caller& caller) override;

    std::optional<bool> OnClose(CloseResult& close_result, ActionInvoker::Caller& caller) override;

    bool OnEngineProgramControlExecuted() override;

private:
    JNIEnv* m_pEnv;
    jobject m_jListener;
};


AndroidActionInvokerListener::AndroidActionInvokerListener(JNIEnv* pEnv, jobject jListener)
    :   m_pEnv(pEnv),
        m_jListener(jListener)
{
    ASSERT(pEnv != nullptr && jListener != nullptr);
}


SharableString AndroidActionInvokerListener::OnGetDisplayOptions(ActionInvoker::Caller& caller)
{
    jstring jDisplayOptionsJson = (jstring)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onGetDisplayOptions,
                                                                    caller.GetCallerId());

    ThrowJavaExceptionAsCSProException(m_pEnv);

    return JavaString::ToSharableString(*m_pEnv, jDisplayOptionsJson);
}


std::optional<bool> AndroidActionInvokerListener::OnSetDisplayOptions(const JsonNode& json_node, ActionInvoker::Caller& caller)
{
    const std::string display_options_json = json_node.GetNodeAsString();
    JNIReferences::scoped_local_ref<jstring> jDisplayOptionsJson(m_pEnv, JavaString::ToJava(*m_pEnv, display_options_json));

    auto jSuccessBoolean = (jobject)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onSetDisplayOptions,
                                                             jDisplayOptionsJson.get(), caller.GetCallerId());

    ThrowJavaExceptionAsCSProException(m_pEnv);

    if( jSuccessBoolean == nullptr )
        return std::nullopt;

    return m_pEnv->CallBooleanMethod(jSuccessBoolean, JNIReferences::methodBoolean_booleanValue);
}


std::optional<bool> AndroidActionInvokerListener::OnClose(CloseResult& close_result, ActionInvoker::Caller& caller)
{
    JNIReferences::scoped_local_ref<jstring> jResultsText(m_pEnv, nullptr);

    if( std::holds_alternative<const JsonNode>(close_result) )
        jResultsText = JavaString::ToJava(*m_pEnv, std::get<const JsonNode>(close_result).GetNodeAsString());

    auto jDialogClosedBoolean = (jobject)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onClose,
                                                                  jResultsText.get(), caller.GetCallerId());

    ThrowJavaExceptionAsCSProException(m_pEnv);

    if( jDialogClosedBoolean == nullptr )
        return std::nullopt;

    const bool window_closed = m_pEnv->CallBooleanMethod(jDialogClosedBoolean, JNIReferences::methodBoolean_booleanValue);

    // if the window was closed via an exception, pass this to the topmost ExceptionHolder
    if( window_closed && std::holds_alternative<std::unique_ptr<const ActionInvoker::Exception>>(close_result) )
    {
        ASSERT(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result) != nullptr);

        auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
        ExceptionHolder* const exception_holder = aai->GetTopmostExceptionHolder();

        if( exception_holder != nullptr )
            exception_holder->AddActionInvokerException(std::move(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result)));
    }

    return window_closed;
}


bool AndroidActionInvokerListener::OnEngineProgramControlExecuted()
{
    bool result = m_pEnv->CallBooleanMethod(m_jListener, JNIReferences::methodActionInvokerListener_onEngineProgramControlExecuted);

    ThrowJavaExceptionAsCSProException(m_pEnv);

    return result;
}



// --------------------------------------------------------------------------
// ActionInvokerData
// --------------------------------------------------------------------------

struct ActionInvokerData
{
    std::shared_ptr<ActionInvoker::Runtime> runtime;
    std::unique_ptr<ActionInvoker::WebController> web_controller;
    std::shared_ptr<AndroidActionInvokerListener> current_listener;
};



// --------------------------------------------------------------------------
// ActionInvoker::WebListener
// --------------------------------------------------------------------------

void ActionInvoker::WebListener::OnPostWebMessage(const std::string& message, const std::optional<std::string>& target_origin)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    const std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(m_callerId, false);

    if( action_invoker_data == nullptr || action_invoker_data->current_listener == nullptr )
    {
        ASSERT(false);
        return;
    }

    JNIEnv* pEnv;
    jobject jListener;
    std::tie(pEnv, jListener) = action_invoker_data->current_listener->GetJNIEnvAndListener();

    JNIReferences::scoped_local_ref<jstring> jMessage(pEnv, JavaString::ToJava(*pEnv, message));
    JNIReferences::scoped_local_ref<jstring> jTargetOrigin(pEnv, JavaString::ToJava(*pEnv, target_origin));

    pEnv->CallVoidMethod(jListener, JNIReferences::methodActionInvokerListener_onPostWebMessage,
                         jMessage.get(), jTargetOrigin.get());

    ThrowJavaExceptionAsCSProException(pEnv);
}



// --------------------------------------------------------------------------
// AndroidApplicationInterface
// --------------------------------------------------------------------------

int AndroidApplicationInterface::ActionInvokerCreateWebController(SharableString access_token_override)
{
    try
    {
        std::lock_guard<std::mutex> action_invoker_web_controllers_lock(m_actionInvokerWebControllersMutex);

        const int caller_id = UniqueId::CreateInt();
        auto web_controller = std::make_unique<ActionInvoker::WebController>(caller_id, nullptr);

        if( access_token_override.IsSet() )
            web_controller->GetCaller().AddAccessTokenOverride(std::move(access_token_override));

        m_actionInvokerWebControllers.try_emplace(caller_id, std::make_unique<ActionInvokerData>(
            ActionInvokerData
            {
                ObjectTransporter::GetActionInvokerRuntime(),
                std::move(web_controller)
            }));

        return caller_id;
    }

    catch( const CSProException& )
    {
        return ReturnProgrammingError(-1);
    }
}


std::shared_ptr<ActionInvokerData> AndroidApplicationInterface::ActionInvokerGetWebController(const int caller_id, const bool release_web_controller)
{
    std::lock_guard<std::mutex> action_invoker_web_controller(m_actionInvokerWebControllersMutex);

    const auto& lookup = m_actionInvokerWebControllers.find(caller_id);

    if( lookup == m_actionInvokerWebControllers.cend() )
        return ReturnProgrammingError(nullptr);

    std::shared_ptr<ActionInvokerData> action_invoker_data = lookup->second;

    if( release_web_controller )
        m_actionInvokerWebControllers.erase(lookup);

    return action_invoker_data;
}


ExceptionHolder* AndroidApplicationInterface::GetTopmostExceptionHolder()
{
    return !m_exceptionHolders.empty() ? m_exceptionHolders.back() :
                                         nullptr;
}



// --------------------------------------------------------------------------
// Java_gov_census_cspro_engine_EngineInterface
// --------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_gov_census_cspro_engine_EngineInterface_ActionInvokerCreateWebController
                       (JNIEnv* pEnv, jobject, jlong /*jNativeReference*/, jstring jActionInvokerAccessTokenOverride)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());

    return aai->ActionInvokerCreateWebController(JavaString::ToSharableString(*pEnv, jActionInvokerAccessTokenOverride));
}


JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ActionInvokerCancelAndWaitOnActionsInProgress
                       (JNIEnv* /*pEnv*/, jobject, jlong /*jNativeReference*/, jint jWebControllerKey)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    const std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(jWebControllerKey, true);

    if( action_invoker_data == nullptr )
    {
        ASSERT(false);
        return;
    }

    action_invoker_data->web_controller->CancelAndWaitOnActionsInProgress();
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_ActionInvokerProcessMessage
                          (JNIEnv* pEnv, jobject, jlong /*jNativeReference*/, jint jWebControllerKey, jobject jListener,
                           jstring jMessage, jboolean jAsync, jboolean jCalledByOldCSProObject)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    const std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(jWebControllerKey, false);

    if( action_invoker_data == nullptr )
        return ReturnProgrammingError(nullptr);

    const int message_id = action_invoker_data->web_controller->PushMessage(JavaString::ToUtf8(*pEnv, jMessage), jCalledByOldCSProObject);

    auto listener = std::make_shared<AndroidActionInvokerListener>(pEnv, jListener);
    ActionInvoker::ListenerHolder listener_holder = action_invoker_data->runtime->RegisterListener(listener);
    RAII::SetValueAndRestoreOnDestruction android_invoker_data_current_listener_holder(action_invoker_data->current_listener, listener);

    const SharableString response = action_invoker_data->web_controller->ProcessMessage(message_id, jAsync);

    return JavaString::ToJava(*pEnv, response);
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_oldCSProJavaScriptInterfaceGetAccessToken
                          (JNIEnv* pEnv, jobject, jlong /*jNativeReference*/)
{
    return JavaString::ToJava(*pEnv, OldCSProJavaScriptInterface::GetAccessToken());
}



// --------------------------------------------------------------------------
// ActionInvokerActivityCaller
// --------------------------------------------------------------------------

class ActionInvokerActivityCaller : public ActionInvoker::ExternalCaller
{
public:
    ActionInvokerActivityCaller(std::string calling_package)
        :   ExternalCaller(UniqueId::CreateInt()),
            m_callingPackage(std::move(calling_package))
    {
    }

    // refresh token management
    void SetRefreshToken(const std::string& refresh_token);
    std::optional<std::string> CreateRefreshToken() const;

    // Caller overrides
    CancelFlag& GetCancelFlag() override
    {
        return m_cancelFlag;
    }

    std::string GetRootDirectory() override
    {
        return PlatformInterface::GetInstance()->GetCSEntryDirectory();
    }

private:
    std::string m_callingPackage;
    CancelFlag m_cancelFlag;

    static constexpr int64_t RefreshTokenExpirationSeconds = DateHelper::SecondsInHour();
    static std::unique_ptr<std::tuple<std::string, std::vector<std::byte>>> m_refreshTokenDetails;
};


// the refresh token will be a GUID, with that as the key for an Encryptor that contains:
// - UTF-8 string of the package name
// - int64_t: the issued timestamp
std::unique_ptr<std::tuple<std::string, std::vector<std::byte>>> ActionInvokerActivityCaller::m_refreshTokenDetails;


void ActionInvokerActivityCaller::SetRefreshToken(const std::string& refresh_token)
{
    // for now there is only one refresh token per instance, which is valid only once,
    // but in the future we could store multiple versions of the tokens
    auto refresh_token_details = std::move(m_refreshTokenDetails);

    if( refresh_token_details == nullptr || refresh_token != std::get<0>(*refresh_token_details) )
        return;

    Encryptor encryptor(Encryptor::Type::RijndaelBase64, refresh_token);
    const std::vector<std::byte> calling_package_and_timestamp = encryptor.Decrypt(std::get<1>(*refresh_token_details));

    const size_t calling_package_length = calling_package_and_timestamp.size() - sizeof(int64_t);

    if( calling_package_length < calling_package_and_timestamp.size() )
    {
        const int64_t* const timestamp_ptr = reinterpret_cast<const int64_t*>(calling_package_and_timestamp.data() + calling_package_length);

        if( ( *timestamp_ptr + RefreshTokenExpirationSeconds ) >= GetTimestamp<int64_t>() )
        {
            const std::string calling_package(reinterpret_cast<const char*>(calling_package_and_timestamp.data()), calling_package_length);

            // at this point, the timestamp is valid, and the package name matches, so override the need to use access tokens
            if( calling_package == m_callingPackage )
                SetUserOverrodeAccessTokenRequirement(true);
        }
    }
}


std::optional<std::string> ActionInvokerActivityCaller::CreateRefreshToken() const
{
    // only issue refresh tokens when the user override the need to use access tokens
    if( !GetUserOverrodeAccessTokenRequirement() )
        return std::nullopt;

    std::string refresh_token = CreateUuid();
    Encryptor encryptor(Encryptor::Type::RijndaelBase64, refresh_token);

    std::vector<std::byte> calling_package_and_timestamp = SO::CreateByteVector(m_callingPackage);
    const int64_t timestamp = GetTimestamp<int64_t>();
    const std::byte* const timestamp_ptr = reinterpret_cast<const std::byte*>(&timestamp);
    calling_package_and_timestamp.insert(calling_package_and_timestamp.end(), timestamp_ptr, timestamp_ptr + sizeof(timestamp));

    m_refreshTokenDetails = std::make_unique<std::tuple<std::string, std::vector<std::byte>>>(refresh_token, encryptor.Encrypt(calling_package_and_timestamp));

    return refresh_token;
}



// --------------------------------------------------------------------------
// Java_gov_census_cspro_engine_EngineInterface (for ActionInvokerActivity)
// --------------------------------------------------------------------------

JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_RunActionInvoker
                          (JNIEnv* pEnv, jobject, jlong, jstring jCallingPackage, jstring jAction, jstring jAccessToken, jstring jRefreshToken, jboolean jAbortOnException)
{
    ActionInvokerActivityCaller caller(JavaString::ToUtf8(*pEnv, jCallingPackage));

    if( jAccessToken != nullptr )
        caller.AddAccessTokenOverride(JavaString::ToUtf8(*pEnv, jAccessToken));

    if( jRefreshToken != nullptr )
        caller.SetRefreshToken(JavaString::ToUtf8(*pEnv, jRefreshToken));

    SharableString result;

    try
    {
        ActionInvoker::JsonExecutor json_executor(false);

        json_executor.ParseActions(JavaString::ToUtf8(*pEnv, jAction));

        json_executor.SetAbortOnException(jAbortOnException);

        // add ActionInvokerActivity to the execution stack
        {
            const ExecutionStackEntry execution_stack_entry = ExecutionStack::AddEntry(ExecutionStack::ActionInvokerActivity { });

            json_executor.RunActions(caller);
        }

        result = json_executor.ReleaseResultsJson();
    }

    catch( const CSProException& exception )
    {
        result = ActionInvoker::JsonResponse(exception).GetResponseText();
    }

    ASSERT(result.IsSet());

    return pEnv->NewObject(JNIReferences::classActionInvokerActivityResult,
                           JNIReferences::methodActionInvokerActivityResultConstructor,
                           JavaString::ToJava(*pEnv, *result),
                           JavaString::ToJava(*pEnv, caller.CreateRefreshToken()));
}
