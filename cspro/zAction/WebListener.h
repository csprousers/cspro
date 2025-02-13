#pragma once

#include <zAction/Listener.h>

#ifdef WIN_DESKTOP
#include <zHtml/HtmlViewCtrl.h>
#endif


namespace ActionInvoker
{
    // --------------------------------------------------------------------------
    // WebListener
    // a listener owned by WebController
    // --------------------------------------------------------------------------

    class WebListener : public Listener
    {
    public:
        WebListener(Caller::WebViewTag web_view_tag);

        void SetOnGetInputDataCallback(std::function<std::wstring()> on_get_input_data_callback) { m_onGetInputDataCallback = std::make_unique<std::function<std::wstring()>>(std::move(on_get_input_data_callback)); }

        // Listener overrides
        std::optional<std::wstring> OnGetInputData(Caller& caller, bool match_caller) override;

        std::optional<ActionInvoker::Caller::WebViewTag> OnGetAssociatedWebViewDetails() override;
        void OnPostWebMessage(const std::wstring& message, const std::optional<std::wstring>& target_origin) override;

    protected:
#ifdef WIN_DESKTOP
        HtmlViewCtrl& GetHtmlViewCtrl() { return *static_cast<HtmlViewCtrl*>(m_webViewTag); }
#endif

    private:
        Caller::WebViewTag m_webViewTag;
        std::unique_ptr<std::function<std::wstring()>> m_onGetInputDataCallback;
    };
}


inline ActionInvoker::WebListener::WebListener(Caller::WebViewTag web_view_tag)
    :   m_webViewTag(web_view_tag)
{
}


inline std::optional<std::wstring> ActionInvoker::WebListener::OnGetInputData(Caller& caller, bool match_caller)
{
    if( m_onGetInputDataCallback == nullptr )
    {
        return Listener::OnGetInputData(caller, match_caller);
    }

    else if( !match_caller || caller.IsFromWebView(m_webViewTag) )
    {
        return (*m_onGetInputDataCallback)();
    }

    else
    {
        return std::nullopt;
    }
}


inline std::optional<ActionInvoker::Caller::WebViewTag> ActionInvoker::WebListener::OnGetAssociatedWebViewDetails()
{
    return m_webViewTag;
}


#ifdef WIN_DESKTOP

// the Android version is defined in CSEntryDroid/.../ActionInvoker.cpp

inline void ActionInvoker::WebListener::OnPostWebMessage(const std::wstring& message, const std::optional<std::wstring>& /*target_origin*/)
{
    GetHtmlViewCtrl().PostWebMessageAsString(message);
}

#endif
