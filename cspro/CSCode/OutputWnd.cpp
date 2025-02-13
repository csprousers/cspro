#include "StdAfx.h"
#include "OutputWnd.h"


BEGIN_MESSAGE_MAP(OutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int OutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    if( !m_loggingListBox.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
                                 CRect(), this, IDC_OUTPUT_LIST_BOX) )
    {
        return -1;
    }

	return 0;
}


void OutputWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	m_loggingListBox.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}
