#include "StdAfx.h"
#include "StartPositionDlg.h"


StartPositionDlg::StartPositionDlg(int start, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_START_POSITION, pParent),
        m_start(start)
{
}


void StartPositionDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_START_POSITION, m_start);
    DDV_MinMaxInt(pDX, m_start, 0, MAX_ITEM_START);
}
