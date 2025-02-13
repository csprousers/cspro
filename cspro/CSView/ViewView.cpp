#include "StdAfx.h"
#include "ViewView.h"


IMPLEMENT_DYNCREATE(ViewView, HtmlViewerView)


ViewView::ViewView()
{
}


void ViewView::OnInitialUpdate()
{
    ViewDoc& view_doc = GetViewDoc();

    // set up the Action Invoker
    SetUpActionInvoker();

    // navigate to the current document
    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();
    const std::wstring document_url = view_doc.GetDocumentUrl(file_server);

    GetHtmlViewCtrl().NavigateTo(document_url);
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class ViewViewActionInvokerListener : public ActionInvoker::Listener
{
public:
    // Listener overrides
    std::optional<bool> OnCloseDialog(const JsonNode<wchar_t>& result_node, ActionInvoker::Caller& caller) override;
    bool OnEngineProgramControlExecuted() override;

private:
    void CloseDocument();
};


std::optional<bool> ViewViewActionInvokerListener::OnCloseDialog(const JsonNode<wchar_t>& /*result_node*/, ActionInvoker::Caller& /*caller*/)
{
    CloseDocument();
    return true;
}


bool ViewViewActionInvokerListener::OnEngineProgramControlExecuted()
{
    CloseDocument();
    return true;
}


void ViewViewActionInvokerListener::CloseDocument()
{
    AfxGetMainWnd()->PostMessage(WM_CLOSE);
}


void ViewView::SetUpActionInvoker()
{
    if( m_actionInvokerListenerHolder != nullptr )
        return;

    GetHtmlViewCtrl().RegisterCSProHostObject();

    m_actionInvokerListenerHolder = ActionInvoker::ListenerHolder::Create<ViewViewActionInvokerListener>();
}
