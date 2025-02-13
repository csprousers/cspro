#include "StdAfx.h"
#include "ViewHtmlDlg.h"
#include <zHtml/UWM.h>
#include <zAction/Listener.h>
#include <zAction/WebController.h>


BEGIN_MESSAGE_MAP(ViewHtmlDlg, HtmlViewDlg)
    ON_MESSAGE(UWM::Html::CloseDialog, OnCloseDialog)
END_MESSAGE_MAP()


ViewHtmlDlg::ViewHtmlDlg(const Viewer& viewer, CWnd* pParent/* = nullptr*/)
    :   HtmlViewDlg(pParent),
        m_viewer(viewer)
{
    // set up the Action Invoker
    SetUpActionInvoker();

    // set the viewer options 
    SetViewerOptions(viewer.GetOptions());
}


ViewHtmlDlg::~ViewHtmlDlg()
{
}


LRESULT ViewHtmlDlg::OnCloseDialog(WPARAM wParam, LPARAM /*lParam*/)
{
    if( wParam == IDOK )
    {
        OnOK();
    }

    else
    {
        ASSERT(wParam == IDCANCEL);
        OnCancel();
    }

    return 0;
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class ViewHtmlDlgActionInvokerListener : public ActionInvoker::Listener
{
public:
    ViewHtmlDlgActionInvokerListener(ViewHtmlDlg& dlg);

    // Listener overrides
    std::optional<bool> OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& caller) override;
    bool OnEngineProgramControlExecuted() override;

private:
    ViewHtmlDlg& m_dlg;
};


ViewHtmlDlgActionInvokerListener::ViewHtmlDlgActionInvokerListener(ViewHtmlDlg& dlg)
    :   m_dlg(dlg)
{
}


std::optional<bool> ViewHtmlDlgActionInvokerListener::OnCloseDialog(const JsonNode<wchar_t>& /*result_node*/, ActionInvoker::Caller& /*caller*/)
{
    m_dlg.PostMessage(UWM::Html::CloseDialog, IDOK);
    return true;
}


bool ViewHtmlDlgActionInvokerListener::OnEngineProgramControlExecuted()
{
    m_dlg.PostMessage(UWM::Html::CloseDialog, IDCANCEL);
    return true;
}


void ViewHtmlDlg::SetUpActionInvoker()
{
    ActionInvoker::WebController& web_controller = m_htmlViewCtrl.RegisterCSProHostObject();

    if( m_viewer.GetData().action_invoker_access_token_override != nullptr )
        web_controller.GetCaller().AddAccessTokenOverride(*m_viewer.GetData().action_invoker_access_token_override);

    m_actionInvokerListenerHolder = ActionInvoker::ListenerHolder::Create<ViewHtmlDlgActionInvokerListener>(*this);
}
