//***************************************************************************
//  File name: SelDlg.cpp
//
//  Description:
//       Implementation of CSelectionDialog class
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
// CSelectionDialog dialog


CSelectionDialog::CSelectionDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CSelectionDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSelectionDialog)
    m_nAll = -1;
    //}}AFX_DATA_INIT
}

void CSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelectionDialog)
    DDX_Radio(pDX, IDC_ALL, m_nAll);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSelectionDialog, CDialog)
    //{{AFX_MSG_MAP(CSelectionDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog message handlers

BOOL CSelectionDialog::PreTranslateMessage(MSG* pMsg)
{
    // CG: The following block was added by the ToolTips component.
    {
        // Let the ToolTip process this message.
        m_tooltip.RelayEvent(pMsg);

        return CDialog::PreTranslateMessage(pMsg);
    }
}

BOOL CSelectionDialog::OnInitDialog()
{
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
