// DiscDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "DiscDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscDlg dialog

CDiscDlg::CDiscDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDiscDlg::IDD, pParent)
{
    m_eAppMode = ADD_MODE;
    //{{AFX_DATA_INIT(CDiscDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CDiscDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDiscDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CDiscDlg message handlers

BOOL CDiscDlg::OnInitDialog()
{
    m_eMode =CANCEL_MODE;

    CDialog::OnInitDialog();

    if(m_eAppMode == VERIFY_MODE) {
        SetWindowText(_T("Stop Verifying"));
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDiscDlg::OnOK()
{
    m_eMode =DISCARD_CASE;
    CDialog::OnOK();
}
