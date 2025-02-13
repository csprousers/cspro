// AppFmtD.cpp : implementation file
//

#include "StdAfx.h"
#include "AppFmtD.h"



// CAppFmtDlg dialog

IMPLEMENT_DYNAMIC(CAppFmtDlg, CDialog)
CAppFmtDlg::CAppFmtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAppFmtDlg::IDD, pParent)
    , m_sTitleTemplate(_T(""))
    , m_sContinueTxt(_T(""))
    , m_sAlternateTxt(_T(""))
    , m_sDigitGrpSymbol(_T(""))
    , m_sDecimalSymbol(_T(""))
{
}

CAppFmtDlg::~CAppFmtDlg()
{
}

void CAppFmtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DIGIT_GRP, m_DigitGrp);
    DDX_Control(pDX, IDC_DISPLAY_LDG_ZEROES, m_LeadingZeroes);
    DDX_Control(pDX, IDC_UNITS, m_Units);
    DDX_Text(pDX, IDC_TITLE_TEMPLATE, m_sTitleTemplate);
    DDX_Text(pDX, IDC_CONTINUATION, m_sContinueTxt);
    DDX_Control(pDX, IDC_DEFAUL_TXT, m_DefTxt);
    DDX_Text(pDX, IDC_ALTERNATE_TXT, m_sAlternateTxt);
    DDX_Control(pDX, IDC_DECIMAL_SYMBL, m_DecSymbl);
    DDX_Control(pDX, IDC_DIGIT_GRP_SYMBL, m_DigitGrpSymbl);
    DDX_Control(pDX, IDC_ROUND_MASK, m_zRound);
    DDX_Control(pDX, IDC_ZERO_MASK, m_ZeroMask);
    DDX_CBString(pDX, IDC_DIGIT_GRP_SYMBL, m_sDigitGrpSymbol);
    DDV_MaxChars(pDX, m_sDigitGrpSymbol, 3);
    DDX_CBString(pDX, IDC_DECIMAL_SYMBL, m_sDecimalSymbol);
    DDV_MaxChars(pDX, m_sDecimalSymbol, 3);
}


BEGIN_MESSAGE_MAP(CAppFmtDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_DEFAUL_TXT, OnCbnSelchangeDefaulTxt)
    ON_CBN_SELENDOK(IDC_DEFAUL_TXT, OnCbnSelendokDefaulTxt)
END_MESSAGE_MAP()


// CAppFmtDlg message handlers

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CAppFmtDlg::OnInitDialog()
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CAppFmtDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ASSERT(m_pFmt);

    //Decimal Symbol
    m_DecSymbl.AddString(m_pFmt->GetDecimalSep());
    m_DecSymbl.SetCurSel(0);
    m_sDecimalSymbol=m_pFmt->GetDecimalSep();
    //Digit grp symbol
    m_DigitGrpSymbl.AddString(m_pFmt->GetThousandsSep());
    m_DigitGrpSymbl.SetCurSel(0);
    m_sDigitGrpSymbol = m_pFmt->GetThousandsSep();
    //Leading Zeroes
    m_LeadingZeroes.AddString(_T("0.7"));
    m_LeadingZeroes.SetItemData(0,1);
    m_LeadingZeroes.AddString(_T(".7"));
    m_LeadingZeroes.SetItemData(1,0);
    m_pFmt->GetZeroBeforeDecimal() ?m_LeadingZeroes.SetCurSel(0):m_LeadingZeroes.SetCurSel(1);
    //Digit Grouping
    m_DigitGrp.AddString(_T("123,456,789"));
    m_DigitGrp.SetItemData(0,DIGIT_GROUPING_THOUSANDS);
    m_DigitGrp.AddString(_T("123456789"));
    m_DigitGrp.SetItemData(1,DIGIT_GROUPING_NONE);
    m_DigitGrp.AddString(_T("12,34,56,789"));
    m_DigitGrp.SetItemData(2,DIGIT_GROUPING_INDIC);
    switch(m_pFmt->GetDigitGrouping()){
        case DIGIT_GROUPING_THOUSANDS:
            m_DigitGrp.SetCurSel(0);
            break;
        case DIGIT_GROUPING_NONE:
            m_DigitGrp.SetCurSel(1);
            break;
        case DIGIT_GROUPING_INDIC:
            m_DigitGrp.SetCurSel(2);
            break;
        default:
            ASSERT(FALSE);
            m_DigitGrp.SetCurSel(0);
            break;
    }
    //units
    m_Units.AddString(_T("Metric"));
    m_Units.SetItemData(0,UNITS_METRIC);
    m_Units.AddString(_T("U.S."));
    m_Units.SetItemData(1,UNITS_US);
    m_pFmt->GetUnits() == UNITS_METRIC ? m_Units.SetCurSel(0): m_Units.SetCurSel(1);
    //ZRound Mask
    m_zRound.AddString(m_pFmt->GetZRoundMask());
    m_zRound.SetCurSel(0);
    m_zRound.AddString(_T(""));
    //Zero Mask
    m_ZeroMask.AddString(m_pFmt->GetZeroMask());
    m_ZeroMask.SetCurSel(0);
    m_zRound.AddString(_T(""));


    m_sTitleTemplate = m_pFmt->GetTitleTemplate();//Title Template
    m_sContinueTxt = m_pFmt->GetContinuationStr();//Continutation string

    //FOREIGN_KEYS
    const FOREIGN_KEYS& altForeingKeys = m_pFmt->GetAltForeignKeys();
    CStringArray aDefaultTexts;
    altForeingKeys.GetAllKeys(aDefaultTexts);

    //memcpy(&m_altKeys,&altForeingKeys,sizeof(FOREIGN_KEYS));
    m_altKeys = altForeingKeys;

    for (int iKey = 0; iKey < aDefaultTexts.GetCount(); ++iKey) {
        m_DefTxt.AddString(aDefaultTexts.GetAt(iKey));
    }

    m_iCurSel = 0;
    m_DefTxt.SetCurSel(m_iCurSel);
    m_sAlternateTxt = m_altKeys.GetKey(aDefaultTexts.GetAt(0));

    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CAppFmtDlg::OnOK()
//
/////////////////////////////////////////////////////////////////////////////////
void CAppFmtDlg::OnOK()
{
    // TODO: Add your specialized code here and/or call the base class
    CDialog::OnOK();
    //Decimal Symbol
    CIMSAString sDecSymbl;
    m_DecSymbl.GetWindowText(sDecSymbl);
    m_pFmt->SetDecimalSep(sDecSymbl);

    //thousands seperator
    CIMSAString sDigitGrpSymbl;
    m_DigitGrpSymbl.GetWindowText(sDigitGrpSymbl);
    m_pFmt->SetThousandsSep(sDigitGrpSymbl);

    //leading zeroes
    int iCurrSelLZ = m_LeadingZeroes.GetCurSel();
    int iIDLZ = m_LeadingZeroes.GetItemData(iCurrSelLZ);
    bool bLeadingZeroes = m_LeadingZeroes.GetItemData(m_LeadingZeroes.GetCurSel()) ? true: false;
    m_pFmt->SetZeroBeforeDecimal(bLeadingZeroes);

    //units
    m_pFmt->SetUnits((UNITS)m_Units.GetItemData(m_Units.GetCurSel()));
    //Title / continue strings
    m_pFmt->SetTitleTemplate(m_sTitleTemplate);
    m_pFmt->SetContinuationStr(m_sContinueTxt);

    //Digit grouping
    m_pFmt->SetDigitGrouping((DIGIT_GROUPING)m_DigitGrp.GetItemData(m_DigitGrp.GetCurSel()));

     //zero mask
    CIMSAString sZeroMask;
    m_ZeroMask.GetWindowText(sZeroMask);
    m_pFmt->SetZeroMask(sZeroMask);

     //zround mask
    CIMSAString sZRoundMask;
    m_zRound.GetWindowText(sZRoundMask);
    m_pFmt->SetZRoundMask(sZRoundMask);

    OnCbnSelendokDefaulTxt();
    m_pFmt->SetAltForeignKeys(m_altKeys);

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CAppFmtDlg::OnCbnSelchangeDefaulTxt()
//
/////////////////////////////////////////////////////////////////////////////////
void CAppFmtDlg::OnCbnSelchangeDefaulTxt()
{
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CAppFmtDlg::OnCbnSelendokDefaulTxt()
//
/////////////////////////////////////////////////////////////////////////////////
void CAppFmtDlg::OnCbnSelendokDefaulTxt()
{
    CIMSAString sEditString;
    CIMSAString sDefTxt;
    m_DefTxt.GetLBText(m_iCurSel, sDefTxt);
    GetDlgItem(IDC_ALTERNATE_TXT)->GetWindowText(sEditString);
    m_altKeys.SetKey(sDefTxt, sEditString);

    m_iCurSel = m_DefTxt.GetCurSel();
    m_DefTxt.GetLBText(m_iCurSel, sDefTxt);
    GetDlgItem(IDC_ALTERNATE_TXT)->SetWindowText(m_altKeys.GetKey(sDefTxt));
}
