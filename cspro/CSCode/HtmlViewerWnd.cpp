#include "StdAfx.h"
#include "HtmlViewerWnd.h"
#include <zToolsO/ExceptionHolder.h>
#include <zAction/WebController.h>


BEGIN_MESSAGE_MAP(HtmlViewerWnd, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()


HtmlViewerWnd::HtmlViewerWnd()
    :   m_htmlBrowserView(new HtmlBrowserView())
{
    // set up the Action Invoker
    SetUpActionInvoker();
}


int HtmlViewerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    if( !m_htmlBrowserView->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, this, IDC_HTML_VIEWER) )
        return -1;

    return 0;
}


void HtmlViewerWnd::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    m_htmlBrowserView->SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class HtmlViewerActionInvokerListener : public ActionInvoker::Listener
{
public:
    HtmlViewerActionInvokerListener(HtmlViewerWnd& html_viewer_wnd, ActionInvoker::Caller& caller);

    // Listener overrides
    std::optional<bool> OnClose(CloseResult& close_result, ActionInvoker::Caller& caller) override;

private:
    HtmlViewerWnd& m_htmlViewerWnd;
    ActionInvoker::Caller& m_actionInvokerCaller;
};


HtmlViewerActionInvokerListener::HtmlViewerActionInvokerListener(HtmlViewerWnd& html_viewer_wnd, ActionInvoker::Caller& caller)
    :   m_htmlViewerWnd(html_viewer_wnd),
        m_actionInvokerCaller(caller)
{
}


std::optional<bool> HtmlViewerActionInvokerListener::OnClose(CloseResult& close_result, ActionInvoker::Caller& /*caller*/)
{
    if( std::holds_alternative<std::unique_ptr<const ActionInvoker::Exception>>(close_result) )
    {
        ASSERT(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result) != nullptr);
        auto action_invoker_exception = std::move(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result));
        ErrorMessage::PostMessageForDisplay(ExceptionHolder::GetMessageToDisplay(*action_invoker_exception));
    }

    AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_WINDOW_HTML_VIEWER);

    return true;
}


void HtmlViewerWnd::SetUpActionInvoker()
{
    ActionInvoker::WebController& web_controller = GetHtmlViewCtrl().RegisterCSProHostObject();

    m_actionInvokerListenerHolder = ActionInvoker::ListenerHolder::Create<HtmlViewerActionInvokerListener>(*this, web_controller.GetCaller());
}
