#pragma once

#include <zToolsO/ObjectTransporter.h>
#include <zHtml/UseHtmlDialogs.h> // temporarily for OldCSProJavaScriptInterface::GetAccessToken
#include <zHtml/WebViewSyncOperationMarker.h>
#include <zAction/ActionInvoker.h>
#include <zAction/Listener.h>
#include <zAction/JsonResponse.h>
#include <zAction/WebCaller.h>
#include <zAction/WebListener.h>
#include <mutex>
#include <thread>


namespace ActionInvoker
{
    class WebController
    {
    public:
        struct Message
        {
            int message_id = -1;
            std::wstring message;
            bool called_by_old_CSPro_object = false;
        };

        class PreProcessMessageWorker
        {
        public:
            virtual ~PreProcessMessageWorker() { }
            virtual std::shared_ptr<Listener> GetListener() = 0;
        };


        WebController(Caller::WebViewTag web_view_tag);

        WebListener& GetListener() { return *m_actionInvokerWebListener; }
        WebCaller& GetCaller()     { return m_actionInvokerCaller; }

        // adds a message for processing, returning the message's unique ID
        int PushMessage(std::wstring message, bool called_by_old_CSPro_object = false);

        // cancels any operations in progress and waits for them to complete
        void CancelAndWaitOnActionsInProgress();

        // sets an object that will be called prior to a message being processed
        void SetPreProcessMessageWorker(std::unique_ptr<PreProcessMessageWorker> pre_process_message_worker);

        // processes a message:
        // - if synchronous, the result is returned;
        // - if asynchronous, the JavaScript call to update the Promise is returned
        std::shared_ptr<const std::wstring> ProcessMessage(int message_id, bool async_call);

    private:
        // returns the message associated with the unique ID
        Message PopMessage(int message_id);

    private:
        std::mutex m_messagesMutex;
        int m_nextMessageId;
        std::vector<Message> m_messages;

        std::mutex m_processActionMutex;
        bool m_webControllerShutdownFlag;

        std::shared_ptr<WebListener> m_actionInvokerWebListener;
        std::unique_ptr<ListenerHolder> m_actionInvokerListenerHolder;
        WebCaller m_actionInvokerCaller;
        std::shared_ptr<Runtime> m_actionInvokerRuntime;
        std::unique_ptr<PreProcessMessageWorker> m_preProcessMessageWorker;
    };
}


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ActionInvoker::WebController::WebController(const Caller::WebViewTag web_view_tag)
    :   m_nextMessageId(1),
        m_webControllerShutdownFlag(false),
        m_actionInvokerWebListener(std::make_shared<WebListener>(web_view_tag)),
        m_actionInvokerListenerHolder(ListenerHolder::Register(m_actionInvokerWebListener)),
        m_actionInvokerCaller(web_view_tag)
{
}


inline int ActionInvoker::WebController::PushMessage(std::wstring message, const bool called_by_old_CSPro_object/* = false*/)
{
    std::lock_guard<std::mutex> messages_lock(m_messagesMutex);

    m_messages.emplace_back(Message { m_nextMessageId, std::move(message), called_by_old_CSPro_object });

    return m_nextMessageId++;
}


inline ActionInvoker::WebController::Message ActionInvoker::WebController::PopMessage(const int message_id)
{
    std::lock_guard<std::mutex> messages_lock(m_messagesMutex);

    auto lookup = std::find_if(m_messages.begin(), m_messages.end(),
                               [&](const Message& message) { return ( message.message_id == message_id ); });

    if( lookup == m_messages.end() )
        return ReturnProgrammingError(Message());

    Message message = std::move(*lookup);

    m_messages.erase(lookup);

    return message;
}


inline void ActionInvoker::WebController::CancelAndWaitOnActionsInProgress()
{
    // cancel any actions...
    m_webControllerShutdownFlag = true;
    m_actionInvokerCaller.SetCancelFlag(true);

    // ...and make sure that the actions are finished
    std::lock_guard<std::mutex> async_operation_lock(m_processActionMutex);
}


inline void ActionInvoker::WebController::SetPreProcessMessageWorker(std::unique_ptr<PreProcessMessageWorker> pre_process_message_worker)
{
    ASSERT(pre_process_message_worker != nullptr);
    m_preProcessMessageWorker = std::move(pre_process_message_worker);
}


inline std::shared_ptr<const std::wstring> ActionInvoker::WebController::ProcessMessage(const int message_id, const bool async_call)
{
    Message message = PopMessage(message_id);
    std::optional<int> request_id;

    auto generate_response = [&](const JsonResponse& json_response) -> std::shared_ptr<const std::wstring>
    {
        // if no request ID was provided, the asynchronous call is invalid
        if( async_call && !request_id.has_value() && !message.called_by_old_CSPro_object )
            return ReturnProgrammingError(nullptr);

        if( async_call )
        {
            if( !message.called_by_old_CSPro_object )
            {
                return std::make_shared<std::wstring>(std::wstring(_T("CSProActionInvoker.$Impl.processAsyncResponse(")) +
                                                      CS2WS(IntToString(*request_id)) + _T(",") + Encoders::ToJsonString(json_response.GetResponseText()) + _T(");"));
            }

            else
            {
#ifdef WIN_DESKTOP
                return std::make_shared<std::wstring>(std::wstring(_T("CSPro.$processPostedMessageResponse(")) +
                                                      CS2WS(IntToString(message_id)) + _T(",") + Encoders::ToJsonString(json_response.GetResponseText()) + _T(");"));
#else
                return json_response.GetSharedResponseText();
#endif
            }
        }

        else
        {
            return json_response.GetSharedResponseText();
        }
    };

    try
    {
        auto json_node = std::make_shared<JsonNode<wchar_t>>(Json::Parse(message.message));

        if( async_call && !message.called_by_old_CSPro_object )
            request_id = json_node->Get<int>(JK::requestId);

        const Action action = static_cast<Action>(json_node->Get<int>(JK::action));
        const std::optional<std::wstring> access_token = json_node->GetOptional<std::wstring>(JK::accessToken);
        const std::optional<std::wstring> json_arguments = json_node->GetOptional<std::wstring>(JK::arguments);

        if( m_actionInvokerRuntime == nullptr )
        {
            m_actionInvokerRuntime = ObjectTransporter::GetActionInvokerRuntime();
            ASSERT(m_actionInvokerRuntime != nullptr);

            if( m_actionInvokerListenerHolder == nullptr )
            {
                // this should only occur when processing the first action coming from the question text window (on Windows)
                ASSERT(m_actionInvokerWebListener != nullptr);
                m_actionInvokerListenerHolder = ListenerHolder::Register(m_actionInvokerWebListener);
            }
        }

        // try to acquire the lock for 200 milliseconds
        constexpr size_t TryLockMilliseconds = 200;
        constexpr std::chrono::milliseconds TryLockSleepMilliseconds = std::chrono::milliseconds(50);
        size_t lock_try_count = TryLockMilliseconds / static_cast<size_t>(TryLockSleepMilliseconds.count());

        std::unique_lock<std::mutex> process_action_lock(m_processActionMutex, std::defer_lock);

        while( !process_action_lock.try_lock() && !m_webControllerShutdownFlag )
        {
            if( --lock_try_count == 0 )
            {
                ASSERT(!async_call);

                throw CSProException("An asynchronous JavaScript to the Action Invoker is still in progress. You can only execution actions on one thread, "
                                     "so synchronous calls will fail when calls to the Action Invoker are still in progress.");
            }

            std::this_thread::sleep_for(TryLockSleepMilliseconds);
        }

        if( m_webControllerShutdownFlag )
            return nullptr;

        // the question text web view will register a listener prior to the processing of the message
        std::unique_ptr<ListenerHolder> listener_holder;

        if( m_preProcessMessageWorker != nullptr )
        {
            std::shared_ptr<Listener> listener = m_preProcessMessageWorker->GetListener();
            ASSERT(listener != nullptr);
            listener_holder = ListenerHolder::Register(std::move(listener));
        }

        // if using the access token for the old CSPro JavaScript interface, allow the request;
        // otherwise make sure the access token is valid
        if( access_token != OldCSProJavaScriptInterface::GetAccessToken() )
            m_actionInvokerRuntime->CheckAccessToken(access_token.has_value() ? &*access_token : nullptr, m_actionInvokerCaller);

        // if applicable, mark that a synchronous message is being processed
        std::unique_ptr<WebViewSyncOperationMarker> web_view_sync_operation_marker = async_call ? nullptr :
                                                                                                  WebViewSyncOperationMarker::MarkInProgress();

        // process the action
        m_actionInvokerCaller.SetCancelFlag(false);
        m_actionInvokerCaller.SetCurrentMessageJsonNode(json_node);

        ActionInvoker::Result result = m_actionInvokerRuntime->ProcessAction(action, json_arguments, m_actionInvokerCaller);

        web_view_sync_operation_marker.reset();

        process_action_lock.unlock();

        return generate_response(JsonResponse(result));
    }

    catch( const CSProException& exception )
    {
        return generate_response(JsonResponse(exception));
    }
}
