#include "StdAfx.h"
#include "HtmlOutputWnd.h"


BEGIN_MESSAGE_MAP(HtmlOutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


HtmlOutputWnd::HtmlOutputWnd()
    :   m_htmlViewerView(new HtmlViewerView)
{
}


int HtmlOutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if( __super::OnCreate(lpCreateStruct) == -1 ||
        !m_htmlViewerView->Create(nullptr, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(), this, IDC_HTML_VIEWER) )
    {
        return -1;
    }

	return 0;
}


void HtmlOutputWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

    m_htmlViewerView->SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}
