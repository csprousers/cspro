#include "StdAfx.h"
#include "ViewHtmlDlg.h"
#include <zHtml/UWM.h>
#include <zToolsO/ExceptionHolder.h>
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
    std::optional<bool> OnClose(CloseResult& close_result, ActionInvoker::Caller& caller) override;
    bool OnEngineProgramControlExecuted() override;

private:
    ViewHtmlDlg& m_dlg;
};


ViewHtmlDlgActionInvokerListener::ViewHtmlDlgActionInvokerListener(ViewHtmlDlg& dlg)
    :   m_dlg(dlg)
{
}


std::optional<bool> ViewHtmlDlgActionInvokerListener::OnClose(CloseResult& close_result, ActionInvoker::Caller& /*caller*/)
{
    if( std::holds_alternative<std::unique_ptr<const ActionInvoker::Exception>>(close_result) )
    {
        ASSERT(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result) != nullptr);

        ExceptionHolder* const exception_holder = m_dlg.m_viewer.GetData().exception_holder.get();

        if( exception_holder != nullptr )
            exception_holder->AddActionInvokerException(std::move(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result)));
    }

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
