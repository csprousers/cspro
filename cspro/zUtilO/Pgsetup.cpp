//***************************************************************************
//  File name: PGSETUP.CPP
//
//  Description:
//       Implementation for CPageMetrics
//                          CFolio
//                          CIMSAPageSetupDlg
//
//  NOTE: Place change history in the *.h file.
//
//***************************************************************************

#include "StdAfx.h"
#include "Pgsetup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//
//                           CPageMetrics::UpdateUserMargins
//
/////////////////////////////////////////////////////////////////////////////

void CPageMetrics::UpdateUserMargins(CRect& userMargins) {

    m_rectUserMargins = userMargins;

//      Get DC
    CDC* pDC = CreatePrinterDC();
    pDC->SetMapMode(MM_TWIPS);

//      Get Physical page size in device units
    CSize sizePage;
    sizePage.cx = pDC->GetDeviceCaps(PHYSICALWIDTH);
    sizePage.cy = pDC->GetDeviceCaps(PHYSICALHEIGHT);
//      Get Printable page size in device units
    CRect rectPrintable;
    rectPrintable.top = 0;
    rectPrintable.left = 0;
    rectPrintable.right  = pDC->GetDeviceCaps(HORZRES);
    rectPrintable.bottom = pDC->GetDeviceCaps(VERTRES);
//      Get Left/top hardware margins in device units
    CRect rectHardMargins;
    rectHardMargins.left = pDC->GetDeviceCaps(PHYSICALOFFSETX);
    rectHardMargins.top  = pDC->GetDeviceCaps(PHYSICALOFFSETY);
//      Compute right/bottom hardware margins in device units
    rectHardMargins.right  = sizePage.cx - rectPrintable.Width()  - rectHardMargins.left;
    rectHardMargins.bottom = sizePage.cy - rectPrintable.Height() - rectHardMargins.top;
//      Convert hardware margins to twips
    pDC->DPtoLP(&rectHardMargins);
    rectHardMargins.top    = abs(rectHardMargins.top);
    rectHardMargins.bottom = abs(rectHardMargins.bottom);
//      Adjust user margins to >= hardware margins
    m_rectUserMargins.top    = std::max(m_rectUserMargins.top, rectHardMargins.top);
    m_rectUserMargins.left   = std::max(m_rectUserMargins.left, rectHardMargins.left);
    m_rectUserMargins.bottom = std::max(m_rectUserMargins.bottom, rectHardMargins.bottom);
    m_rectUserMargins.right  = std::max(m_rectUserMargins.right, rectHardMargins.right);
//      Convert printable area to page size to twips
    pDC->DPtoLP(&rectPrintable);
//      Compute user area in twips
    m_rectUserArea.top = - m_rectUserMargins.top + rectHardMargins.top;
    m_rectUserArea.left = m_rectUserMargins.left - rectHardMargins.left;
    m_rectUserArea.bottom = rectPrintable.bottom + m_rectUserMargins.bottom - rectHardMargins.bottom;
    m_rectUserArea.right = rectPrintable.right - m_rectUserMargins.right + rectHardMargins.right;

    userMargins = m_rectUserMargins;
//      Must free DC
    ::DeleteDC(pDC->GetSafeHdc());
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CPageMetrics::CreatePrinterDC
//
/////////////////////////////////////////////////////////////////////////////

CDC* CPageMetrics::CreatePrinterDC() {
   PRINTDLG PrtDlg;
   HDC      hDC;

   if (!AfxGetApp()->GetPrinterDeviceDefaults(&PrtDlg)) {
      TRACE(_T("No default printer.\n"));
//        Use screen DC for calculations
//        It's OK to do this because this CDC will not be used for any output.
      hDC = ::CreateDC(_T("display"),NULL,NULL,NULL);
   }
   else {
      CPrintDialog dlg(FALSE);

      dlg.m_pd.hDevMode = PrtDlg.hDevMode;
      dlg.m_pd.hDevNames = PrtDlg.hDevNames;
      hDC = dlg.CreatePrinterDC();
   }

   CDC* pDC = CDC::FromHandle(hDC);

//     This is a printer DC, so set m_bPrinting
//     this is necessary so CScrollView::OnPrepareDC won't modify the
//     ViewportOrg, and cause LPtoDP to return an inappropriate result.
   if( pDC )
   pDC->m_bPrinting = TRUE;

   return pDC;

}


/////////////////////////////////////////////////////////////////////////////
//
//                                CFolio::Create
//
/////////////////////////////////////////////////////////////////////////////

void CFolio::Create(CIMSAString csFileName) {

    m_csFileName = csFileName;

    m_csHeaderLeft = AfxGetApp()->GetProfileString(_T("Header"), _T("Left"), _T("&F"));
    m_csHeaderCenter = AfxGetApp()->GetProfileString(_T("Header"), _T("Center"), NULL);
    m_csHeaderRight = AfxGetApp()->GetProfileString(_T("Header"), _T("Right"), _T("&D"));

    if (m_csHeaderLeft.GetLength() + m_csHeaderCenter.GetLength() + m_csHeaderRight.GetLength() == 0) {
        m_bHeader = FALSE;
    }
    else {
        m_bHeader = TRUE;
        SetDateTime(m_csHeaderLeft);
        SetDateTime(m_csHeaderCenter);
        SetDateTime(m_csHeaderRight);
    }

    m_csFooterLeft = AfxGetApp()->GetProfileString(_T("Footer"), _T("Left"), NULL);
    m_csFooterCenter = AfxGetApp()->GetProfileString(_T("Footer"), _T("Center"), _T("- &P -"));
    m_csFooterRight = AfxGetApp()->GetProfileString(_T("Footer"), _T("Right"), NULL);

    if (m_csFooterLeft.GetLength() + m_csFooterCenter.GetLength() + m_csFooterRight.GetLength() == 0) {
        m_bFooter = FALSE;
    }
    else {
        m_bFooter = TRUE;
        SetDateTime(m_csFooterLeft);
        SetDateTime(m_csFooterCenter);
        SetDateTime(m_csFooterRight);
    }

    m_rectMargins.top = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Top"), _T("0")));
    m_rectMargins.bottom = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Bottom"), _T("0")));
    m_rectMargins.left = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Left"), _T("0")));
    m_rectMargins.right = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Right"), _T("0")));

    CPageMetrics pageMetrics;
    pageMetrics.UpdateUserMargins(m_rectMargins);
    m_rectUserArea = pageMetrics.GetUserArea();
}


/////////////////////////////////////////////////////////////////////////////
//
//                                CFolio::ExpandText
//
/////////////////////////////////////////////////////////////////////////////

/*static*/ CIMSAString CFolio::ExpandText(const CIMSAString& csText, const CIMSAString& csFileName, int iPage) {

    int i;
    CIMSAString csTemp;
    CIMSAString csRet(csText);

    while ((i = csRet.Find(_T("&D"))) != -1) {;
        csTemp.Date();
        csRet = csRet.Left(i) + csTemp + csRet.Mid(i + 2);
    }
    while ((i = csRet.Find(_T("&T"))) != -1) {;
        csTemp.Time();
        csRet = csRet.Left(i) + csTemp + csRet.Mid(i + 2);
    }
    while ((i = csRet.Find(_T("&F"))) != -1) {;
        csRet = csRet.Left(i) + csFileName + csRet.Mid(i + 2);
    }
    while ((i = csRet.Find(_T("&P"))) != -1) {
        csRet = csRet.Left(i) + IntToString(iPage) + csRet.Mid(i + 2);
    }
    // 20090915 GHM also look in zTableF\PrtView.cpp CTabPrtView::BuildHeaders
    // for handling of the &I (input data filename) option

    return csRet;
}


/////////////////////////////////////////////////////////////////////////////
//
//                                CFolio::SetDateTime
//
/////////////////////////////////////////////////////////////////////////////

void CFolio::SetDateTime(CIMSAString& cs) {

    int i;
    CIMSAString csTemp;

    while ((i = cs.Find(_T("&D"))) != -1) {;
        csTemp.Date();
        cs = cs.Left(i) + csTemp + cs.Mid(i + 2);
    }
    while ((i = cs.Find(_T("&T"))) != -1) {;
        csTemp.Time();
        cs = cs.Left(i) + csTemp + cs.Mid(i + 2);
    }
    while ((i = cs.Find(_T("&F"))) != -1) {;
        cs = cs.Left(i) + m_csFileName + cs.Mid(i + 2);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                CFolio::SetPage
//
/////////////////////////////////////////////////////////////////////////////

CIMSAString CFolio::SetPage(CIMSAString cs, int iPage) {

    int i;
    CIMSAString csPage = cs;

    while ((i = csPage.Find(_T("&P"))) != -1) {
        csPage = csPage.Left(i) + IntToString(iPage) + csPage.Mid(i + 2);
    }
    return csPage;
}


//***************************************************************************
//
//                             CIMSAPageSetupDlg
//
//***************************************************************************

CIMSAPageSetupDlg::CIMSAPageSetupDlg(CWnd* pParent /*=NULL*/)
    : CDialog(IDD_PAGE_SETUP, pParent)
{
    //{{AFX_DATA_INIT(CIMSAPageSetupDlg)
    m_csFooterCenter = _T("");
    m_csFooterLeft = _T("");
    m_csFooterRight = _T("");
    m_csHeaderCenter = _T("");
    m_csHeaderLeft = _T("");
    m_csHeaderRight = _T("");
    m_fMarginTop = 0.0f;
    m_fMarginBottom = 0.0f;
    m_fMarginLeft = 0.0f;
    m_fMarginRight = 0.0f;
    //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CIMSAPageSetupDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL CIMSAPageSetupDlg::OnInitDialog() {

    CDialog::OnInitDialog();

    m_csHeaderLeft = AfxGetApp()->GetProfileString(_T("Header"), _T("Left"), _T("&F"));
    m_csHeaderCenter = AfxGetApp()->GetProfileString(_T("Header"), _T("Center"), NULL);
    m_csHeaderRight = AfxGetApp()->GetProfileString(_T("Header"), _T("Right"), _T("&D"));

    m_csFooterLeft = AfxGetApp()->GetProfileString(_T("Footer"), _T("Left"), NULL);
    m_csFooterCenter = AfxGetApp()->GetProfileString(_T("Footer"), _T("Center"), _T("- &P -"));
    m_csFooterRight = AfxGetApp()->GetProfileString(_T("Footer"), _T("Right"), NULL);

    m_rectMargins.top = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Top"), _T("0")));
    m_rectMargins.bottom = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Bottom"), _T("0")));
    m_rectMargins.left = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Left"), _T("0")));
    m_rectMargins.right = _ttoi(AfxGetApp()->GetProfileString(_T("Margin"), _T("Right"), _T("0")));

    m_pageMetrics.UpdateUserMargins(m_rectMargins);

    UINT uUnit = GetPrivateProfileInt(_T("Intl"), _T("iMeasure"), 0, _T("WIN.INI"));
    if (uUnit == 0) {
        GetDlgItem(IDC_MARGINS_TITLE)->SetWindowText(_T("Margins (millimeters)"));
        m_fMarginTop = (float) (floor(((m_rectMargins.top * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginBottom = (float) (floor(((m_rectMargins.bottom * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginLeft = (float) (floor(((m_rectMargins.left * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginRight = (float) (floor(((m_rectMargins.right * 2540 / (double) 1440) + .5)) / (double) 100);
    }
    else {
        m_fMarginTop = (float) (floor(((m_rectMargins.top * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginBottom = (float) (floor(((m_rectMargins.bottom * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginLeft = (float) (floor(((m_rectMargins.left * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginRight = (float) (floor(((m_rectMargins.right * 1000 / (double) 1440) + .5)) / (double) 1000);
    }

    UpdateData(FALSE);
    GetDlgItem(IDC_HEADER_LEFT)->SetFocus();
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CIMSAPageSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIMSAPageSetupDlg)
    DDX_Text(pDX, IDC_FOOTER_CENTER, m_csFooterCenter);
    DDX_Text(pDX, IDC_FOOTER_LEFT, m_csFooterLeft);
    DDX_Text(pDX, IDC_FOOTER_RIGHT, m_csFooterRight);
    DDX_Text(pDX, IDC_HEADER_CENTER, m_csHeaderCenter);
    DDX_Text(pDX, IDC_HEADER_LEFT, m_csHeaderLeft);
    DDX_Text(pDX, IDC_HEADER_RIGHT, m_csHeaderRight);
    DDX_Text(pDX, IDC_MARGIN_TOP, m_fMarginTop);
    DDX_Text(pDX, IDC_MARGIN_BOTTOM, m_fMarginBottom);
    DDX_Text(pDX, IDC_MARGIN_LEFT, m_fMarginLeft);
    DDX_Text(pDX, IDC_MARGIN_RIGHT, m_fMarginRight);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIMSAPageSetupDlg, CDialog)
    //{{AFX_MSG_MAP(CIMSAPageSetupDlg)
    ON_BN_CLICKED(IDC_BUTTON_DATE, OnButtonDate)
    ON_BN_CLICKED(IDC_BUTTON_TIME, OnButtonTime)
    ON_BN_CLICKED(IDC_BUTTON_FILE, OnButtonFile)
    ON_BN_CLICKED(IDC_BUTTON_PAGE, OnButtonPage)
    ON_EN_SETFOCUS(IDC_MARGIN_LEFT, OnSetfocusMarginLeft)
    ON_EN_CHANGE(IDC_HEADER_LEFT, OnChangeHeaderLeft)
    ON_EN_CHANGE(IDC_HEADER_CENTER, OnChangeHeaderCenter)
    ON_EN_CHANGE(IDC_HEADER_RIGHT, OnChangeHeaderRight)
    ON_EN_CHANGE(IDC_FOOTER_LEFT, OnChangeFooterLeft)
    ON_EN_CHANGE(IDC_FOOTER_CENTER, OnChangeFooterCenter)
    ON_EN_CHANGE(IDC_FOOTER_RIGHT, OnChangeFooterRight)
    ON_EN_SETFOCUS(IDC_MARGIN_TOP, OnSetfocusMarginTop)
    ON_EN_SETFOCUS(IDC_MARGIN_BOTTOM, OnSetfocusMarginBottom)
    ON_EN_SETFOCUS(IDC_MARGIN_RIGHT, OnSetfocusMarginRight)
    ON_EN_KILLFOCUS(IDC_MARGIN_LEFT, OnKillfocusMarginLeft)
    ON_EN_KILLFOCUS(IDC_MARGIN_RIGHT, OnKillfocusMarginRight)
    ON_EN_KILLFOCUS(IDC_MARGIN_TOP, OnKillfocusMarginTop)
    ON_EN_KILLFOCUS(IDC_MARGIN_BOTTOM, OnKillfocusMarginBottom)
    ON_EN_SETFOCUS(IDC_HEADER_LEFT, OnSetfocusHeaderLeft)
    ON_EN_SETFOCUS(IDC_HEADER_CENTER, OnSetfocusHeaderCenter)
    ON_EN_SETFOCUS(IDC_HEADER_RIGHT, OnSetfocusHeaderRight)
    ON_EN_SETFOCUS(IDC_FOOTER_LEFT, OnSetfocusFooterLeft)
    ON_EN_SETFOCUS(IDC_FOOTER_CENTER, OnSetfocusFooterCenter)
    ON_EN_SETFOCUS(IDC_FOOTER_RIGHT, OnSetfocusFooterRight)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIMSAPageSetupDlg message handlers

void CIMSAPageSetupDlg::OnSetfocusHeadFoot() {

    GetDlgItem(IDC_BUTTON_DATE)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_TIME)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_FILE)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_PAGE)->EnableWindow(TRUE);
}

void CIMSAPageSetupDlg::OnKillfocusHeadFoot() {

    GetDlgItem(IDC_BUTTON_DATE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_TIME)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_FILE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_PAGE)->EnableWindow(FALSE);
}

void CIMSAPageSetupDlg::OnButtonDate() {

    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&D"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CIMSAPageSetupDlg::OnButtonTime() {

    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&T"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CIMSAPageSetupDlg::OnButtonFile() {

    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&F"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CIMSAPageSetupDlg::OnButtonPage() {

    int iBegin, iEnd;
    CEdit* pEdit = (CEdit*) GetDlgItem(m_uLastControl);
    pEdit->GetSel(iBegin, iEnd);
    pEdit->ReplaceSel(_T("&P"));
    pEdit->SetFocus();
    pEdit->SetSel(iBegin + 2, iBegin + 2);
}

void CIMSAPageSetupDlg::OnChangeHeaderLeft() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnChangeHeaderCenter() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnChangeHeaderRight() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnChangeFooterLeft() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnChangeFooterCenter() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnChangeFooterRight() {

    UpdateData(TRUE);
}

void CIMSAPageSetupDlg::OnSetfocusMarginLeft() {

    OnKillfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusMarginTop() {

    OnKillfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusMarginBottom() {

    OnKillfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusMarginRight() {

    OnKillfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnKillfocusMarginTop() {

    UpdateData(TRUE);
    ComputeMetrics();
    UpdateData(FALSE);
}

void CIMSAPageSetupDlg::OnKillfocusMarginBottom() {

    UpdateData(TRUE);
    ComputeMetrics();
    UpdateData(FALSE);
}

void CIMSAPageSetupDlg::OnKillfocusMarginLeft() {

    UpdateData(TRUE);
    ComputeMetrics();
    UpdateData(FALSE);
}

void CIMSAPageSetupDlg::OnKillfocusMarginRight() {

    UpdateData(TRUE);
    ComputeMetrics();
    UpdateData(FALSE);
}

void CIMSAPageSetupDlg::OnOK() {

    CIMSAString csMargin;
    UpdateData(TRUE);

    AfxGetApp()->WriteProfileString(_T("Header"), _T("Left"), m_csHeaderLeft);
    AfxGetApp()->WriteProfileString(_T("Header"), _T("Center"), m_csHeaderCenter);
    AfxGetApp()->WriteProfileString(_T("Header"), _T("Right"), m_csHeaderRight);

    AfxGetApp()->WriteProfileString(_T("Footer"), _T("Left"), m_csFooterLeft);
    AfxGetApp()->WriteProfileString(_T("Footer"), _T("Center"), m_csFooterCenter);
    AfxGetApp()->WriteProfileString(_T("Footer"), _T("Right"), m_csFooterRight);

    ComputeMetrics();

    csMargin.Format(_T("%u"), m_rectMargins.top);
    AfxGetApp()->WriteProfileString(_T("Margin"), _T("Top"), csMargin);

    csMargin.Format(_T("%u"), m_rectMargins.bottom);
    AfxGetApp()->WriteProfileString(_T("Margin"), _T("Bottom"), csMargin);

    csMargin.Format(_T("%u"), m_rectMargins.left);
    AfxGetApp()->WriteProfileString(_T("Margin"), _T("Left"), csMargin);

    csMargin.Format(_T("%u"), m_rectMargins.right);
    AfxGetApp()->WriteProfileString(_T("Margin"), _T("Right"), csMargin);

    CDialog::OnOK();
}

void CIMSAPageSetupDlg::ComputeMetrics(void) {

    UINT uUnit = GetPrivateProfileInt(_T("Intl"), _T("iMeasure"), 0, _T("WIN.INI"));
    if (uUnit == 0) {
        m_rectMargins.top = (int) ((m_fMarginTop * 1440.0 / 25.4) + .5);
        m_rectMargins.bottom = (int) ((m_fMarginBottom * 1440.0 / 25.4) + .5);
        m_rectMargins.left = (int) ((m_fMarginLeft * 1440.0 / 25.4) + .5);
        m_rectMargins.right = (int) ((m_fMarginRight * 1440.0 / 25.4) + .5);
    }
    else {
        m_rectMargins.top = (int) (m_fMarginTop * 1440.0);
        m_rectMargins.bottom = (int) (m_fMarginBottom * 1440.0);
        m_rectMargins.left = (int) (m_fMarginLeft * 1440.0);
        m_rectMargins.right = (int) (m_fMarginRight * 1440.0);
    }

    m_pageMetrics.UpdateUserMargins(m_rectMargins);

    if (uUnit == 0) {
        m_fMarginTop = (float) (floor(((m_rectMargins.top * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginBottom = (float) (floor(((m_rectMargins.bottom * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginLeft = (float) (floor(((m_rectMargins.left * 2540 / (double) 1440) + .5)) / (double) 100);
        m_fMarginRight = (float) (floor(((m_rectMargins.right * 2540 / (double) 1440) + .5)) / (double) 100);
    }
    else {
        m_fMarginTop = (float) (floor(((m_rectMargins.top * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginBottom = (float) (floor(((m_rectMargins.bottom * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginLeft = (float) (floor(((m_rectMargins.left * 1000 / (double) 1440) + .5)) / (double) 1000);
        m_fMarginRight = (float) (floor(((m_rectMargins.right * 1000 / (double) 1440) + .5)) / (double) 1000);
    }
}

void CIMSAPageSetupDlg::OnSetfocusHeaderLeft() {

    m_uLastControl = IDC_HEADER_LEFT;
    OnSetfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusHeaderCenter() {

    m_uLastControl = IDC_HEADER_CENTER;
    OnSetfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusHeaderRight() {

    m_uLastControl = IDC_HEADER_RIGHT;
    OnSetfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusFooterLeft() {

    m_uLastControl = IDC_FOOTER_LEFT;
    OnSetfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusFooterCenter() {

    m_uLastControl = IDC_FOOTER_CENTER;
    OnSetfocusHeadFoot();
}

void CIMSAPageSetupDlg::OnSetfocusFooterRight() {

    m_uLastControl = IDC_FOOTER_RIGHT;
    OnSetfocusHeadFoot();
}
