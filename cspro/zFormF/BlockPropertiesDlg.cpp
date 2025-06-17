#include "StdAfx.h"
#include "BlockPropertiesDlg.h"


CBlockPropDlg::CBlockPropDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CBlockPropDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CBlockPropDlg)
    m_sBlockLabel = _T("");
    m_sBlockName = _T("");
    m_bDisplayOnSameScreen = TRUE;
    //}}AFX_DATA_INIT
    m_pBlock = NULL;
    m_pView = NULL;
}

CBlockPropDlg::CBlockPropDlg (CDEBlock* pBlock, CFormScrollView* pParent)
    : CDialog(CBlockPropDlg::IDD, pParent)
{
    m_pBlock = pBlock;
    m_pView = pParent;
    m_sBlockLabel = pBlock->GetLabel();
    m_sBlockName = pBlock->GetName();
    m_bDisplayOnSameScreen = pBlock->GetDisplayTogether();
}

void CBlockPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBlockPropDlg)
    DDX_Text(pDX, IDC_GROUP_LABEL, m_sBlockLabel);
    DDV_MaxChars(pDX, m_sBlockLabel, 100);
    DDX_Text(pDX, IDC_GROUP_NAME, m_sBlockName);
    DDX_Check(pDX, IDC_CHECK_SHOW_SAME_SCREEN, m_bDisplayOnSameScreen);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CBlockPropDlg message handlers

void CBlockPropDlg::OnOK()
{
    UpdateData(TRUE);
    if(m_sBlockLabel.IsEmpty()){
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem(IDC_GROUP_LABEL)->SetFocus();
        return;
    }

    if(m_sBlockName.IsEmpty()) {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_GROUP_NAME)->SetFocus();
        return;
    }

    if(!m_sBlockName.IsName()) {
        AfxMessageBox(_T("Name is not a valid name "));
        GetDlgItem(IDC_GROUP_NAME)->SetFocus();
        return;
    }
    if(m_sBlockName.IsReservedWord()) {
        CIMSAString sMsg;
        sMsg.FormatMessage(_T("%1 is a reserved word"), m_sBlockName.GetString());
        AfxMessageBox(sMsg);
        GetDlgItem(IDC_GROUP_NAME)->SetFocus();
        return;
    }

    if((m_pBlock->GetName().CompareNoCase(m_sBlockName) != 0) && m_pView) {
        CFormDoc* pDoc = m_pView->GetDocument();
        if(!AfxGetMainWnd()->SendMessage(UWM::Form::IsNameUnique, (WPARAM)m_sBlockName.GetString(), (LPARAM)pDoc)) {
            GetDlgItem(IDC_GROUP_NAME)->SetFocus();
            return;
        }
    }

    CDialog::OnOK();
}

BOOL CBlockPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    if(!m_pBlock)
        return FALSE;

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
