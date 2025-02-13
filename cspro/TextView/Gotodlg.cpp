//***************************************************************************
//  File name: GotoDlg.cpp
//
//  Description:
//       Implementation of CGotoDialog class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//
//***************************************************************************

#include "StdAfx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog


CGotoDialog::CGotoDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CGotoDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGotoDialog)
    m_LineNumber = 0;
    //}}AFX_DATA_INIT
}

void CGotoDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGotoDialog)
    DDX_Text(pDX, IDC_GOTO_LINE_NUMBER, m_LineNumber);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGotoDialog, CDialog)
    //{{AFX_MSG_MAP(CGotoDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGotoDialog message handlers

//void CGotoDialog::OnSize(UINT nType, int cx, int cy)
//{
//    CRect rcViewClient, rcDlgRect;
//    CPoint ptDlgOrigin;
//
//    // position the dialog box so that it is displayed in the lower right hand corner of the main client rect...
//    GetWindowRect ( &rcDlgRect);                 // position of the dialog, relative to client view
//    m_pCurrView->GetClientRect (&rcViewClient);        // right,bottom hold size of client view
//    ptDlgOrigin = CPoint (rcViewClient.right-rcDlgRect.Size().cx, rcViewClient.bottom-rcDlgRect.Size().cy);    // left, top of dialog, relative to client view
//    m_pCurrView->ClientToScreen ( &ptDlgOrigin );      // convert to absolute (screen) coordinates
//    MoveWindow ( ptDlgOrigin.x, ptDlgOrigin.y, rcDlgRect.Size().cx, rcDlgRect.Size().cy);  // x,y,width,height
//
//    CDialog::OnSize(nType, cx, cy);
//}

BOOL CGotoDialog::PreTranslateMessage(MSG* pMsg)
{
    // CG: The following block was added by the ToolTips component.
    {
        // Let the ToolTip process this message.
        m_tooltip.RelayEvent(pMsg);

        return CDialog::PreTranslateMessage(pMsg);
    }
}

BOOL CGotoDialog::OnInitDialog()
{
    // CG: The following block was added by the ToolTips component.
    CDialog::OnInitDialog();

    // Create the ToolTip control.
    m_tooltip.Create(this);
    m_tooltip.Activate(TRUE);

    // TODO: Use one of the following forms to add controls:
    // m_tooltip.AddTool(GetDlgItem(IDC_<name>), <string-table-id>);
    // m_tooltip.AddTool(GetDlgItem(IDC_<name>), "<text>");

    CenterWindow();
    return TRUE;
}
