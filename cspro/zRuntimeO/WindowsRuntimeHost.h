#pragma once

#include <zRuntimeO/zRuntimeO.h>
#include <zRuntimeO/RuntimeHost.h>
#include <mutex>

class WindowsRuntime;
class WindowsRuntimeView;


class ZRUNTIMEO_API WindowsRuntimeHost : public RuntimeHost
{
    friend class WindowsRuntimeView;

public:
    WindowsRuntimeHost(WindowsRuntimeView& runtime_view);
    ~WindowsRuntimeHost();

    // tries to close the current, or all, runtimes;
    // the number of runtimes active following the closure is returned
    size_t CloseRuntimes(bool close_all_runtimes);

    // RuntimeHost overrides
    void CloseRuntimeAsync() noexcept override;
    void NavigateToAsync(const SharableString& url) noexcept override;
    void PostWebMessageAsync(const SharableString& message) noexcept override;
    std::unique_ptr<Runtime> CreateRuntimeForApplication(const std::string& file_path) noexcept override;
    void StartRuntimeAsync(std::shared_ptr<Runtime> runtime) noexcept override;

private:
    // adds a callback function for WindowsRuntimeView to run on the UI thread
    template<typename... Args>
    void PostActionToRunOnUiThread(Args&&... args);

    // called by WindowsRuntimeView from the UI thread to run some action on the UI thread
    void RunUiThreadAction(int ui_thread_action_id);

    // iterates over each RuntimeEntry, from the top of the stack to the bottom
    template<typename CF>
    void ForeachRuntimeEntry(const CF& callback_function);

    // returns the runtime associated with this access token, throwing an exception if not found
    Runtime& FindRuntimeByAccessToken(const std::string& access_token);

    // returns the runtime entry associated with this URL, returning null if not foud
    struct RuntimeEntry;
    RuntimeEntry* FindRuntimeEntryByUrl(const std::string& url) noexcept;

    // starts the runtime 
    void StartRuntime(std::shared_ptr<Runtime> runtime) noexcept;

    // activates the topmost runtime
    void ActivateRuntime() noexcept;

    // activates the topmost runtime only if it is not already displaying
    void ActivateTopmostRuntimeIfNecessary();

    // called by WindowsRuntimeView (from the UI thread)
    void OnNavigationStarting(bool& cancel_navigation, const std::string& url);

    // called by WindowsRuntimeView (from the UI thread)
    void OnSourceChanged(std::string url);

    // called by WindowsRuntimeView (from the UI thread)
    void OnNavigationCompleted();

    // called by WindowsRuntimeView (from the UI thread)
    void OnWebMessageReceived(std::string message);

    // called by WindowsRuntimeView (from the UI thread)
    void ProcessMessageQueue();

    // processes a message (called in a background thread)
    void ProcessMessage(const std::string& message);

    // processes the message result (from the UI thread)
    struct MessageResult;
    void ProcessMessageResult(const MessageResult& message_result);

private:
    WindowsRuntimeView& m_runtimeView;
    std::string m_lastSourceChangedUrl;

    // a mutex for adding runtimes, UI thread actions, or messages
    std::mutex m_mutex;

    // the stack of runtimes
    struct RuntimeEntry
    {
        RuntimeEntry(std::shared_ptr<Runtime> runtime_);

        std::shared_ptr<Runtime> runtime;
        std::string access_token;
        SharableString url;
    };

    std::vector<RuntimeEntry> m_runtimeStack;
    std::vector<std::shared_ptr<Runtime>> m_pendingRuntimes;

    // actions that need to run on the UI thread
    int m_uiThreadActionsCounter;
    std::vector<std::tuple<int, std::unique_ptr<std::function<void()>>>> m_uiThreadActions;

    // message processing
    std::unique_ptr<std::thread> m_messageThread;
    std::vector<std::string> m_pendingMessages;
};
