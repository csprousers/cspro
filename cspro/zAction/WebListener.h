#pragma once

#include <zAction/Listener.h>

#ifdef WIN_DESKTOP
#include <zHtml/HtmlViewCtrl.h>
#endif

namespace ActionInvoker { class WebListener; }


// --------------------------------------------------------------------------
// WebListener
// a listener owned by WebController
// --------------------------------------------------------------------------

class ActionInvoker::WebListener : public Listener
{
public:
    WebListener(int caller_id, void* web_view_tag);

    void SetOnGetInputDataCallback(std::function<SharableString()> on_get_input_data_callback);

    // Listener overrides
    SharableString OnGetInputData(Caller& caller, bool match_caller) override;

    std::optional<int> OnGetAssociatedWebViewCallerId() override;
    void OnPostWebMessage(const std::string& message, const std::optional<std::string>& target_origin) override;

protected:
#ifdef WIN_DESKTOP
    HtmlViewCtrl& GetHtmlViewCtrl() { return *static_cast<HtmlViewCtrl*>(m_webViewTag); }
#endif

private:
    int m_callerId;
    void* m_webViewTag;
    std::unique_ptr<std::function<SharableString()>> m_onGetInputDataCallback;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ActionInvoker::WebListener::WebListener(const int caller_id, void* const web_view_tag)
    :   m_callerId(caller_id),
        m_webViewTag(web_view_tag)
{
}


inline void ActionInvoker::WebListener::SetOnGetInputDataCallback(std::function<SharableString()> on_get_input_data_callback)
{
    m_onGetInputDataCallback = std::make_unique<std::function<SharableString()>>(std::move(on_get_input_data_callback));
}


inline SharableString ActionInvoker::WebListener::OnGetInputData(Caller& caller, const bool match_caller)
{
    if( m_onGetInputDataCallback == nullptr )
    {
        return Listener::OnGetInputData(caller, match_caller);
    }

    else if( !match_caller || m_callerId == caller.GetCallerId() )
    {
        return (*m_onGetInputDataCallback)();
    }

    else
    {
        return SharableString();
    }
}


inline std::optional<int> ActionInvoker::WebListener::OnGetAssociatedWebViewCallerId()
{
    return m_callerId;
}


#ifdef WIN_DESKTOP

// the Android version is defined in CSEntryDroid/.../ActionInvoker.cpp

inline void ActionInvoker::WebListener::OnPostWebMessage(const std::string& message, const std::optional<std::string>& /*target_origin*/)
{
    GetHtmlViewCtrl().PostWebMessageAsString(message);
}

#endif
