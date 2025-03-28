#include "StdAfx.h"
#include "ViewView.h"
#include <zToolsO/ExceptionHolder.h>


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
    try
    {
        m_htmlViewCtrl.NavigateTo(view_doc.GetUrl());
    }

    catch( const CSProException& exception )
    {
        m_htmlViewCtrl.SetHtml(Encoders::ToPreformattedTextHtml("CSView Error", exception.what()));
    }
}



// --------------------------------------------------------------------------
// Action Invoker functionality
// --------------------------------------------------------------------------

class ViewViewActionInvokerListener : public ActionInvoker::Listener
{
public:
    // Listener overrides
    std::optional<bool> OnClose(CloseResult& close_result, ActionInvoker::Caller& caller) override;
    bool OnEngineProgramControlExecuted() override;

private:
    void CloseDocument();
};


std::optional<bool> ViewViewActionInvokerListener::OnClose(CloseResult& close_result, ActionInvoker::Caller& /*caller*/)
{
    // display any pending exceptions
    if( std::holds_alternative<std::unique_ptr<const ActionInvoker::Exception>>(close_result) )
    {
        ASSERT(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result) != nullptr);
        auto action_invoker_exception = std::move(std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result));
        ErrorMessage::PostMessageForDisplay(ExceptionHolder::GetMessageToDisplay(*action_invoker_exception), false);
    }

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
    AfxGetMainWnd()->PostMessage(UWM::CSView::CloseDocument);
}


void ViewView::SetUpActionInvoker()
{
    if( m_actionInvokerListenerHolder != nullptr )
        return;

    m_htmlViewCtrl.RegisterCSProHostObject();

    m_actionInvokerListenerHolder = ActionInvoker::ListenerHolder::Create<ViewViewActionInvokerListener>();
}
