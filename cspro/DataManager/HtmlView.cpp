#include "StdAfx.h"
#include "HtmlView.h"


IMPLEMENT_DYNCREATE(HtmlView, HtmlViewerView)


HtmlView::HtmlView()
{
    m_htmlViewCtrl.SetContextMenuEnabled(false);
    m_htmlViewCtrl.SetZoomControlEnabled(true);

    m_htmlViewCtrl.SetAcceleratorKeyHandler(
        [&](const UINT message, const UINT key, const INT lParam)
        {
            if( message == WM_KEYDOWN || message == WM_SYSKEYDOWN )
            {
                CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
                return main_frame->HandleAcceleratorKeyFromWebView2(GetCaseHoldingDoc(), key, lParam);
            }

            return false;
        });

    m_htmlViewCtrl.AddWebEventObserver(
        [&](const std::wstring_view message_sv)
        {
            WindowsDesktopMessage::PostObject(GetParentFrame(), UWM::DataManager::ProcessWebViewMessage, TC::ToUtf8(message_sv));
        });
}


void HtmlView::OnInitialUpdate()
{
    GetParentFrame()->PostMessage(UWM::DataManager::ShowDefaultPage);
}
