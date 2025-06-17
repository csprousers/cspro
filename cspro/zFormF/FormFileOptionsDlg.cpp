#include "StdAfx.h"
#include "FormFileOptionsDlg.h"


CFFOptionsDlg::CFFOptionsDlg(CWnd* pParent, CDEFormFile* pFormFile)
    :   CDialog(CFFOptionsDlg::IDD, pParent),
        m_pFormFile(pFormFile)
{
    m_iPath = pFormFile->IsPathOn() ? 1 : // then it's a survey, system controlled
                                      0;  // then it's a census, operator controlled

    m_bRTLRosters = pFormFile->GetRTLRostersFlag();
}

void CFFOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFFOptionsDlg)
    DDX_Radio(pDX, IDC_PATH_OFF, m_iPath);
    DDX_Check(pDX, IDC_ASKOPID, m_bAskOpID);
    DDX_Check(pDX, IDC_SHOWENDCASE, m_bShowEndCaseMsg);
    DDX_Check(pDX, IDC_ALLOW_PART_SAVE, m_bAllowPartialSave);
    DDX_Text(pDX, IDC_FREQ, m_iVerifyFreq);
    DDV_MinMaxUInt(pDX, m_iVerifyFreq, 1, Application::GetVerifyFreqMax());
    DDX_Text(pDX, IDC_STARTVER, m_iVerifyStart);
    DDX_Check(pDX, IDC_RNDSTART, m_bRandom);
    DDX_Control(pDX, IDC_COMBO_CASE_TREE, m_comboCaseTree);
    DDX_Check(pDX, IDC_USE_QUESTION_TEXT, m_bUseQuestionText);
    DDX_Check(pDX, IDC_SHOWREFUSALS, m_bShowRefusals);
    DDX_Check(pDX, IDC_CENTERFORMS, m_bCenterForms);
    DDX_Check(pDX, IDC_COMMADECIMAL, m_bDecimalComma); // 20120306
    DDX_Check(pDX, IDC_RTLROSTERS, m_bRTLRosters);
    DDX_Check(pDX, IDC_AUTO_ADVANCE, m_bAutoAdvanceOnSelection);
    DDX_Check(pDX, IDC_DISPLAY_CODES_WITH_LABELS, m_bDisplayCodesAlongsideLabels);
    DDX_Check(pDX, IDC_SHOW_FIELD_LABELS, m_bShowFieldLabels);
    DDX_Check(pDX, IDC_SHOW_ERROR_MESSAGE_NUMBERS, m_bShowErrorMessageNumbers);
    DDX_Check(pDX, IDC_SHOW_ONLY_DISCRETE_IN_COMBO, m_bComboBoxShowOnlyDiscreteValues);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFFOptionsDlg, CDialog)
    //{{AFX_MSG_MAP(CFFOptionsDlg)
    ON_BN_CLICKED(IDC_RNDSTART, OnRndstart)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFFOptionsDlg message handlers

BOOL CFFOptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    if(m_iVerifyStart == -1) {
        m_bRandom = 1;
        m_iVerifyStart =1;
    }

    if(!m_pFormFile)
        return FALSE;
    CSpinButtonCtrl* pStart = (CSpinButtonCtrl*)GetDlgItem (IDC_SPINSTART);
    CWnd* pWnd = GetDlgItem(IDC_STARTVER);
    pStart->SetBuddy(pWnd);
    pStart->SetRange(1,99);
    pWnd = GetDlgItem(IDC_FREQ);
    CSpinButtonCtrl* pFreq = (CSpinButtonCtrl*)GetDlgItem (IDC_SPINFREQ);
    pFreq->SetBuddy(pWnd);
    pFreq->SetRange(1,99);
    UpdateData(FALSE);
    if(m_bRandom) {
        CWnd* pWnd = GetDlgItem(IDC_STARTVER);
        pWnd->EnableWindow(!m_bRandom);
        pStart->EnableWindow(!m_bRandom);
    }

    m_comboCaseTree.SetCurSel((int)m_caseTreeType + 1);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CFFOptionsDlg::OnOK()
{
    if(!UpdateData(true)) {
        return;
    }
    if(m_bRandom) {
        m_iVerifyStart = -1;
    }
    else {
        if(m_iVerifyStart < 1 || m_iVerifyStart > (int)m_iVerifyFreq) {
            AfxMessageBox(_T("Start value should be between 1 and verify frequency  value"));
            CWnd* pWnd = GetDlgItem(IDC_STARTVER);
            pWnd->SetFocus();
            return;
        }
    }

    m_pFormFile->IsPathOn(m_iPath == 1);

    m_caseTreeType = (CaseTreeType)( m_comboCaseTree.GetCurSel() - 1 );

    CDialog::OnOK();
}

void CFFOptionsDlg::OnRndstart()
{
    CButton* pCB = (CButton*) GetDlgItem (IDC_RNDSTART);
    CWnd* pWnd = GetDlgItem(IDC_STARTVER);
    pWnd->EnableWindow(!pCB->GetCheck());
    CSpinButtonCtrl* pStart = (CSpinButtonCtrl*)GetDlgItem (IDC_SPINSTART);
    pStart->EnableWindow(!pCB->GetCheck());
}
