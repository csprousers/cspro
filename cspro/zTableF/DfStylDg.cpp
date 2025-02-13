//***************************************************************************
//  File name: DfStylDg.cpp
//
//  Description:
//  Dialog for picking default styles for tables and table elements.
//  Edits values in format registry
//
//***************************************************************************

#include "StdAfx.h"
#include "DfStylDg.h"
#include "TabDoc.h"
#include "TabView.h"

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::CDefaultStylesDlg
// constructor
/////////////////////////////////////////////////////////////////////////////////
CDefaultStylesDlg::CDefaultStylesDlg(CFmtReg& fmtReg,CTabView* pTabView)
: m_fmtReg(fmtReg),
  m_bChanged(false),
  CTreePropertiesDlg(_T("Preferences"))
{
    m_pTabView = pTabView;
    // object formats page
    m_objFormatPage.Create(IDD_OBJECT_FORMATS_PAGE, this);
    AddPage(&m_objFormatPage, _T("Object formats"));

    m_aObjFmtDlgs.SetSize(18);

    // spanner format page
    AddObjectFormatPage(FMT_ID_SPANNER, m_aObjFmtDlgs[0], &m_objFormatPage);

    // column head format page
    AddObjectFormatPage(FMT_ID_COLHEAD, m_aObjFmtDlgs[1], &m_objFormatPage);

    // caption format page
    AddObjectFormatPage(FMT_ID_CAPTION, m_aObjFmtDlgs[2], &m_objFormatPage);

    // stub format page
    AddObjectFormatPage(FMT_ID_STUB, m_aObjFmtDlgs[3], &m_objFormatPage);

    // data cell format page
    AddObjectFormatPage(FMT_ID_DATACELL, m_aObjFmtDlgs[4], &m_objFormatPage);

    // left header format page
    AddObjectFormatPage(FMT_ID_HEADER_LEFT, m_aObjFmtDlgs[5], &m_objFormatPage);

    // center header format page
    AddObjectFormatPage(FMT_ID_HEADER_CENTER, m_aObjFmtDlgs[6], &m_objFormatPage);

    // right header format page
    AddObjectFormatPage(FMT_ID_HEADER_RIGHT, m_aObjFmtDlgs[7], &m_objFormatPage);

    // left footer format page
    AddObjectFormatPage(FMT_ID_FOOTER_LEFT, m_aObjFmtDlgs[8], &m_objFormatPage);

    // center footer format page
    AddObjectFormatPage(FMT_ID_FOOTER_CENTER, m_aObjFmtDlgs[9], &m_objFormatPage);

    // right footer format page
    AddObjectFormatPage(FMT_ID_FOOTER_RIGHT, m_aObjFmtDlgs[10], &m_objFormatPage);

    // title format page
    AddObjectFormatPage(FMT_ID_TITLE, m_aObjFmtDlgs[11], &m_objFormatPage);

    // subtitle format page
    AddObjectFormatPage(FMT_ID_SUBTITLE, m_aObjFmtDlgs[12], &m_objFormatPage);

    // page note format page
    AddObjectFormatPage(FMT_ID_PAGENOTE, m_aObjFmtDlgs[13], &m_objFormatPage);

    // end note format page
    AddObjectFormatPage(FMT_ID_ENDNOTE, m_aObjFmtDlgs[14], &m_objFormatPage);

    // stub head format page
    AddObjectFormatPage(FMT_ID_STUBHEAD, m_aObjFmtDlgs[15], &m_objFormatPage);

    // secondary stub head format page
    AddObjectFormatPage(FMT_ID_STUBHEAD_SEC, m_aObjFmtDlgs[16], &m_objFormatPage);

    // area caption format page
    AddObjectFormatPage(FMT_ID_AREA_CAPTION, m_aObjFmtDlgs[17], &m_objFormatPage);

    // Tally format page
    m_varTallyFmtDlgR.m_pDefaultFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(FMT_ID_TALLY_ROW));
    m_varTallyFmtDlgR.m_pFmtSelected= new CTallyFmt(*m_varTallyFmtDlgR.m_pDefaultFmt);
    m_varTallyFmtDlgR.m_dVarMax = DBL_MAX;
    m_varTallyFmtDlgR.m_dVarMin = DBL_MIN;
    m_varTallyFmtDlgR.m_dVarMaxDefault = 100;
    m_varTallyFmtDlgR.m_dVarMinDefault = 100;
    m_varTallyFmtDlgR.m_sDefaultPropRange = _T("1");
    m_varTallyFmtDlgR.Create(CTallyVarDlg::IDD, this);

    AddPage(&m_varTallyFmtDlgR, _T("Tally Attributes (Row)"));

    m_varTallyFmtDlgC.m_pDefaultFmt = DYNAMIC_DOWNCAST(CTallyFmt,m_fmtReg.GetFmt(FMT_ID_TALLY_COL));
    m_varTallyFmtDlgC.m_pFmtSelected= new CTallyFmt(*m_varTallyFmtDlgC.m_pDefaultFmt);
    m_varTallyFmtDlgC.Create(CTallyVarDlg::IDD, this);
    m_varTallyFmtDlgC.m_dVarMax = DBL_MAX;
    m_varTallyFmtDlgC.m_dVarMin = DBL_MIN;
    m_varTallyFmtDlgC.m_dVarMaxDefault = 100;
    m_varTallyFmtDlgC.m_dVarMinDefault = 100;
    m_varTallyFmtDlgC.m_sDefaultPropRange = _T("1");
    AddPage(&m_varTallyFmtDlgC, _T("Tally Attributes (Column)"));

    // Application format page
    m_appFmtPage.m_pFmt = new CTabSetFmt(*DYNAMIC_DOWNCAST(CTabSetFmt,m_fmtReg.GetFmt(FMT_ID_TABSET)));
    m_appFmtPage.Create(CAppFmtDlg::IDD, this);
    AddPage(&m_appFmtPage, _T("Application Format"));

    // table format page
    m_tblFmtDlg.m_pDefFmt = DYNAMIC_DOWNCAST(CTblFmt,m_fmtReg.GetFmt(FMT_ID_TABLE));
    m_tblFmtDlg.m_pFmt = new CTblFmt(*m_tblFmtDlg.m_pDefFmt);
    m_tblFmtDlg.m_bShowDefaults = FALSE;
    m_tblFmtDlg.Create(CTblFmtDlg::IDD, this);
    m_tblFmtDlg.GetDlgItem(IDC_RESET)->ShowWindow(SW_HIDE);
    AddPage(&m_tblFmtDlg, _T("Table Format"));

    // table print format page
    m_tblPrintFmtDlg.m_pDefaultFmtTblPrint=DYNAMIC_DOWNCAST(CTblPrintFmt, m_fmtReg.GetFmt(FMT_ID_TBLPRINT));
    m_tblPrintFmtDlg.m_pFmtTblPrint= new CTblPrintFmt(*m_tblPrintFmtDlg.m_pDefaultFmtTblPrint);
    ::ZeroMemory(&m_tblPrintFmtDlg.m_lfHeader, sizeof(LOGFONT));
    (DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_LEFT)))->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfHeader);
    ::ZeroMemory(&m_tblPrintFmtDlg.m_lfDefaultHeader, sizeof(LOGFONT));
    (DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_LEFT)))->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfDefaultHeader);
    ::ZeroMemory(&m_tblPrintFmtDlg.m_lfFooter, sizeof(LOGFONT));
    (DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_LEFT)))->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfFooter);
    ::ZeroMemory(&m_tblPrintFmtDlg.m_lfDefaultFooter, sizeof(LOGFONT));
    (DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_LEFT)))->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfDefaultFooter);
    memcpy(&m_lfLastPrintHdr, &m_tblPrintFmtDlg.m_lfHeader, sizeof(LOGFONT));
    memcpy(&m_lfLastPrintFtr, &m_tblPrintFmtDlg.m_lfFooter, sizeof(LOGFONT));
    m_tblPrintFmtDlg.m_bShowDefaults = FALSE;

    CFmt* pLeftHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_LEFT));
    m_tblPrintFmtDlg.m_sHeaderLeft = pLeftHeaderFmt->GetCustomText();
    CFmt* pCenterHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_CENTER));
    m_tblPrintFmtDlg.m_sHeaderCenter = pCenterHeaderFmt->GetCustomText();
    CFmt* pRightHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_RIGHT));
    m_tblPrintFmtDlg.m_sHeaderRight = pRightHeaderFmt->GetCustomText();
    CFmt* pLeftFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_LEFT));
    m_tblPrintFmtDlg.m_sFooterLeft = pLeftFooterFmt->GetCustomText();
    CFmt* pCenterFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_CENTER));
    m_tblPrintFmtDlg.m_sFooterCenter = pCenterFooterFmt->GetCustomText();
    CFmt* pRightFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_RIGHT));
    m_tblPrintFmtDlg.m_sFooterRight = pRightFooterFmt->GetCustomText();

    m_tblPrintFmtDlg.Create(CTblPrintFmtDlg::IDD, this);
    AddPage(&m_tblPrintFmtDlg, _T("Table Print Format"));

    SetPage(&m_aObjFmtDlgs[0]);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::~CDefaultStylesDlg
// destructor
/////////////////////////////////////////////////////////////////////////////////
CDefaultStylesDlg::~CDefaultStylesDlg()
{
    // cleanup format copies
    for (int i = 0; i < m_aObjFmtDlgs.GetSize(); ++i) {
        SAFE_DELETE(m_aObjFmtDlgs[i].m_pFmt);
    }
    SAFE_DELETE(m_appFmtPage.m_pFmt);
    SAFE_DELETE(m_varTallyFmtDlgR.m_pFmtSelected);
    SAFE_DELETE(m_varTallyFmtDlgC.m_pFmtSelected);
    SAFE_DELETE(m_tblFmtDlg.m_pFmt);
    SAFE_DELETE(m_tblPrintFmtDlg.m_pFmtTblPrint);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::OnPageChange
// called when page changes for extra handling
/////////////////////////////////////////////////////////////////////////////////
void CDefaultStylesDlg::OnPageChange(CDialog* pOldPage, CDialog* pNewPage)
{
    if (pOldPage == &m_tblPrintFmtDlg) {
        // left table print fmt page - if the header or footer fonts were changed
        // while viewing the page, we need to synch up the header and footer format pages
        // single font for all headers and single font for all footers are applied to
        // the three footer and three header dialogs
        if (memcmp(&m_tblPrintFmtDlg.m_lfHeader, &m_lfLastPrintHdr, sizeof(LOGFONT))!=0) {
            FindObjectFormatPage(FMT_ID_HEADER_LEFT)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfHeader);
            FindObjectFormatPage(FMT_ID_HEADER_RIGHT)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfHeader);
            FindObjectFormatPage(FMT_ID_HEADER_CENTER)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfHeader);
            memcpy(&m_lfLastPrintHdr, &m_tblPrintFmtDlg.m_lfHeader, sizeof(LOGFONT));
            FindObjectFormatPage(FMT_ID_HEADER_LEFT)->UpdateData(FALSE);
            FindObjectFormatPage(FMT_ID_HEADER_CENTER)->UpdateData(FALSE);
            FindObjectFormatPage(FMT_ID_HEADER_RIGHT)->UpdateData(FALSE);
        }
        if (memcmp(&m_tblPrintFmtDlg.m_lfFooter, &m_lfLastPrintFtr, sizeof(LOGFONT))!=0) {
            FindObjectFormatPage(FMT_ID_FOOTER_LEFT)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfFooter);
            FindObjectFormatPage(FMT_ID_FOOTER_CENTER)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfFooter);
            FindObjectFormatPage(FMT_ID_FOOTER_RIGHT)->m_pFmt->SetFont(&m_tblPrintFmtDlg.m_lfFooter);
            memcpy(&m_lfLastPrintFtr, &m_tblPrintFmtDlg.m_lfFooter, sizeof(LOGFONT));
            FindObjectFormatPage(FMT_ID_FOOTER_LEFT)->UpdateData(FALSE);
            FindObjectFormatPage(FMT_ID_FOOTER_CENTER)->UpdateData(FALSE);
            FindObjectFormatPage(FMT_ID_FOOTER_RIGHT)->UpdateData(FALSE);
        }
    }
    else if (pOldPage && pOldPage->IsKindOf(RUNTIME_CLASS(CCompFmtDlg))) {
        // check if we are leaving the left header or left footer format page
        // if those change we need to synch up the header and footer fonts
        // for the table printf format page.
        CCompFmtDlg* pFmtDlg = DYNAMIC_DOWNCAST(CCompFmtDlg, pOldPage);
        ASSERT_VALID(pFmtDlg);
        switch (pFmtDlg->m_eGridComp) {
            case FMT_ID_HEADER_LEFT:
                pFmtDlg->m_pFmt->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfHeader);
                memcpy(&m_lfLastPrintHdr, &m_tblPrintFmtDlg.m_lfHeader, sizeof(LOGFONT));
                break;
            case FMT_ID_FOOTER_LEFT:
                pFmtDlg->m_pFmt->GetFont()->GetLogFont(&m_tblPrintFmtDlg.m_lfFooter);
                memcpy(&m_lfLastPrintFtr, &m_tblPrintFmtDlg.m_lfFooter, sizeof(LOGFONT));
                break;
        }
        m_tblPrintFmtDlg.UpdateData(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::OnOK
// called when dialog dismissed with OK
/////////////////////////////////////////////////////////////////////////////////
void CDefaultStylesDlg::OnOK()
{
    CTreePropertiesDlg::OnOK(); // this calls OnOK() for each of the pages

    // check object format dlgs for change
    for (int i = 0; i < m_aObjFmtDlgs.GetSize(); ++i) {
        CCompFmtDlg& dlg = m_aObjFmtDlgs[i];
        if (!dlg.m_pDefFmt->Compare(*dlg.m_pFmt)) {
            m_bChanged = true;
            dlg.m_pDefFmt->Assign(*dlg.m_pFmt);
        }
    }

    // check tally for change
    if (*m_varTallyFmtDlgR.m_pDefaultFmt != *m_varTallyFmtDlgR.m_pFmtSelected) {
        m_bChanged = true;
        *m_varTallyFmtDlgR.m_pDefaultFmt = *m_varTallyFmtDlgR.m_pFmtSelected;
    }

     // check tally for change
    if (*m_varTallyFmtDlgC.m_pDefaultFmt != *m_varTallyFmtDlgC.m_pFmtSelected) {
        m_bChanged = true;
        *m_varTallyFmtDlgC.m_pDefaultFmt = *m_varTallyFmtDlgC.m_pFmtSelected;
    }
    // check app for change
    CTabSetFmt* pDefAppFmt = DYNAMIC_DOWNCAST(CTabSetFmt,m_fmtReg.GetFmt(FMT_ID_TABSET));
    if (*m_appFmtPage.m_pFmt != *pDefAppFmt) {
        m_bChanged = true;

        // all CFmts in registry have copy of units - need to synch them when units change
        if (m_appFmtPage.m_pFmt->GetUnits() != pDefAppFmt->GetUnits()) {
            m_fmtReg.SetUnits(m_appFmtPage.m_pFmt->GetUnits());
        }
        *pDefAppFmt = *m_appFmtPage.m_pFmt;
    }

    // check table for change
    if (*m_tblFmtDlg.m_pDefFmt != *m_tblFmtDlg.m_pFmt) {
        m_bChanged = true;
        *m_tblFmtDlg.m_pDefFmt = *m_tblFmtDlg.m_pFmt;
    }

    // check table print for changes
    bool bTblPrintChanged=(m_tblPrintFmtDlg.m_pFmtTblPrint->GetHeaderFrequency()!=m_tblPrintFmtDlg.m_eHeaderFrequency
        || m_tblPrintFmtDlg.m_pFmtTblPrint->GetTblLayout()!=m_tblPrintFmtDlg.m_eTblLayout
        || m_tblPrintFmtDlg.m_pFmtTblPrint->GetStartPage()!=m_tblPrintFmtDlg.m_iStartPage
        || m_tblPrintFmtDlg.m_pFmtTblPrint->GetPageMargin()!=m_tblPrintFmtDlg.m_rcPageMargin);
    if (bTblPrintChanged) {
        m_bChanged = true;
        // synch from table print dlg - unlike others this one doesn't synch itself
        m_tblPrintFmtDlg.m_pDefaultFmtTblPrint->SetHeaderFrequency(m_tblPrintFmtDlg.m_eHeaderFrequency);
        m_tblPrintFmtDlg.m_pDefaultFmtTblPrint->SetTblLayout(m_tblPrintFmtDlg.m_eTblLayout);
        m_tblPrintFmtDlg.m_pDefaultFmtTblPrint->SetStartPage(m_tblPrintFmtDlg.m_iStartPage);
        m_tblPrintFmtDlg.m_pDefaultFmtTblPrint->SetPageMargin(m_tblPrintFmtDlg.m_rcPageMargin);
    }

    // check header and footer text
    CFmt* pLeftHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_LEFT));
    CFmt* pCenterHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_CENTER));
    CFmt* pRightHeaderFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_HEADER_RIGHT));
    CFmt* pLeftFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_LEFT));
    CFmt* pCenterFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_CENTER));
    CFmt* pRightFooterFmt = DYNAMIC_DOWNCAST(CFmt, m_fmtReg.GetFmt(FMT_ID_FOOTER_RIGHT));
    if (pLeftHeaderFmt->GetCustomText() != m_tblPrintFmtDlg.m_sHeaderLeft) {
        m_bChanged = true;
        pLeftHeaderFmt->SetCustomText(m_tblPrintFmtDlg.m_sHeaderLeft);
    }
    if (pCenterHeaderFmt->GetCustomText() != m_tblPrintFmtDlg.m_sHeaderCenter) {
        m_bChanged = true;
        pCenterHeaderFmt->SetCustomText(m_tblPrintFmtDlg.m_sHeaderCenter);
    }
    if (pRightHeaderFmt->GetCustomText() != m_tblPrintFmtDlg.m_sHeaderRight) {
        m_bChanged = true;
        pRightHeaderFmt->SetCustomText(m_tblPrintFmtDlg.m_sHeaderRight);
    }
    if (pLeftFooterFmt->GetCustomText() != m_tblPrintFmtDlg.m_sFooterLeft) {
        m_bChanged = true;
        pLeftFooterFmt->SetCustomText(m_tblPrintFmtDlg.m_sFooterLeft);
    }
    if (pCenterFooterFmt->GetCustomText() != m_tblPrintFmtDlg.m_sFooterCenter) {
        m_bChanged = true;
        pCenterFooterFmt->SetCustomText(m_tblPrintFmtDlg.m_sFooterCenter);
    }
    if (pRightFooterFmt->GetCustomText() != m_tblPrintFmtDlg.m_sFooterRight) {
        m_bChanged = true;
        pRightFooterFmt->SetCustomText(m_tblPrintFmtDlg.m_sFooterRight);
    }
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    CTabSet* pTabSet = pDoc->GetTableSpec();
    if(m_varTallyFmtDlgR.m_pDefaultFmt->HasPercents() && m_varTallyFmtDlgC.m_pDefaultFmt->HasPercents()){
        //We are preferences
        AfxMessageBox(_T("Cannot have percents in both row and column Tally formats.You have to set percent type to None"));
        return;
    }
    // don't check header and footer format in tbl print dlg for changes - they will get picked
    // up by the header format object dlg checks above due to OnPageChange synch between the two
}

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::AddObjectFormatPage
// Helper function for adding pages for CFmt (CCompFmtDlg) objects.
/////////////////////////////////////////////////////////////////////////////////
void CDefaultStylesDlg::AddObjectFormatPage(FMT_ID fmtId,
                                         CCompFmtDlg& objFmtdlg,
                                         CDialog* pParentPage)
{
    objFmtdlg.m_pDefFmt = DYNAMIC_DOWNCAST(CFmt,m_fmtReg.GetFmt(fmtId));
    objFmtdlg.m_pFmt = DYNAMIC_DOWNCAST(CFmt, objFmtdlg.m_pDefFmt->Clone());
    objFmtdlg.m_eGridComp = fmtId;
    objFmtdlg.m_bShowDefaults = FALSE;
    objFmtdlg.Create(CCompFmtDlg::IDD, this);
    objFmtdlg.GetDlgItem(IDC_RESET)->ShowWindow(SW_HIDE);
    AddPage(&objFmtdlg, GetFormatString(fmtId), pParentPage);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CDefaultStylesDlg::FindObjectFormatPage
// Return the page for a given format object.
/////////////////////////////////////////////////////////////////////////////////
CCompFmtDlg* CDefaultStylesDlg::FindObjectFormatPage(FMT_ID fmtId)
{
    for (int i = 0; i < m_aObjFmtDlgs.GetSize(); ++i) {
        if (m_aObjFmtDlgs[i].m_eGridComp == fmtId) {
            return &m_aObjFmtDlgs[i];
        }
    }

    return NULL;
}
