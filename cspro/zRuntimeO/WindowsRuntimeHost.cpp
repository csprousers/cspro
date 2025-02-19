#include "StdAfx.h"
#include "WindowsRuntimeHost.h"
#include "ErrorDisplayRuntime.h"
#include "WindowsRuntimeView.h"
#include <zJson/ValidJsonAsserter.h>


WindowsRuntimeHost::WindowsRuntimeHost(WindowsRuntimeView& runtime_view)
    :   m_runtimeView(runtime_view),
        m_uiThreadActionsCounter(0)
{
}


WindowsRuntimeHost::~WindowsRuntimeHost()
{
    if( m_messageThread != nullptr && m_messageThread->joinable() )
        m_messageThread->join();
}


template<typename... Args>
void WindowsRuntimeHost::PostActionToRunOnUiThread(Args&&... args)
{
    const int ui_thread_action_id = ++m_uiThreadActionsCounter;

    // lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_uiThreadActions.emplace_back(ui_thread_action_id, std::make_unique<std::function<void()>>(std::forward<Args>(args)...));
    }

    m_runtimeView.PostMessage(UWM::Runtime::RunUiThreadAction, ui_thread_action_id);
}


void WindowsRuntimeHost::RunUiThreadAction(const int ui_thread_action_id)
{
    std::unique_ptr<std::function<void()>> action;

    // lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto lookup = std::find_if(m_uiThreadActions.begin(), m_uiThreadActions.end(),
                                   [&](const auto& id_and_action) { return ( std::get<0>(id_and_action) == ui_thread_action_id ); });

        if( lookup == m_uiThreadActions.end() )
        {
            ASSERT(false);
            return;
        }

        action = std::move(std::get<1>(*lookup));
        m_uiThreadActions.erase(lookup);
    }

    ASSERT(action != nullptr);

    (*action)();
}


template<typename CF>
void WindowsRuntimeHost::ForeachRuntimeEntry(const CF& callback_function)
{
    if( m_runtimeStack.empty() )
        return;

    const auto& runtime_stack_rend = m_runtimeStack.rend();

    for( auto runtime_stack_itr = m_runtimeStack.rbegin();
         runtime_stack_itr != runtime_stack_rend;
         ++runtime_stack_itr )
    {
        if( !CallbackFunctionProcessor::KeepProcessing(callback_function, *runtime_stack_itr) )
            return;
    }
}


Runtime& WindowsRuntimeHost::FindRuntimeByAccessToken(const std::string& access_token)
{
    Runtime* runtime = nullptr;

    ForeachRuntimeEntry(
        [&](RuntimeEntry& this_runtime_entry)
        {
            if( this_runtime_entry.access_token == access_token )
            {
                runtime = this_runtime_entry.runtime.get();
                return false;
            }

            return true;
        });

    if( runtime != nullptr )
        return *runtime;

    throw CSProException("No runtime is associated with the access token '%s'", access_token.c_str());
}


WindowsRuntimeHost::RuntimeEntry* WindowsRuntimeHost::FindRuntimeEntryByUrl(const std::string& url) noexcept
{
    RuntimeEntry* runtime_entry = nullptr;

    ForeachRuntimeEntry(
        [&](RuntimeEntry& this_runtime_entry)
        {
            if( *this_runtime_entry.url == url )
            {
                runtime_entry = &this_runtime_entry;
                return false;
            }

            return true;
        });

    return runtime_entry;
}


std::unique_ptr<Runtime> WindowsRuntimeHost::CreateRuntimeForApplication(const std::string& file_path) noexcept
{
    try
    {
        throw CSProException("RT_TODO ... create runtime for: " + file_path);
    }

    catch( const CSProException& exception )
    {
        return std::make_unique<ErrorDisplayRuntime>(exception.what());
    }
}


void WindowsRuntimeHost::StartRuntimeAsync(std::shared_ptr<Runtime> runtime) noexcept
{
    PostActionToRunOnUiThread(
        [this, runtime_ = std::move(runtime)]() mutable
        {
            StartRuntime(std::move(runtime_));
        });
}


WindowsRuntimeHost::RuntimeEntry::RuntimeEntry(std::shared_ptr<Runtime> runtime_)
    :   runtime(std::move(runtime_)),
        access_token(CreateUuid()),
        url(runtime->GetUrl())
{
    ASSERT(runtime != nullptr);
}


void WindowsRuntimeHost::StartRuntime(std::shared_ptr<Runtime> runtime) noexcept
{
    ASSERT(runtime != nullptr);    

    // if a runtime is active...
    if( !m_runtimeStack.empty() )
    {
        Runtime& current_runtime = *m_runtimeStack.back().runtime;

        // ...and is not suspendable, then we need to hold this runtime for later
        if( !current_runtime.IsSuspendable() )
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingRuntimes.emplace_back(std::move(runtime));
            return;
        }

        // otherwise suspend the runtime
        current_runtime.OnSuspend();        
    }

    // the runtime starting code is in a loop so that, on exception, the loop runs again with runtime as ErrorDisplayRuntime
    for( int i = 0; i < 2; ++i )
    {
        try
        {
            ASSERT(runtime != nullptr);

            runtime->SetRuntimeHost(this);

            ASSERT(FindRuntimeEntryByUrl(*runtime->GetUrl()) == nullptr);

            Runtime* this_runtime;            

            // lock
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                this_runtime = m_runtimeStack.emplace_back(std::move(runtime)).runtime.get();                
            }

            this_runtime->Start();

            ActivateRuntime();

            return;
        }

        catch( const CSProException& exception )
        {
            // remove the runtime from the stack if there were errors starting it
            m_runtimeStack.pop_back();

            // display the error as a runtime
            runtime = std::make_unique<ErrorDisplayRuntime>(exception.what());
        }
    }
}


void WindowsRuntimeHost::ActivateRuntime() noexcept
{
    ASSERT(!m_runtimeStack.empty());
    Runtime& current_runtime = *m_runtimeStack.back().runtime;

    // update the title
    m_runtimeView.SetRuntimeTitle(current_runtime.GetDescription());

    // activate the runtime
    current_runtime.OnActivate();
}


void WindowsRuntimeHost::ActivateTopmostRuntimeIfNecessary()
{
    ASSERT(!m_runtimeStack.empty());
    Runtime& current_runtime = *m_runtimeStack.back().runtime;

    if( m_lastSourceChangedUrl != *current_runtime.GetUrl() )
        ActivateRuntime();   
}


void WindowsRuntimeHost::CloseRuntimeAsync() noexcept
{
    ASSERT(!m_runtimeStack.empty());

    PostActionToRunOnUiThread(
        [this]()
        {
            CloseRuntimes(false);
        });
}


size_t WindowsRuntimeHost::CloseRuntimes(const bool close_all_runtimes)
{
    bool runtime_closed = false;

    while( !m_runtimeStack.empty() )
    {
        Runtime& current_runtime = *m_runtimeStack.back().runtime;

        // if the current runtime is not in a closeable state, send a notification to close it
        if( !current_runtime.IsCloseable() )
        {
            ActivateTopmostRuntimeIfNecessary();
            current_runtime.OnClose();
            break;
        }

        // remove this runtime
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_runtimeStack.pop_back();
        }

        runtime_closed = true;

        if( !close_all_runtimes )
            break;
    }

    // if a runtime was closed, or no runtimes remain, and a pending runtime exists, start it
    if( ( runtime_closed || m_runtimeStack.empty() ) && !m_pendingRuntimes.empty() )
    {
        std::shared_ptr<Runtime> pending_runtime;

        // lock
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            pending_runtime = std::move(m_pendingRuntimes.front());
            m_pendingRuntimes.erase(m_pendingRuntimes.begin());
        }

        StartRuntimeAsync(std::move(pending_runtime));

        return m_runtimeStack.size() + 1;
    }

    // if a runtime remains, activate it
    if( !m_runtimeStack.empty() )
    {
        ActivateTopmostRuntimeIfNecessary();
    }

    else if( m_runtimeStack.empty() )
    {
        m_runtimeView.PostMessage(UWM::Runtime::AllRuntimesClosed);
    }

    return m_runtimeStack.size();
}


void WindowsRuntimeHost::NavigateToAsync(const SharableString& url) noexcept
{
    PostActionToRunOnUiThread(
        [this, url_ = url]()
        {
            m_runtimeView.GetHtmlViewCtrl().NavigateTo(*url_);
        });
}


void WindowsRuntimeHost::PostWebMessageAsync(const SharableString& message) noexcept
{
    AssertValidJson(*message);

    PostActionToRunOnUiThread(
        [this, message_ = message]()
        {
            m_runtimeView.GetHtmlViewCtrl().PostWebMessageAsString(*message_);
        });    
}


void WindowsRuntimeHost::OnNavigationStarting(bool& cancel_navigation, const std::string& url)
{
    if( !m_runtimeStack.empty() )
    {
        RuntimeEntry& current_runtime_entry = m_runtimeStack.back();

        if( *current_runtime_entry.url != url &&
            !current_runtime_entry.runtime->IsSuspendable() )
        {
            cancel_navigation = true;
        }
    }
}


void WindowsRuntimeHost::OnSourceChanged(std::string url)
{
    m_lastSourceChangedUrl = std::move(url);
}


void WindowsRuntimeHost::OnNavigationCompleted()
{
    // if navigating to a runtime, activate it
    RuntimeEntry* const runtime_entry_to_activate = FindRuntimeEntryByUrl(m_lastSourceChangedUrl);

    if( runtime_entry_to_activate != nullptr )
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

        json_writer->BeginObject()
                    .Write(JK::accessToken, runtime_entry_to_activate->access_token)
                    .EndObject();

        PostActionMessageAsync("CSProRT.activate", json_writer->GetString());
    }
}


void WindowsRuntimeHost::OnWebMessageReceived(std::string message)
{
    // lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingMessages.emplace_back(std::move(message));
    }

    m_runtimeView.PostMessage(UWM::Runtime::ProcessMessages);
}


void WindowsRuntimeHost::ProcessMessageQueue()
{
    if( m_messageThread != nullptr || m_pendingMessages.empty() )
        return;

    std::optional<std::string> message;

    // lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        message.emplace(std::move(m_pendingMessages.front()));
        m_pendingMessages.erase(m_pendingMessages.begin());
    }

    ASSERT(message.has_value());

    m_messageThread = std::make_unique<std::thread>(
        [this, message_ = std::move(*message)]()
        {
            ProcessMessage(message_);
        });
}


struct WindowsRuntimeHost::MessageResult
{
    std::optional<int> async_request_id;
    SharableString results;
    const char* result_type;
};


void WindowsRuntimeHost::ProcessMessage(const std::string& message)
{
    if( m_runtimeStack.empty() )
    {
        ASSERT(false);
        return;
    }

    MessageResult message_result;

    try
    {
        const JsonNode json_node = Json::Parse(message);

        Runtime& runtime = FindRuntimeByAccessToken(json_node.Get<std::string>(JK::accessToken));

        message_result.async_request_id = json_node.GetOptional<int>(JK::requestId);

        message_result.results = runtime.OnMessage(json_node.Get<std::string_view>(JK::action),
                                                   json_node.GetOrEmpty(JK::data));

        message_result.result_type = message_result.results.IsSet() ? "value" : "undefined";
    }

    catch( const CSProException& exception )
    {
        if( message_result.async_request_id.has_value() )
        {
            message_result.results = exception.what();
            message_result.result_type = "exception";
        }

        else
        {
            ErrorMessage::PostMessageForDisplay(exception);
        }
    }

    PostActionToRunOnUiThread(
        [this, message_result_ = std::move(message_result)]()
        {
            ProcessMessageResult(message_result_);
        });
}


void WindowsRuntimeHost::ProcessMessageResult(const MessageResult& message_result)
{
    ASSERT(m_messageThread != nullptr);

    if( m_messageThread->joinable() )
        m_messageThread->join();

    m_messageThread.reset();

    if( message_result.async_request_id.has_value() )
    {
        const std::string result_call = SO::Concatenate(FormatText("CSProRT.$Impl.processMessageResult(%d,\"%s\",",
                                                                   *message_result.async_request_id,
                                                                   message_result.result_type),
                                                        Encoders::ToJsonString(*message_result.results),
                                                        ");");

        m_runtimeView.GetHtmlViewCtrl().ExecuteScript(result_call);
    }

    // process any additional messages in the queue
    m_runtimeView.PostMessage(UWM::Runtime::ProcessMessages);
}
