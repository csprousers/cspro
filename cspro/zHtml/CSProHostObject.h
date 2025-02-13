#pragma once

#include <zAction/WebController.h>

class HtmlViewCtrl;


// --------------------------------------------------------------------------
// CSProHostObject
// --------------------------------------------------------------------------

class CSProHostObject : public CCmdTarget
{
public:
    CSProHostObject(HtmlViewCtrl* source_window);
    ~CSProHostObject();

    ActionInvoker::WebController& GetActionInvokerWebController() { return m_actionInvokerWebController; }

    // the CSPro JavaScript class that wraps the host object's methods
    static std::wstring GetJavaScriptClassText();

protected:
    DECLARE_DISPATCH_MAP()

    void ActionInvokerSyncSetMessageSync(BSTR message);
    BSTR ActionInvokerSyncGetProcessedMessage();
    void ActionInvokerAsyncSetMessage(BSTR message);
    BSTR ActionInvokerAsyncGetUnused();
    void ActionInvokerOldCSProClassSetMessage(BSTR message);
    BSTR ActionInvokerOldCSProClassGetRequestId();

private:
    HtmlViewCtrl* m_sourceWindow;

    ActionInvoker::WebController m_actionInvokerWebController;
    int m_actionInvokerSyncMessageId;
    int m_actionInvokerOldCSProAsyncMessageId;
};
