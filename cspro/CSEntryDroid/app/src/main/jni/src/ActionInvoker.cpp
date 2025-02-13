#include <engine/StandardSystemIncludes.h>
#include "gov_census_cspro_engine_EngineInterface_jni.h"
#include "AndroidApplicationInterface.h"
#include "JNIHelpers.h"
#include <zToolsO/Encryption.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/ExecutionStack.h>
#include <zAction/JsonExecutor.h>
#include <zAction/Listener.h>
#include <zAction/WebController.h>


namespace
{
    constexpr int InvalidWebControllerKey = -1;
}


// --------------------------------------------------------------------------
// AndroidActionInvokerListener
// --------------------------------------------------------------------------

class AndroidActionInvokerListener : public ActionInvoker::Listener
{
public:
    AndroidActionInvokerListener(JNIEnv* pEnv, jobject jListener);

    std::tuple<JNIEnv*, jobject> GetJNIEnvAndListener() { return { m_pEnv, m_jListener }; }

    // Listener overrides
    std::optional<std::wstring> OnGetDisplayOptions(ActionInvoker::Caller& caller) override;
    std::optional<bool> OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller) override;

    std::optional<bool> OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& caller) override;

    bool OnEngineProgramControlExecuted() override;

private:
    static int GetWebControllerKey(const ActionInvoker::Caller& caller);

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


int AndroidActionInvokerListener::GetWebControllerKey(const ActionInvoker::Caller& caller)
{
    std::optional<ActionInvoker::Caller::WebViewTag> web_view_tag = caller.GetWebViewTag();
    return web_view_tag.value_or(InvalidWebControllerKey);
}


std::optional<std::wstring> AndroidActionInvokerListener::OnGetDisplayOptions(ActionInvoker::Caller& caller)
{
    jint jWebControllerKey = GetWebControllerKey(caller);

    jstring jDisplayOptionsJson = (jstring)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onGetDisplayOptions,
                                                                    jWebControllerKey);

    ThrowJavaExceptionAsCSProException(m_pEnv);

    if( jDisplayOptionsJson == nullptr )
        return std::nullopt;

    return JavaToWSZ(m_pEnv, jDisplayOptionsJson);
}


std::optional<bool> AndroidActionInvokerListener::OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, ActionInvoker::Caller& caller)
{
    std::wstring display_options_json = json_node.GetNodeAsString();
    JNIReferences::scoped_local_ref<jstring> jDisplayOptionsJson(m_pEnv, WideToJava(m_pEnv, display_options_json));

    jint jWebControllerKey = GetWebControllerKey(caller);

    auto jSuccessBoolean = (jobject)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onSetDisplayOptions,
                                                             jDisplayOptionsJson.get(), jWebControllerKey);

    ThrowJavaExceptionAsCSProException(m_pEnv);

    if( jSuccessBoolean == nullptr )
        return std::nullopt;

    return m_pEnv->CallBooleanMethod(jSuccessBoolean, JNIReferences::methodBoolean_booleanValue);
}


std::optional<bool> AndroidActionInvokerListener::OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& caller)
{
    JNIReferences::scoped_local_ref<jstring> jResultsText(m_pEnv, nullptr);

    if( !result_node.IsEmpty() )
        jResultsText = WideToJava(m_pEnv, result_node.GetNodeAsString());

    jint jWebControllerKey = GetWebControllerKey(caller);

    auto jDialogClosedBoolean = (jobject)m_pEnv->CallObjectMethod(m_jListener, JNIReferences::methodActionInvokerListener_onCloseDialog,
                                                                  jResultsText.get(), jWebControllerKey);

    ThrowJavaExceptionAsCSProException(m_pEnv);

    if( jDialogClosedBoolean == nullptr )
        return std::nullopt;

    return m_pEnv->CallBooleanMethod(jDialogClosedBoolean, JNIReferences::methodBoolean_booleanValue);
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

void ActionInvoker::WebListener::OnPostWebMessage(const std::wstring& message, const std::optional<std::wstring>& target_origin)
{
    jint jWebControllerKey = m_webViewTag;

    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(jWebControllerKey, false);

    if( action_invoker_data == nullptr || action_invoker_data->current_listener == nullptr )
    {
        ASSERT(false);
        return;
    }

    JNIEnv* pEnv;
    jobject jListener;
    std::tie(pEnv, jListener) = action_invoker_data->current_listener->GetJNIEnvAndListener();

    JNIReferences::scoped_local_ref<jstring> jMessage(pEnv, WideToJava(pEnv, message));
    JNIReferences::scoped_local_ref<jstring> jTargetOrigin(pEnv, OptionalWideToJava(pEnv, target_origin));

    pEnv->CallVoidMethod(jListener, JNIReferences::methodActionInvokerListener_onPostWebMessage,
                         jMessage.get(), jTargetOrigin.get());

    ThrowJavaExceptionAsCSProException(pEnv);
}



// --------------------------------------------------------------------------
// AndroidApplicationInterface
// --------------------------------------------------------------------------

int AndroidApplicationInterface::ActionInvokerCreateWebController(const std::wstring* const access_token_override)
{
    try
    {
        int web_controller_key = static_cast<int>(m_actionInvokerWebControllers.size());

        std::lock_guard<std::mutex> action_invoker_web_controllers_lock(m_actionInvokerWebControllersMutex);

        while( m_actionInvokerWebControllers.find(web_controller_key) != m_actionInvokerWebControllers.cend() )
            ++web_controller_key;

        auto web_controller = std::make_unique<ActionInvoker::WebController>(web_controller_key);

        if( access_token_override != nullptr )
            web_controller->GetCaller().AddAccessTokenOverride(*access_token_override);

        m_actionInvokerWebControllers.try_emplace(web_controller_key, std::make_shared<ActionInvokerData>(
            ActionInvokerData
            {
                ObjectTransporter::GetActionInvokerRuntime(),
                std::move(web_controller)
            }));

        return web_controller_key;
    }

    catch( const CSProException& )
    {
        return ReturnProgrammingError(InvalidWebControllerKey);
    }
}


std::shared_ptr<ActionInvokerData> AndroidApplicationInterface::ActionInvokerGetWebController(int web_controller_key, bool release_web_controller)
{
    std::lock_guard<std::mutex> action_invoker_web_controller(m_actionInvokerWebControllersMutex);

    const auto& lookup = m_actionInvokerWebControllers.find(web_controller_key);

    if( lookup == m_actionInvokerWebControllers.cend() )
        return ReturnProgrammingError(nullptr);

    std::shared_ptr<ActionInvokerData> action_invoker_data = lookup->second;

    if( release_web_controller )
        m_actionInvokerWebControllers.erase(lookup);

    return action_invoker_data;
}



// --------------------------------------------------------------------------
// Java_gov_census_cspro_engine_EngineInterface
// --------------------------------------------------------------------------

JNIEXPORT jint JNICALL Java_gov_census_cspro_engine_EngineInterface_ActionInvokerCreateWebController
                       (JNIEnv* pEnv, jobject, jlong /*jNativeReference*/, jstring jActionInvokerAccessTokenOverride)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());

    std::unique_ptr<std::wstring> access_token_override;

    if( jActionInvokerAccessTokenOverride != nullptr )
        access_token_override = std::make_unique<std::wstring>(JavaToWSZ(pEnv, jActionInvokerAccessTokenOverride));

    return aai->ActionInvokerCreateWebController(access_token_override.get());
}


JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ActionInvokerCancelAndWaitOnActionsInProgress
                       (JNIEnv* /*pEnv*/, jobject, jlong /*jNativeReference*/, jint jWebControllerKey)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(jWebControllerKey, true);

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
    std::shared_ptr<ActionInvokerData> action_invoker_data = aai->ActionInvokerGetWebController(jWebControllerKey, false);

    if( action_invoker_data == nullptr )
        return ReturnProgrammingError(nullptr);

    int message_id = action_invoker_data->web_controller->PushMessage(JavaToWSZ(pEnv, jMessage), jCalledByOldCSProObject);

    auto listener = std::make_shared<AndroidActionInvokerListener>(pEnv, jListener);
    ActionInvoker::ListenerHolder listener_holder = action_invoker_data->runtime->RegisterListener(listener);
    RAII::SetValueAndRestoreOnDestruction android_invoker_data_current_listener_holder(action_invoker_data->current_listener, listener);

    const std::shared_ptr<const std::wstring> response = action_invoker_data->web_controller->ProcessMessage(message_id, jAsync);

    return ( response != nullptr ) ? WideToJava(pEnv, *response) :
                                     nullptr;
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_oldCSProJavaScriptInterfaceGetAccessToken
                          (JNIEnv* pEnv, jobject, jlong /*jNativeReference*/)
{
    return WideToJava(pEnv, OldCSProJavaScriptInterface::GetAccessToken());
}



// --------------------------------------------------------------------------
// ActionInvokerActivityCaller +
// --------------------------------------------------------------------------

class ActionInvokerActivityCaller : public ActionInvoker::ExternalCaller
{
public:
    ActionInvokerActivityCaller(std::wstring calling_package)
        :   m_callingPackage(std::move(calling_package)),
            m_cancelFlag(false)
    {
    }

    // refresh token management
    void SetRefreshToken(const std::wstring& refresh_token);
    std::optional<std::wstring> CreateRefreshToken() const;

    // Caller overrides
    bool& GetCancelFlag() override
    {
        return m_cancelFlag;
    }

    std::wstring GetRootDirectory() override
    {
        return PlatformInterface::GetInstance()->GetCSEntryDirectory();
    }

private:
    const std::wstring m_callingPackage;
    bool m_cancelFlag;

    static constexpr int64_t RefreshTokenExpirationSeconds = DateHelper::SecondsInHour();
    static std::unique_ptr<std::tuple<std::wstring, std::vector<std::byte>>> m_refreshTokenDetails;
};


// the refresh token will be a GUID, with that as the key for an Encryptor that contains:
// - UTF-8 string of the package name
// - int64_t: the issued timestamp
std::unique_ptr<std::tuple<std::wstring, std::vector<std::byte>>> ActionInvokerActivityCaller::m_refreshTokenDetails;


void ActionInvokerActivityCaller::SetRefreshToken(const std::wstring& refresh_token)
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
            const std::wstring calling_package = UTF8Convert::UTF8ToWide(reinterpret_cast<const char*>(calling_package_and_timestamp.data()), calling_package_length);

            // at this point, the timestamp is valid, and the package name matches, so override the need to use access tokens
            if( calling_package == m_callingPackage )
                SetUserOverrodeAccessTokenRequirement(true);
        }
    }
}


std::optional<std::wstring> ActionInvokerActivityCaller::CreateRefreshToken() const
{
    // only issue refresh tokens when the user override the need to use access tokens
    if( !GetUserOverrodeAccessTokenRequirement() )
        return std::nullopt;

    std::wstring refresh_token = CreateUuid();
    Encryptor encryptor(Encryptor::Type::RijndaelBase64, refresh_token);

    std::vector<std::byte> calling_package_and_timestamp = UTF8Convert::WideToUTF8Buffer(m_callingPackage);
    const int64_t timestamp = GetTimestamp<int64_t>();
    const std::byte* const timestamp_ptr = reinterpret_cast<const std::byte*>(&timestamp);
    calling_package_and_timestamp.insert(calling_package_and_timestamp.end(), timestamp_ptr, timestamp_ptr + sizeof(timestamp));

    m_refreshTokenDetails = std::make_unique<std::tuple<std::wstring, std::vector<std::byte>>>(refresh_token, encryptor.Encrypt(calling_package_and_timestamp));

    return refresh_token;
}



// --------------------------------------------------------------------------
// Java_gov_census_cspro_engine_EngineInterface (for ActionInvokerActivity)
// --------------------------------------------------------------------------

JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_RunActionInvoker
                          (JNIEnv* pEnv, jobject, jlong, jstring jCallingPackage, jstring jAction, jstring jAccessToken, jstring jRefreshToken, jboolean jAbortOnException)
{
    ActionInvokerActivityCaller caller(JavaToWSZ(pEnv, jCallingPackage));

    if( jAccessToken != nullptr )
        caller.AddAccessTokenOverride(JavaToWSZ(pEnv, jAccessToken));

    if( jRefreshToken != nullptr )
        caller.SetRefreshToken(JavaToWSZ(pEnv, jRefreshToken));

    std::shared_ptr<const std::wstring> result;

	try
	{
        ActionInvoker::JsonExecutor json_executor(false);

        json_executor.ParseActions(JavaToWSZ(pEnv, jAction));

        json_executor.SetAbortOnException(jAbortOnException);

        // add ActionInvokerActivity to the execution stack
        {
            const ExecutionStackEntry execution_stack_entry = ExecutionStack::AddEntry(ExecutionStack::ActionInvokerActivity { });

            json_executor.RunActions(caller);
        }

        result = json_executor.GetResultsJson();
	}

	catch( const CSProException& exception )
	{
        result = ActionInvoker::JsonResponse(exception).GetSharedResponseText();
	}

    ASSERT(result != nullptr);

    return pEnv->NewObject(JNIReferences::classActionInvokerActivityResult,
                           JNIReferences::methodActionInvokerActivityResultConstructor,
                           WideToJava(pEnv, *result),
                           OptionalWideToJava(pEnv, caller.CreateRefreshToken()));
}
