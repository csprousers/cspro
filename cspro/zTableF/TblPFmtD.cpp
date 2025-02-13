// TblPFmtD.cpp : implementation file
//

#include "StdAfx.h"
#include "TblPFmtD.h"
#include "FmtFontD.h"

const CIMSAString AUTO_STR= _T("(default)");

// CTblPrintFmtDlg dialog

IMPLEMENT_DYNAMIC(CTblPrintFmtDlg, CDialog)
CTblPrintFmtDlg::CTblPrintFmtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTblPrintFmtDlg::IDD, pParent)
    , m_iTblLayout(0)
    , m_iHeaderFrequency(0)
    , m_iStartPage(0)
    , m_fPageMarginLeft(0)
    , m_bShowDefaults(TRUE)
{
    m_sStartPageCtrl=AUTO_STR;
    m_pFmtTblPrint=NULL;
    m_pDefaultFmtTblPrint=NULL;
    m_uLastFolioControl=IDC_HEADER_LEFT;
    m_uLastMarginControl=IDC_PAGE_MARGIN_LEFT;
    ::ZeroMemory(&m_lfHeader,sizeof(LOGFONT));
    ::ZeroMemory(&m_lfFooter,sizeof(LOGFONT));
    ::ZeroMemory(&m_lfDefaultHeader,sizeof(LOGFONT));
    ::ZeroMemory(&m_lfDefaultFooter,sizeof(LOGFONT));
    m_bDefaultHeaderFont=FALSE;
    m_bDefaultFooterFont=FALSE;
}

CTblPrintFmtDlg::~CTblPrintFmtDlg()
{
}

void CTblPrintFmtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTblPrintFmtDlg)
    DDX_Text(pDX, IDC_HEADER_LEFT, m_sHeaderLeft);
    DDX_Text(pDX, IDC_HEADER_CENTER, m_sHeaderCenter);
    DDX_Text(pDX, IDC_HEADER_RIGHT, m_sHeaderRight);
    DDX_Text(pDX, IDC_FOOTER_LEFT, m_sFooterLeft);
    DDX_Text(pDX, IDC_FOOTER_CENTER, m_sFooterCenter);
    DDX_Text(pDX, IDC_FOOTER_RIGHT, m_sFooterRight);
    DDX_CBIndex(pDX, IDC_HEADER_FREQUENCY, m_iHeaderFrequency);
    DDX_CBIndex(pDX, IDC_TBL_LAYOUT, m_iTblLayout);
    DDX_Text(pDX, IDC_PAGE_MARGIN_LEFT, m_fPageMarginLeft);
    DDX_Text(pDX, IDC_PAGE_MARGIN_TOP, m_fPageMarginTop);
    DDX_Text(pDX, IDC_PAGE_MARGIN_RIGHT, m_fPageMarginRight);
    DDX_Text(pDX, IDC_PAGE_MARGIN_BOTTOM, m_fPageMarginBottom);
    DDX_Text(pDX, IDC_UNITS1, m_sUnits1);
    DDX_Text(pDX, IDC_UNITS2, m_sUnits2);
    DDX_Text(pDX, IDC_UNITS3, m_sUnits3);
    DDX_Text(pDX, IDC_UNITS4, m_sUnits4);
    DDX_Text(pDX, IDC_START_PAGE, m_sStartPageCtrl);
    DDX_Control(pDX, IDC_STARTPG_SPIN, m_spinStartPage);
    DDX_Control(pDX, IDC_HEADER_FREQUENCY, m_comboHeaderFrequency);
    DDX_Control(pDX, IDC_TBL_LAYOUT, m_comboTblLayout);
    //}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CTblPrintFmtDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_BUTTON_DATE, OnBnClickedButtonDate)
    ON_BN_CLICKED(IDC_BUTTON_TIME, OnBnClickedButtonTime)
    ON_BN_CLICKED(IDC_BUTTON_FILE, OnBnClickedButtonFile)
    ON_BN_CLICKED(IDC_BUTTON_INPUTFILE, OnBnClickedButtonInputFile)
    ON_BN_CLICKED(IDC_BUTTON_PAGE, OnBnClickedButtonPage)
    ON_EN_SETFOCUS(IDC_HEADER_LEFT, OnEnSetfocusHeaderLeft)
    ON_EN_SETFOCUS(IDC_HEADER_CENTER, OnEnSetfocusHeaderCenter)
    ON_EN_SETFOCUS(IDC_HEADER_RIGHT, OnEnSetfocusHeaderRight)
    ON_EN_SETFOCUS(IDC_FOOTER_LEFT, OnEnSetfocusFooterLeft)
    ON_EN_SETFOCUS(IDC_FOOTER_CENTER, OnEnSetfocusFooterCenter)
    ON_EN_SETFOCUS(IDC_FOOTER_RIGHT, OnEnSetfocusFooterRight)
    ON_BN_CLICKED(IDC_HEADER_FONT, OnBnClickedHeaderFont)
    ON_BN_CLICKED(IDC_FOOTER_FONT, OnBnClickedFooterFont)
    ON_BN_CLICKED(IDC_MAKE_EQUAL, OnBnClickedMakeEqual)
    ON_EN_CHANGE(IDC_PAGE_MARGIN_BOTTOM, OnEnChangePageMarginBottom)
    ON_EN_CHANGE(IDC_PAGE_MARGIN_LEFT, OnEnChangePageMarginLeft)
    ON_EN_CHANGE(IDC_PAGE_MARGIN_RIGHT, OnEnChangePageMarginRight)
    ON_EN_CHANGE(IDC_PAGE_MARGIN_TOP, OnEnChangePageMarginTop)
    ON_NOTIFY(UDN_DELTAPOS, IDC_STARTPG_SPIN, OnDeltaposStartpgSpin)
END_MESSAGE_MAP()



BOOL CTblPrintFmtDlg::OnInitDialog()
{
    if (m_pFmtTblPrint==NULL) {
        AfxMessageBox(_T("internal error 1 -- CTblPrintFmtDlg::m_pFmtTblPrint is null!"));
        return FALSE;
    }
    if (m_pDefaultFmtTblPrint==NULL) {
        AfxMessageBox(_T("internal error 2 -- CTblPrintFmtDlg::m_pDefaultFmtTblPrint is null!"));
        return FALSE;
    }

    LOGFONT lfTest;
    ::ZeroMemory(&lfTest,sizeof(LOGFONT));
    if (memcmp(&lfTest,&m_lfHeader,sizeof(LOGFONT))==0 && !m_bDefaultHeaderFont) {
        AfxMessageBox(_T("internal error 3 -- CTblPrintFmtDlg::m_lfHeader is not set!"));
        return FALSE;
    }
    if (memcmp(&lfTest,&m_lfFooter, sizeof(LOGFONT))==0 && !m_bDefaultFooterFont) {
        AfxMessageBox(_T("internal error 4 -- CTblPrintFmtDlg::m_lfFooter is not set!"));
        return FALSE;
    }
    if (memcmp(&lfTest,&m_lfDefaultHeader,sizeof(LOGFONT))==0) {
        AfxMessageBox(_T("internal error 5 -- CTblPrintFmtDlg::m_lfDefaultHeader is not set!"));
        return FALSE;
    }
    if (memcmp(&lfTest,&m_lfDefaultFooter, sizeof(LOGFONT))==0) {
        AfxMessageBox(_T("internal error 6 -- CTblPrintFmtDlg::m_lfDefaultFooter is not set!"));
        return FALSE;
    }

    ASSERT_VALID(m_pFmtTblPrint);
    ASSERT_VALID(m_pDefaultFmtTblPrint);

    CString s;

    CDialog::OnInitDialog();

    //////////////////////////////////////////////////////////////////////
    // set up controls ...
    m_spinStartPage.SetBuddy(GetDlgItem(IDC_START_PAGE));
    m_spinStartPage.SetRange(0,100);


    //////////////////////////////////////////////////////////////////////
    // add strings and set starting selections ...

    // stubs ... since tbl layout is a combination of m_comboTblLayout, we can't use SetItemData() directly.
    m_comboTblLayout.ResetContent();
    if (m_bShowDefaults) {
        switch(m_pDefaultFmtTblPrint->GetTblLayout()) {
        case TBL_LAYOUT_LEFT_STANDARD:
            s=_T("Default (Left side, standard)");
            break;
        case TBL_LAYOUT_LEFT_FACING:
            s=_T("Default (Left side, facing pages)");
            break;
        case TBL_LAYOUT_BOTH_STANDARD:
            s=_T("Default (Left and right sides, standard)");
            break;
        case TBL_LAYOUT_BOTH_FACING:
            s=_T("Default (Left and right sides, facing pages)");
            break;
        default:
            ASSERT(FALSE);
        }
        m_comboTblLayout.AddString(s);
        m_comboTblLayout.SetItemData(0,TBL_LAYOUT_DEFAULT);
    }
    m_comboTblLayout.SetItemData(m_comboTblLayout.AddString(_T("Left side, standard")),TBL_LAYOUT_LEFT_STANDARD);
    m_comboTblLayout.SetItemData(m_comboTblLayout.AddString(_T("Left side, facing pages")),TBL_LAYOUT_LEFT_FACING);
    m_comboTblLayout.SetItemData(m_comboTblLayout.AddString(_T("Left and right sides, standard")),TBL_LAYOUT_BOTH_STANDARD);
    m_comboTblLayout.SetItemData(m_comboTblLayout.AddString(_T("Left and right sides, facing pages")),TBL_LAYOUT_BOTH_FACING);

    switch (m_pFmtTblPrint->GetTblLayout()) {
    case HEADER_FREQUENCY_DEFAULT:
        ASSERT(m_bShowDefaults);
        m_comboTblLayout.SetCurSel(0);
        break;
    case TBL_LAYOUT_LEFT_STANDARD:
        m_comboTblLayout.SetCurSel(m_bShowDefaults ? 1 : 0);
        break;
    case TBL_LAYOUT_LEFT_FACING:
        m_comboTblLayout.SetCurSel(m_bShowDefaults ? 2 : 1);
        break;
    case TBL_LAYOUT_BOTH_STANDARD:
        m_comboTblLayout.SetCurSel(m_bShowDefaults ? 3 : 2);
        break;
    case TBL_LAYOUT_BOTH_FACING:
        m_comboTblLayout.SetCurSel(m_bShowDefaults ? 4 : 3);
        break;
    default:
        ASSERT(FALSE);
    }

    // header frequency ...
    m_comboHeaderFrequency.ResetContent();
    if (m_bShowDefaults) {
        switch(m_pDefaultFmtTblPrint->GetHeaderFrequency()) {
        case HEADER_FREQUENCY_NONE:
            s=_T("Default (No boxheads)");
            break;
        case HEADER_FREQUENCY_TOP_TABLE:
            s=_T("Default (Top of table only)");
            break;
        case HEADER_FREQUENCY_TOP_PAGE:
            s=_T("Default (Top of each page)");
            break;
        default:
            ASSERT(FALSE);
        }
        m_comboHeaderFrequency.AddString(s);
        m_comboHeaderFrequency.SetItemData(0,HEADER_FREQUENCY_DEFAULT);
    }
    m_comboHeaderFrequency.SetItemData(m_comboHeaderFrequency.AddString(_T("No boxheads")),HEADER_FREQUENCY_NONE);
    m_comboHeaderFrequency.SetItemData(m_comboHeaderFrequency.AddString(_T("Top of table only")),HEADER_FREQUENCY_TOP_TABLE);
    m_comboHeaderFrequency.SetItemData(m_comboHeaderFrequency.AddString(_T("Top of each page")),HEADER_FREQUENCY_TOP_PAGE);

    switch (m_pFmtTblPrint->GetHeaderFrequency()) {
    case HEADER_FREQUENCY_DEFAULT:
        ASSERT(m_bShowDefaults);
        m_comboHeaderFrequency.SetCurSel(0);
        break;
    case HEADER_FREQUENCY_NONE:
        m_comboHeaderFrequency.SetCurSel(m_bShowDefaults ? 1 : 0);
        break;
    case HEADER_FREQUENCY_TOP_TABLE:
        m_comboHeaderFrequency.SetCurSel(m_bShowDefaults ? 2 : 1);
        break;
    case HEADER_FREQUENCY_TOP_PAGE:
        m_comboHeaderFrequency.SetCurSel(m_bShowDefaults ? 3 : 2);
        break;
    default:
        ASSERT(FALSE);
    }
    UpdateData();

    CRect rcPageMargin(m_pFmtTblPrint->GetPageMargin());
    if (rcPageMargin.left==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.left=m_pDefaultFmtTblPrint->GetPageMargin().left;
    }
    if (rcPageMargin.top==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.top=m_pDefaultFmtTblPrint->GetPageMargin().top;
    }
    if (rcPageMargin.right==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.right=m_pDefaultFmtTblPrint->GetPageMargin().right;
    }
    if (rcPageMargin.bottom==PAGE_MARGIN_DEFAULT) {
        rcPageMargin.bottom=m_pDefaultFmtTblPrint->GetPageMargin().bottom;
    }
    if (m_pFmtTblPrint->GetUnits()==UNITS_METRIC) {
        m_fPageMarginLeft=TwipsToCm(rcPageMargin.left);
        m_fPageMarginTop=TwipsToCm(rcPageMargin.top);
        m_fPageMarginRight=TwipsToCm(rcPageMargin.right);
        m_fPageMarginBottom=TwipsToCm(rcPageMargin.bottom);
        m_sUnits1=m_sUnits2=m_sUnits3=m_sUnits4=_T("cm.");
        m_eUnits=UNITS_METRIC;
    }
    else {
        m_fPageMarginLeft=TwipsToInches(rcPageMargin.left);
        m_fPageMarginTop=TwipsToInches(rcPageMargin.top);
        m_fPageMarginRight=TwipsToInches(rcPageMargin.right);
        m_fPageMarginBottom=TwipsToInches(rcPageMargin.bottom);
        m_sUnits1=m_sUnits2=m_sUnits3=m_sUnits4=_T("in.");
        m_eUnits=UNITS_US;
    }

    if (m_pFmtTblPrint->GetStartPage()==START_PAGE_DEFAULT) {
        m_sStartPageCtrl.Str(m_pDefaultFmtTblPrint->GetStartPage());
    }
    else {
        m_sStartPageCtrl.Str(m_pFmtTblPrint->GetStartPage());
    }
    if (m_sStartPageCtrl==_T("0")) {
        m_sStartPageCtrl=AUTO_STR;
    }

    if (!m_bShowDefaults) {
        GetDlgItem(IDC_START_PAGE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_STARTPG_SPIN)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_START_PAGE_STATIC_LABEL)->ShowWindow(SW_HIDE);
    }
    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTblPrintFmtDlg::OnBnClickedOk()
{
    // verify starting page before allowing OnOK
    CIMSAString sStartPage;
    GetDlgItem(IDC_START_PAGE)->GetWindowText(sStartPage);

    if (sStartPage==AUTO_STR)  {
        m_iStartPage=0;
    }
    else if (!sStartPage.IsNumeric() || sStartPage.FindOneOf(_T("'."))!=NONE)  {
        AfxMessageBox(_T("Invalid starting page number. (Use 0 (zero) for automatic starting page number.)"), MB_ICONEXCLAMATION);
        m_iStartPage = 0;
        return;
    }
    else  {
        m_iStartPage = (int)sStartPage.Val();
    }

    OnOK();

    // output variables ...
    if (m_pFmtTblPrint->GetUnits()==UNITS_METRIC) {
        m_rcPageMargin.left=CmToTwips(m_fPageMarginLeft);
        m_rcPageMargin.top=CmToTwips(m_fPageMarginTop);
        m_rcPageMargin.right=CmToTwips(m_fPageMarginRight);
        m_rcPageMargin.bottom=CmToTwips(m_fPageMarginBottom);
    }
    else {
        m_rcPageMargin.left=InchesToTwips(m_fPageMarginLeft);
        m_rcPageMargin.top=InchesToTwips(m_fPageMarginTop);
        m_rcPageMargin.right=InchesToTwips(m_fPageMarginRight);
        m_rcPageMargin.bottom=InchesToTwips(m_fPageMarginBottom);
    }

    // header frequency
    m_eHeaderFrequency=(HEADER_FREQUENCY)m_comboHeaderFrequency.GetItemData(m_iHeaderFrequency);

    // table layout
    m_eTblLayout=(TBL_LAYOUT)m_comboTblLayout.GetItemData(m_iTblLayout);



    // convert defaults to real values ...

// csc: this needs work ... if we start with TBL_LAYOUT_DEFAULT and don't change it, then we need to set
// m_eTblLayout==TBL_LAYOUT_DEFAULT
// same deal with page margins


/*
    if (m_iStubs==0) {
        switch(m_pDefaultFmtTblPrint->GetTblLayout()) {
        case TBL_LAYOUT_LEFT_STANDARD:
        case TBL_LAYOUT_LEFT_FACING:
            m_iStubs=1;
            break;
        case TBL_LAYOUT_BOTH_STANDARD:
        case TBL_LAYOUT_BOTH_FACING:
            m_iStubs=2;
            break;
        default:
            ASSERT(FALSE);
        }
    }
    if (m_iMultiPage==0) {
        switch(m_pDefaultFmtTblPrint->GetTblLayout()) {
        case TBL_LAYOUT_LEFT_STANDARD:
        case TBL_LAYOUT_BOTH_STANDARD:
            m_iMultiPage=1;
            break;
        case TBL_LAYOUT_LEFT_FACING:
        case TBL_LAYOUT_BOTH_FACING:
            m_iMultiPage=2;
            break;
        default:
            ASSERT(FALSE);
        }
    }

    ASSERT(m_iStubs>0 && m_iMultiPage>0);
    if (m_iStubs==1 && m_iMultiPage==1) {
        m_eTblLayout=TBL_LAYOUT_LEFT_STANDARD;
    }
    else if (m_iStubs==1 && m_iMultiPage==2) {
        m_eTblLayout=TBL_LAYOUT_LEFT_FACING;
    }
    else if (m_iStubs==2 && m_iMultiPage==1) {
        m_eTblLayout=TBL_LAYOUT_BOTH_STANDARD;
    }
    else if (m_iStubs==2 && m_iMultiPage==2) {
        m_eTblLayout=TBL_LAYOUT_BOTH_FACING;
    }
    else {
        ASSERT(FALSE);
    }
*/
}



void CTblPrintFmtDlg::OnBnClickedButtonDate()
{
    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastFolioControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&D"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CTblPrintFmtDlg::OnBnClickedButtonTime()
{
    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastFolioControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&T"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CTblPrintFmtDlg::OnBnClickedButtonFile()
{
    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastFolioControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&F"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}


void CTblPrintFmtDlg::OnBnClickedButtonInputFile() // GHM 20090916
{
    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastFolioControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&I"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}


void CTblPrintFmtDlg::OnBnClickedButtonPage()
{
    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastFolioControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&P"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}


void CTblPrintFmtDlg::OnEnSetfocusHeaderLeft()
{
    m_uLastFolioControl=IDC_HEADER_LEFT;
}

void CTblPrintFmtDlg::OnEnSetfocusHeaderCenter()
{
    m_uLastFolioControl=IDC_HEADER_CENTER;
}

void CTblPrintFmtDlg::OnEnSetfocusHeaderRight()
{
    m_uLastFolioControl=IDC_HEADER_RIGHT;
}

void CTblPrintFmtDlg::OnEnSetfocusFooterLeft()
{
    m_uLastFolioControl=IDC_FOOTER_LEFT;
}

void CTblPrintFmtDlg::OnEnSetfocusFooterCenter()
{
    m_uLastFolioControl=IDC_FOOTER_CENTER;
}

void CTblPrintFmtDlg::OnEnSetfocusFooterRight()
{
    m_uLastFolioControl=IDC_FOOTER_RIGHT;
}

void CTblPrintFmtDlg::OnDeltaposStartpgSpin(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

    CIMSAString sStartPage;
    GetDlgItem(IDC_START_PAGE)->GetWindowText(sStartPage);

    if (sStartPage==AUTO_STR)  {
        m_iStartPage=pNMUpDown->iDelta;
    }
    else if (!sStartPage.IsNumeric() || sStartPage.FindOneOf(_T("'."))!=NONE)  {
        AfxMessageBox(_T("Invalid starting page number. (Use 0 (zero) for automatic starting page number.)"), MB_ICONEXCLAMATION);
        m_iStartPage = 0;
    }
    else  {
        m_iStartPage = (int) sStartPage.Val() + pNMUpDown->iDelta;
    }

    // rounding issues...
    if (m_iStartPage<0)  {
        m_iStartPage=0;
    }
    if (m_iStartPage==0)  {
        sStartPage = AUTO_STR;
    }
    else  {
        sStartPage = IntToString(m_iStartPage);
    }

    GetDlgItem(IDC_START_PAGE)->SetWindowText(sStartPage);
    *pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  OnBnClickedHeaderFont()
//  OnBnClickedFooterFont()
//
// Lets the user change the font used by header formats.
//
// Even though each header has its own font, for now we force all to use the same format.
//
/////////////////////////////////////////////////////////////////////////////////
void CTblPrintFmtDlg::OnBnClickedHeaderFont()
{
    LOGFONT lf;
    if (m_bDefaultHeaderFont) {
        lf=m_lfDefaultHeader;
    }
    else {
        lf=m_lfHeader;
    }

    CFmtFontDlg dlg(&lf, CF_SCREENFONTS|CF_EFFECTS);
    dlg.m_bUseDefault=m_bDefaultHeaderFont;
    dlg.m_lfDef=m_lfDefaultHeader;

    if (dlg.DoModal()==IDOK) {
        m_bDefaultHeaderFont=dlg.m_bUseDefault?true:false;
        if (!m_bDefaultHeaderFont) {
            m_lfHeader=*dlg.m_cf.lpLogFont;
        }
    }
}


void CTblPrintFmtDlg::OnBnClickedFooterFont()
{
    LOGFONT lf;
    if (m_bDefaultFooterFont) {
        lf=m_lfDefaultFooter;
    }
    else {
        lf=m_lfFooter;
    }

    CFmtFontDlg dlg(&lf, CF_SCREENFONTS|CF_EFFECTS);
    dlg.m_bUseDefault=m_bDefaultFooterFont;
    dlg.m_lfDef=m_lfDefaultFooter;

    if (dlg.DoModal()==IDOK) {
        m_bDefaultFooterFont=dlg.m_bUseDefault?true:false;
        if (!m_bDefaultFooterFont) {
            m_lfFooter=*dlg.m_cf.lpLogFont;
        }
    }
}


void CTblPrintFmtDlg::OnBnClickedMakeEqual()
{
    CString sMsg;
    float fLastChangedMargin;

    UpdateData();

    switch (m_uLastMarginControl) {
    case IDC_PAGE_MARGIN_LEFT:
        fLastChangedMargin=m_fPageMarginLeft;
        break;
    case IDC_PAGE_MARGIN_RIGHT:
        fLastChangedMargin=m_fPageMarginRight;
        break;
    case IDC_PAGE_MARGIN_TOP:
        fLastChangedMargin=m_fPageMarginTop;
        break;
    case IDC_PAGE_MARGIN_BOTTOM:
        fLastChangedMargin=m_fPageMarginBottom;
        break;
    default:
        ASSERT(FALSE);
    }

    sMsg.Format(_T("Do you want to set all page margins to %f %s?"), fLastChangedMargin, (const TCHAR*)m_sUnits1);
    if (AfxMessageBox(sMsg, MB_ICONQUESTION|MB_YESNOCANCEL)==IDYES) {
        m_fPageMarginLeft=fLastChangedMargin;
        m_fPageMarginTop=fLastChangedMargin;
        m_fPageMarginRight=fLastChangedMargin;
        m_fPageMarginBottom=fLastChangedMargin;
    }
    UpdateData(FALSE);
}


void CTblPrintFmtDlg::OnEnChangePageMarginLeft()
{
    m_uLastMarginControl=IDC_PAGE_MARGIN_LEFT;
}

void CTblPrintFmtDlg::OnEnChangePageMarginRight()
{
    m_uLastMarginControl=IDC_PAGE_MARGIN_RIGHT;
}

void CTblPrintFmtDlg::OnEnChangePageMarginTop()
{
    m_uLastMarginControl=IDC_PAGE_MARGIN_TOP;
}

void CTblPrintFmtDlg::OnEnChangePageMarginBottom()
{
    m_uLastMarginControl=IDC_PAGE_MARGIN_BOTTOM;
}

// helper functions for intializing and checking for change in folio text (shared between
// CTabView and CPrintView so easiest to put them here).
void SetupFolioText(CTblOb* pFolioObj, FMT_ID id, CIMSAString& sTxtToSet, CFmtReg* pFmtReg)
{
    if (pFolioObj->GetDerFmt()) {
        sTxtToSet=pFolioObj->GetDerFmt()->GetCustomText();
    }
    else {
        sTxtToSet = (DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(id)))->GetCustomText();
    }
}

bool FolioTextNeedsUpdate(CTblOb* pFolioObj, FMT_ID id, const CIMSAString& sNewTxt, CFmtReg* pFmtReg)
{
    CFmt* pCurrFmt = pFolioObj->GetDerFmt();
    if (pCurrFmt == NULL) {
        pCurrFmt = DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(id));
    }

    return pCurrFmt->GetCustomText()!=sNewTxt;
}

bool UpdateFolioText(CTblOb* pFolioObj, FMT_ID id, const CIMSAString& sNewTxt, CFmtReg* pFmtReg)
{
    if (FolioTextNeedsUpdate(pFolioObj, id, sNewTxt, pFmtReg)) {
        CFmt* pDefFmt = DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(id));
        CFmt* pNewFmt=new CFmt(*pDefFmt);
        pNewFmt->SetCustomText(sNewTxt);
        int iIndex=pFmtReg->GetNextCustomFmtIndex(id);
        pNewFmt->SetIndex(iIndex);
        if (!pFmtReg->AddFmt(pNewFmt)) {
            ASSERT(FALSE);
        }
        pFolioObj->SetFmt(pNewFmt);
        return true;
    }

    return false;
}
