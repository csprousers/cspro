// FlashMsg.cpp : implementation file
//

#include "StdAfx.h"
#include "FlashMsg.h"


// CFlashMsgDlg dialog

IMPLEMENT_DYNAMIC(CFlashMsgDlg, CDialog)
CFlashMsgDlg::CFlashMsgDlg(CWnd* pParent /*=NULL*/, int iSeconds /*=2*/)
    : CDialog(CFlashMsgDlg::IDD, pParent)
    , m_sFlashMsg(_T(""))
{
    m_iSeconds=iSeconds;
}

CFlashMsgDlg::~CFlashMsgDlg()
{
}

void CFlashMsgDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFlashMsgDlg)
    DDX_Text(pDX, IDC_FLASHMSG, m_sFlashMsg);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFlashMsgDlg, CDialog)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CFlashMsgDlg message handlers

void CFlashMsgDlg::OnTimer(UINT nIDEvent)
{
    if (nIDEvent==FLASH_TIMER) {
        KillTimer(FLASH_TIMER);
        PostMessage(WM_COMMAND, IDOK);
    }
    CDialog::OnTimer(nIDEvent);
}

BOOL CFlashMsgDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetTimer(FLASH_TIMER, m_iSeconds*1000, NULL);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
