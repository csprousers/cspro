#pragma once

#include <zAction/ActionInvoker.h>
#include <zToolsO/ObjectTransporter.h>


namespace ActionInvoker
{
    // subclasses only need to override what they implement;
    // the default implementations return a value meaning that the action was not processed
    class Listener
    {
    public:
        virtual ~Listener() { }

        virtual std::optional<std::wstring> OnGetDisplayOptions(Caller& caller);
        virtual std::optional<bool> OnSetDisplayOptions(const JsonNode<wchar_t>& json_node, Caller& caller);

        virtual std::optional<std::wstring> OnGetInputData(Caller& caller, bool match_caller);

        virtual std::optional<bool> OnCloseDialog(const JsonNode<wchar_t>& result_node, Caller& caller);

        virtual std::optional<Caller::WebViewTag> OnGetAssociatedWebViewDetails();
        virtual void OnPostWebMessage(const std::wstring& message, const std::optional<std::wstring>& target_origin);

        virtual bool OnEngineProgramControlExecuted();
    };


    // a RAII class for maintaining the lifecyle of the listeners
    class ListenerHolder
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
}



// --------------------------------------------------------------------------
// Listener: default implementations
// --------------------------------------------------------------------------

inline std::optional<bool> ActionInvoker::Listener::OnSetDisplayOptions(const JsonNode<wchar_t>& /*json_node*/, Caller& /*caller*/)
{
    return std::nullopt;
}


inline std::optional<std::wstring> ActionInvoker::Listener::OnGetDisplayOptions(Caller& /*caller*/)
{
    return std::nullopt;
}


inline std::optional<std::wstring> ActionInvoker::Listener::OnGetInputData(Caller& /*caller*/, bool /*match_caller*/)
{
    return std::nullopt;
}


inline std::optional<bool> ActionInvoker::Listener::OnCloseDialog(const JsonNode<wchar_t>& /*result_node*/, Caller& /*caller*/)
{
    return std::nullopt;
}


inline std::optional<ActionInvoker::Caller::WebViewTag> ActionInvoker::Listener::OnGetAssociatedWebViewDetails() 
{
    return std::nullopt;
}


inline void ActionInvoker::Listener::OnPostWebMessage(const std::wstring& /*message*/, const std::optional<std::wstring>& /*target_origin*/)
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
