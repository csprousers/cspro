// TSPDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "TSPDlg.h"
#include "TabTrCtl.h"

// CTabSetPropDlg dialog

IMPLEMENT_DYNAMIC(CTabSetPropDlg, CDialog)
CTabSetPropDlg::CTabSetPropDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTabSetPropDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CTabSetPropDlg)
    m_sTSLabel = _T("");
    m_sTSName = _T("");
    //}}AFX_DATA_INIT
}


CTabSetPropDlg::CTabSetPropDlg (CTabSet* pTabSet, CTabTreeCtrl* pParent)
    : CDialog(CTabSetPropDlg::IDD, pParent)
{
    m_sTSName = pTabSet->GetName();
    m_sTSLabel = pTabSet->GetLabel();

//  m_pCTabTreeCtrl = pParent;
    m_pTabSet = pTabSet;
}

CTabSetPropDlg::~CTabSetPropDlg()
{
}

void CTabSetPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTabSetPropDlg)
    DDX_Text(pDX, IDC_FF_LABEL, m_sTSLabel);
    DDV_MaxChars(pDX, m_sTSLabel, 132);
    DDX_Text(pDX, IDC_FF_NAME, m_sTSName);
    //}}AFX_DATA_MAP
}



// CTabSetPropDlg message handlers

void CTabSetPropDlg::OnOK()
{
    UpdateData(TRUE);

    if(m_sTSLabel.IsEmpty())
    {
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem (IDC_FF_LABEL)->SetFocus();
    }

    if(m_sTSName.IsEmpty())
    {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_FF_NAME)->SetFocus();
    }

    if(!m_sTSName.IsName())
    {
        AfxMessageBox(_T("Not a Valid Name"));
        return;
    }
    if(m_sTSName.IsReservedWord())
    {
        CIMSAString sMsg;
        sMsg.FormatMessage(_T("%1 is a reserved word"), (LPCTSTR)m_sTSName);
        AfxMessageBox (sMsg);
        return;
    }

    CDialog::OnOK();
}

BOOL CTabSetPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
