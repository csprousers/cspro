#include "stdafx.h"
#include "HtmlViewerView.h"


IMPLEMENT_DYNCREATE(HtmlViewerView, CView)

BEGIN_MESSAGE_MAP(HtmlViewerView, CView)
    ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


HtmlViewerView::HtmlViewerView()
    :   m_htmlViewCtrl(false)
{
    m_htmlViewCtrl.UseWebView2AcceleratorKeyHandler();
}


int HtmlViewerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if( __super::OnCreate(lpCreateStruct) == -1 ||
        !m_htmlViewCtrl.Create(nullptr, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(), this, IDC_HTML_VIEWER_CTRL) )
    {
        return -1;
    }

    return 0;
}


void HtmlViewerView::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

    // the WebView2 control doesn't seem to fully respond to Dynamic Layout settings
    // so size the control manually
    m_htmlViewCtrl.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


void HtmlViewerView::OnDraw(CDC* /*pDC*/)
{
}
