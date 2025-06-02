#include <engine/StandardSystemIncludes.h>
#include "AndroidHtmlMapUI.h"
#include "JNIHelpers.h"
#include <zToolsO/RaiiHelpers.h>


namespace
{
    constexpr useconds_t SleepInterval = 100 * 1000; // 100 milliseconds
}


namespace RequestType
{
    constexpr int POST_WEB_MESSAGE = 1;
    constexpr int HIDE             = 2;
    constexpr int SAVE_SNAPSHOT    = 3;
    constexpr int SET_WINDOW_TITLE = 4;
}


AndroidHtmlMapUI::AndroidHtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   HtmlMapUI(std::move(mapping_properties)),
        m_jHtmlMapActivity(nullptr),
        m_waitingForMapEvent(false)
{
}


AndroidHtmlMapUI::~AndroidHtmlMapUI()
{
    if( m_jHtmlMapActivity != nullptr )
    {
        ASSERT(false);
        NotifyLifecycle(nullptr);
    }
}


void AndroidHtmlMapUI::NotifyLifecycle(jobject jHtmlMapActivity)
{
    JNIEnv* const jni_env = GetJNIEnvForCurrentThread();

    // called from HtmlMapActivity's onCreate
    if( jHtmlMapActivity != nullptr )
    {
        m_jHtmlMapActivity = jni_env->NewGlobalRef(jHtmlMapActivity);
    }

    // called from HtmlMapActivity's onDestroy
    else
    {
        jni_env->DeleteGlobalRef(m_jHtmlMapActivity);
        m_jHtmlMapActivity = nullptr;

        // if still waiting for an event, set the event to the map closing
        if( m_waitingForMapEvent )
            NotifyEvent(EventCode::MapClosed);
    }
}


void AndroidHtmlMapUI::NotifyWebMessageReceived(const std::string_view event_json_sv)
{
    OnWebMessageReceived(event_json_sv);
}


bool AndroidHtmlMapUI::Show()
{
    if( !AndroidHtmlMapUI::IsMapShowing() )
    {
        JNIEnv* const jni_env = GetJNIEnvForCurrentThread();

        JNIReferences::scoped_local_ref<jstring> jMappingUrl(jni_env, JavaString::ToJava(*jni_env, GetUrlOfMapHtml()));

        jni_env->CallStaticVoidMethod(
            JNIReferences::classApplicationInterface,
            JNIReferences::methodApplicationInterfaceLaunchHtmlMap,
            reinterpret_cast<jlong>(this),
            jMappingUrl.get()
        );
    }

    return true;
}


bool AndroidHtmlMapUI::Hide()
{
    ASSERT(!m_waitingForMapEvent);

    if( !AndroidHtmlMapUI::IsMapShowing() )
        return false;

    JNIEnv* const jni_env = GetJNIEnvForCurrentThread();

    jni_env->CallVoidMethod(
        m_jHtmlMapActivity,
        JNIReferences::methodHtmlMapActivityHandleRequest,
        RequestType::HIDE,
        nullptr
    );

    // wait for the activity to finish
    while( AndroidHtmlMapUI::IsMapShowing() )
        usleep(SleepInterval);

    return true;
}


bool AndroidHtmlMapUI::SaveSnapshot(const std::string& image_file_path)
{
    ASSERT(IsMapShowing());

    JNIEnv* const jni_env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jImageFilePath(jni_env, JavaString::ToJava(*jni_env, image_file_path));

    jni_env->CallVoidMethod(
        m_jHtmlMapActivity,
        JNIReferences::methodHtmlMapActivityHandleRequest,
        RequestType::SAVE_SNAPSHOT,
        jImageFilePath.get()
    );

    ThrowJavaExceptionAsCSProException(jni_env);

    return true;
}


IMapUI::MapEvent AndroidHtmlMapUI::WaitForEvent()
{
    // wait until we have received an event (via NotifyWebMessageReceived or NotifyLifecycle)
    ASSERT(!m_waitingForMapEvent);
    const RAII::SetValueAndRestoreOnDestruction waiting_modifier(m_waitingForMapEvent, true);

    while( m_mapEvent == nullptr )
        usleep(SleepInterval);

    std::unique_ptr<MapEvent> received_map_event;

    // lock guard
    {
        std::lock_guard<std::mutex> lock(m_mapEventMutex);
        received_map_event = std::move(m_mapEvent);
    }

    return *received_map_event;
}


bool AndroidHtmlMapUI::IsMapShowing()
{
    return ( m_jHtmlMapActivity != nullptr );
}


void AndroidHtmlMapUI::OnPostActionMessage(const SharableString action_message_json)
{
    ASSERT(IsMapShowing());

    JNIEnv* const jni_env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jActionMessageJson(jni_env, JavaString::ToJava(*jni_env, *action_message_json));

    jni_env->CallVoidMethod(
        m_jHtmlMapActivity,
        JNIReferences::methodHtmlMapActivityHandleRequest,
        RequestType::POST_WEB_MESSAGE,
        jActionMessageJson.get()
    );
}


void AndroidHtmlMapUI::OnNotifyEvent(std::unique_ptr<MapEvent> event)
{
    ASSERT(event != nullptr);

    // wait until any existing events have been processed by the engine
    while( m_mapEvent != nullptr )
        Sleep(SleepInterval);

    std::lock_guard<std::mutex> lock(m_mapEventMutex);
    m_mapEvent = std::move(event);
}


void AndroidHtmlMapUI::OnSetWindowTitle(const std::string& title)
{
    if( !AndroidHtmlMapUI::IsMapShowing() )
        return;

    JNIEnv* const jni_env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jTitle(jni_env, JavaString::ToJava(*jni_env, title));

    jni_env->CallVoidMethod(
        m_jHtmlMapActivity,
        JNIReferences::methodHtmlMapActivityHandleRequest,
        RequestType::SET_WINDOW_TITLE,
        jTitle.get()
    );
}


bool AndroidHtmlMapUI::OnShowCurrentLocation()
{
    return false; // MAP_TODO
}
