// DisPMDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "DisPMDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisPMDlg dialog

CDisPMDlg::CDisPMDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDisPMDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDisPMDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CDisPMDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDisPMDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisPMDlg, CDialog)
    //{{AFX_MSG_MAP(CDisPMDlg)
    ON_BN_CLICKED(IDOK, OnPartialSave)
    ON_BN_CLICKED(IDC_FINISH, OnFinish)
    ON_BN_CLICKED(IDC_DICARD, OnDiscard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisPMDlg message handlers

BOOL CDisPMDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    m_eMode = CANCEL_MODE;

    if( !m_csFinishText.IsEmpty() )
        GetDlgItem(IDC_FINISH)->SetWindowText(m_csFinishText);

    if(!m_bEnablePartialButton)
    {
        CWnd* pWnd = GetDlgItem(IDOK);
        pWnd->EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDisPMDlg::OnPartialSave()
{
    m_eMode = PARTIAL_SAVE;
    CDialog::OnOK();
}

void CDisPMDlg::OnFinish()
{
    m_eMode = FINISH_CASE;
    CDialog::OnOK();
}

void CDisPMDlg::OnDiscard()
{
    m_eMode = DISCARD_CASE;
    CDialog::OnOK();
}

void CDisPMDlg::OnCancel()
{
    // TODO: Add extra cleanup here
    m_eMode = CANCEL_MODE;
    CDialog::OnCancel();
}
