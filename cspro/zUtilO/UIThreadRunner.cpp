#include "StdAfx.h"
#include "UIThreadRunner.h"
#include <mutex>


class AsyncUIThreadRunner : public UIThreadRunner
{
public:
    void PostForRunning(std::function<void()> callback_function);

    void Execute(LPARAM lParam) override;

private:
    std::vector<std::unique_ptr<std::function<void()>>> m_callbackFunctions;
    std::mutex m_mutex;
};


void AsyncUIThreadRunner::PostForRunning(std::function<void()> callback_function)
{
    ASSERT(callback_function);

    const std::lock_guard<std::mutex> lock(m_mutex);

    // try to reuse a slot
    size_t cache_key = 0;

    for( auto& cf : m_callbackFunctions )
    {
        if( cf == nullptr )
            break;

        ++cache_key;
    }

    if( cache_key == m_callbackFunctions.size() )
        m_callbackFunctions.emplace_back();

    m_callbackFunctions[cache_key] = std::make_unique<std::function<void()>>(std::move(callback_function));

    WindowsDesktopMessage::Post(UWM::UtilO::RunOnUIThread, this, cache_key);
}


void AsyncUIThreadRunner::Execute(const LPARAM lParam)
{
    std::unique_ptr<std::function<void()>> callback_function;

    // lock
    {
        const std::lock_guard<std::mutex> lock(m_mutex);

        if( static_cast<size_t>(lParam) < m_callbackFunctions.size() )
            callback_function = std::move(m_callbackFunctions[lParam]);
    }

    if( callback_function == nullptr )
    {
        ASSERT(false);
    }

    else
    {
        (*callback_function)();
    }
}


void RunOnUIThreadAsync(std::function<void()> callback_function)
{
    static AsyncUIThreadRunner async_ui_thread_runner;
    async_ui_thread_runner.PostForRunning(std::move(callback_function));
}
