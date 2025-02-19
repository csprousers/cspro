#pragma once

#include <zAction/ExternalCaller.h>
#include <zHtml/LocalhostUrl.h>

namespace ActionInvoker { class WebCaller; }


class ActionInvoker::WebCaller : public ActionInvoker::ExternalCaller
{
public:
    using ExternalCaller::ExternalCaller;

    // used by GetRootDirectory
    void SetCurrentMessageJsonNode(std::shared_ptr<const JsonNode> json_node)
    {
        m_currentMessageJsonNode = std::move(json_node);
    }

    // Caller overrides
    CancelFlag& GetCancelFlag() override
    {
        return m_cancelFlag;
    }

    std::string GetRootDirectory() override
    {
        if( m_currentMessageJsonNode != nullptr && m_currentMessageJsonNode->Contains(JK::url) )
            return LocalhostUrl::GetDirectoryFromUrl(m_currentMessageJsonNode->Get<std::string>(JK::url));

        return std::string();
    }

    bool IsWebView() const override
    {
        return true;
    }

private:
    CancelFlag m_cancelFlag;
    std::shared_ptr<const JsonNode> m_currentMessageJsonNode;
};
