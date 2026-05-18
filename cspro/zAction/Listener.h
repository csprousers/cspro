#pragma once

#include <zAction/ActionInvoker.h>
#include <zToolsO/ObjectTransporter.h>
#include <zHtml/WebViewPermission.h>

namespace ActionInvoker { class Exception; class Listener; class ListenerHolder; }


// --------------------------------------------------------------------------
// Listener
//
// Subclasses only need to override what they implement. The default
// implementations return a value meaning that the action was not processed.
// --------------------------------------------------------------------------

class ActionInvoker::Listener
{
public:
    virtual ~Listener() { }

    virtual SharableString OnGetDisplayOptions(Caller& caller);
    virtual std::optional<bool> OnSetDisplayOptions(const JsonNode& json_node, Caller& caller);

    virtual void OnSetWebViewOptions(const std::vector<WebViewPermission>* permissions);

    virtual SharableString OnGetInputData(Caller& caller, bool match_caller);

    using CloseResult = std::variant<
        std::monostate,                                 // close without a result
        const JsonNode,                                 // close with a result
        std::unique_ptr<const ActionInvoker::Exception> // close with a non-null exception;
    >;                                                  // if the caller handles the exception, it should set this value to null
    virtual std::optional<bool> OnClose(CloseResult& close_result, Caller& caller);

    virtual std::optional<int> OnGetAssociatedWebViewCallerId();
    virtual void OnPostWebMessage(const std::string& message, const std::optional<std::string>& target_origin);

    virtual void OnLogDebugMessage(const std::string& message);

    virtual bool OnEngineProgramControlExecuted();
};



// --------------------------------------------------------------------------
// ListenerHolder
//
// a RAII class for maintaining the lifecyle of the listeners
// --------------------------------------------------------------------------

class ActionInvoker::ListenerHolder
{
    friend class Runtime;

private:
    ListenerHolder(std::shared_ptr<std::vector<Listener*>> listeners, std::shared_ptr<Listener> listener);

public:
    ListenerHolder(const ListenerHolder&) = delete;
    ListenerHolder(ListenerHolder&& rhs) = default;
    ~ListenerHolder();

    // a convenience method to register a listener if the Action Invoker is available, returning null if not
    [[nodiscard]] static std::unique_ptr<ListenerHolder> Register(std::shared_ptr<Listener> listener);

    // a convenience method to create a listener if the Action Invoker is available, returning null if not
    template<typename T, typename... Args>
    [[nodiscard]] static std::unique_ptr<ListenerHolder> Create(Args&&... args);

private:
    std::shared_ptr<std::vector<Listener*>> m_listeners;
    std::shared_ptr<Listener> m_thisListener;
};



// --------------------------------------------------------------------------
// Listener: default implementations
// --------------------------------------------------------------------------

inline std::optional<bool> ActionInvoker::Listener::OnSetDisplayOptions(const JsonNode& /*json_node*/, Caller& /*caller*/)
{
    return std::nullopt;
}


inline SharableString ActionInvoker::Listener::OnGetDisplayOptions(Caller& /*caller*/)
{
    return SharableString();
}


inline void ActionInvoker::Listener::OnSetWebViewOptions(const std::vector<WebViewPermission>* /*permissions*/)
{
}


inline SharableString ActionInvoker::Listener::OnGetInputData(Caller& /*caller*/, bool /*match_caller*/)
{
    return SharableString();
}


inline std::optional<bool> ActionInvoker::Listener::OnClose(CloseResult& /*close_result*/, Caller& /*caller*/)
{
    return std::nullopt;
}


inline std::optional<int> ActionInvoker::Listener::OnGetAssociatedWebViewCallerId()
{
    return std::nullopt;
}


inline void ActionInvoker::Listener::OnPostWebMessage(const std::string& /*message*/, const std::optional<std::string>& /*target_origin*/)
{
    ASSERT(false);
}


inline void ActionInvoker::Listener::OnLogDebugMessage(const std::string& /*message*/)
{
    ASSERT(false);
}


inline bool ActionInvoker::Listener::OnEngineProgramControlExecuted()
{
    return false;
}



// --------------------------------------------------------------------------
// ListenerHolder
// --------------------------------------------------------------------------

inline ActionInvoker::ListenerHolder::ListenerHolder(std::shared_ptr<std::vector<Listener*>> listeners, std::shared_ptr<Listener> listener)
    :   m_listeners(std::move(listeners)),
        m_thisListener(std::move(listener))
{
    ASSERT(m_listeners != nullptr && m_thisListener != nullptr);
    ASSERT(std::find(m_listeners->cbegin(), m_listeners->cend(), m_thisListener.get()) == m_listeners->cend());

    m_listeners->emplace_back(m_thisListener.get());
}


inline ActionInvoker::ListenerHolder::~ListenerHolder()
{
    if( m_listeners == nullptr )
        return;

    const auto& lookup = std::find(m_listeners->cbegin(), m_listeners->cend(), m_thisListener.get());
    ASSERT(lookup != m_listeners->cend());

    m_listeners->erase(lookup);
}


inline std::unique_ptr<ActionInvoker::ListenerHolder> ActionInvoker::ListenerHolder::Register(std::shared_ptr<Listener> listener)
{
    ASSERT(listener != nullptr);

    std::shared_ptr<Runtime> action_invoker_runtime;

    try
    {
        action_invoker_runtime = ObjectTransporter::GetActionInvokerRuntime();
    }

    catch(...)
    {
        // this should only occur when setting up the question text window's listener (on Windows)
        return nullptr;
    }

    return std::make_unique<ListenerHolder>(action_invoker_runtime->RegisterListener(std::move(listener)));
}


template<typename T, typename... Args>
std::unique_ptr<ActionInvoker::ListenerHolder> ActionInvoker::ListenerHolder::Create(Args&&... args)
{
    return Register(std::make_shared<T>(std::forward<Args>(args)...));
}
