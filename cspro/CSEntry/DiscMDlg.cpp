// DiscMDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "DiscMDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscMDlg dialog

CDiscMDlg::CDiscMDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDiscMDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDiscMDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CDiscMDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDiscMDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscMDlg, CDialog)
    //{{AFX_MSG_MAP(CDiscMDlg)
    ON_BN_CLICKED(IDC_FINISH, OnFinish)
    ON_BN_CLICKED(IDOK, OnDiscard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscMDlg message handlers


BOOL CDiscMDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    m_eMode = CANCEL_MODE;

    if( !m_csFinishText.IsEmpty() )
        GetDlgItem(IDC_FINISH)->SetWindowText(m_csFinishText);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDiscMDlg::OnFinish()
{
    m_eMode = FINISH_CASE;
    CDialog::OnOK();
}

void CDiscMDlg::OnDiscard()
{
    m_eMode = DISCARD_CASE;
    CDialog::OnOK();
}

void CDiscMDlg::OnCancel()
{
    m_eMode = CANCEL_MODE;
    CDialog::OnCancel();
}
