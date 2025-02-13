#pragma once

#include <zAction/ExternalCaller.h>
#include <zHtml/LocalhostUrl.h>


namespace ActionInvoker
{
    class WebCaller : public ActionInvoker::ExternalCaller
    {
    public:
        WebCaller(WebViewTag web_view_tag)
            :   m_webViewTag(web_view_tag),
                m_cancelFlag(false)
        {
        }

        // used by GetRootDirectory
        void SetCurrentMessageJsonNode(std::shared_ptr<const JsonNode<wchar_t>> json_node)
        {
            m_currentMessageJsonNode = std::move(json_node);
        }

        // Caller overrides
        bool& GetCancelFlag() override
        {
            return m_cancelFlag;
        }

        std::wstring GetRootDirectory() override
        {
            if( m_currentMessageJsonNode != nullptr && m_currentMessageJsonNode->Contains(JK::url) )
                return LocalhostUrl::GetDirectoryFromUrl(m_currentMessageJsonNode->Get<std::wstring>(JK::url));

            return std::wstring();
        }

        std::optional<WebViewTag> GetWebViewTag() const override
        {
            return m_webViewTag;
        }

    private:
        WebViewTag m_webViewTag;
        bool m_cancelFlag;
        std::shared_ptr<const JsonNode<wchar_t>> m_currentMessageJsonNode;
    };
}
