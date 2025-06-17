// OPDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Opdlg.h"
#include "CSEntry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COPDlg dialog


COPDlg::COPDlg(CWnd* pParent /*=NULL*/)
    : CDialog(COPDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(COPDlg)
    m_sOpID = _T("");
    //}}AFX_DATA_INIT
}


void COPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COPDlg)
    DDX_Text(pDX, IDC_OPID, m_sOpID);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COPDlg, CDialog)
    //{{AFX_MSG_MAP(COPDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COPDlg message handlers

void COPDlg::OnOK()
{
    // TODO: Add extra validation here
    UpdateData(TRUE);
    if(m_sOpID.IsEmpty() || SO::IsBlank(m_sOpID)) {
        AfxMessageBox(_T("Operator ID cannot be blank"));
        GotoDlgCtrl(GetDlgItem(IDC_OPID));
        return;
    }
    CDialog::OnOK();
}
