#pragma once

#include <zTableF/zTableF.h>

class CTblOb;
class CFmtReg;

// CTblPrintFmtDlg dialog

class CLASS_DECL_ZTABLEF CTblPrintFmtDlg : public CDialog
{
    DECLARE_DYNAMIC(CTblPrintFmtDlg)

// Attributes
public:
    TBL_LAYOUT                      m_eTblLayout;           // table layout (default, left std, left facing, both std, both facing)
    int                             m_iStartPage;           // starting page number, START_PAGE_DEFAULT (-1) means use default
    CRect                           m_rcPageMargin;         // page margins (in TWIPS; thus 1 inch=1440)
    HEADER_FREQUENCY                m_eHeaderFrequency;     // header frequency (default, top of each table only, top of each page, none)
    UNITS                           m_eUnits;
    BOOL                            m_bShowDefaults;        // show default values in combo boxes (set to false when editing defaults)

public:
    // SET THESE BEFORE CALLING DOMODAL!
    CTblPrintFmt*       m_pFmtTblPrint;
    CTblPrintFmt*       m_pDefaultFmtTblPrint;
    LOGFONT             m_lfHeader;           // zeromemory if using default
    LOGFONT             m_lfFooter;           // zeromemory if using default
    LOGFONT             m_lfDefaultHeader;    // must be valid
    LOGFONT             m_lfDefaultFooter;    // must be valid
    bool                m_bDefaultHeaderFont;
    bool                m_bDefaultFooterFont;
    CIMSAString         m_sHeaderLeft;
    CIMSAString         m_sHeaderCenter;
    CIMSAString         m_sHeaderRight;
    CIMSAString         m_sFooterLeft;
    CIMSAString         m_sFooterCenter;
    CIMSAString         m_sFooterRight;

public:
    CTblPrintFmtDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTblPrintFmtDlg();

// Dialog Data
    enum { IDD = IDD_FORMAT_PRINT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
private:
    int           m_iTblLayout;
    int           m_iHeaderFrequency;
    float         m_fPageMarginLeft;
    float         m_fPageMarginTop;
    float         m_fPageMarginRight;
    float         m_fPageMarginBottom;
    CIMSAString   m_sUnits1;
    CIMSAString   m_sUnits2;
    CIMSAString   m_sUnits3;
    CIMSAString   m_sUnits4;
    CIMSAString   m_sStartPageCtrl;
    UINT          m_uLastFolioControl;   // for inserting date/time/page/file into header and footer strings
    UINT          m_uLastMarginControl;  // for handling "make even" button

// Messages and overrides
public:
    virtual BOOL OnInitDialog();
    CComboBox m_comboHeaderFrequency;
    CComboBox m_comboTblLayout;
    CComboBox m_comboMultiPage;
    CSpinButtonCtrl m_spinStartPage;
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedButtonDate();
    afx_msg void OnBnClickedButtonTime();
    afx_msg void OnBnClickedButtonFile();
    afx_msg void OnBnClickedButtonInputFile();
    afx_msg void OnBnClickedButtonPage();
    afx_msg void OnEnSetfocusHeaderLeft();
    afx_msg void OnEnSetfocusHeaderCenter();
    afx_msg void OnEnSetfocusHeaderRight();
    afx_msg void OnEnSetfocusFooterLeft();
    afx_msg void OnEnSetfocusFooterCenter();
    afx_msg void OnEnSetfocusFooterRight();
    afx_msg void OnBnClickedHeaderFont();
    afx_msg void OnBnClickedFooterFont();
    afx_msg void OnBnClickedMakeEqual();
    afx_msg void OnEnChangePageMarginBottom();
    afx_msg void OnEnChangePageMarginLeft();
    afx_msg void OnEnChangePageMarginRight();
    afx_msg void OnEnChangePageMarginTop();
    afx_msg void OnDeltaposStartpgSpin(NMHDR* pNMHDR, LRESULT* pResult);
};


// helper functions for intializing and checking for change in folio text (shared between
// CTabView and CPrintView so easiest to put them here).
CLASS_DECL_ZTABLEF void SetupFolioText(CTblOb* pFolioObj, FMT_ID id, CIMSAString& sTxtToSet, CFmtReg* pFmtReg);
CLASS_DECL_ZTABLEF bool FolioTextNeedsUpdate(CTblOb* pFolioObj, FMT_ID id, const CIMSAString& sNewTxt, CFmtReg* pFmtReg);
CLASS_DECL_ZTABLEF bool UpdateFolioText(CTblOb* pFolioObj, FMT_ID id, const CIMSAString& sNewTxt, CFmtReg* pFmtReg);
