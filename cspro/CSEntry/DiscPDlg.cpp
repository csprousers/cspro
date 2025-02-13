// DiscPDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "DiscPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscPDlg dialog

CDiscPDlg::CDiscPDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDiscPDlg::IDD, pParent)
{
    m_eAppMode = ADD_MODE;

    //{{AFX_DATA_INIT(CDiscPDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CDiscPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDiscPDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscPDlg, CDialog)
    //{{AFX_MSG_MAP(CDiscPDlg)
    ON_BN_CLICKED(IDC_SAVEPARTIAL, OnPartialSave)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscPDlg message handlers
BOOL CDiscPDlg::OnInitDialog()
{
    m_eMode = CANCEL_MODE;
    CDialog::OnInitDialog();
    if(m_eAppMode == VERIFY_MODE) {
        SetWindowText(_T("Stop Verifying"));
    }
    if(!m_bEnablePartialButton){
        CWnd* pWnd = GetDlgItem(IDC_SAVEPARTIAL);
        pWnd->EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDiscPDlg::OnPartialSave()
{
    m_eMode = PARTIAL_SAVE;
    CDialog::OnOK();
}

void CDiscPDlg::OnOK()
{
    m_eMode = DISCARD_CASE;
    CDialog::OnOK();
}
