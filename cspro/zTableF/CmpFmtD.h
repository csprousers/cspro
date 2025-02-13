#pragma once

class CTabView;

// CCompFmtDlg dialog

class CCompFmtDlg : public CDialog
{
    DECLARE_DYNAMIC(CCompFmtDlg)

public:
    CCompFmtDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CCompFmtDlg();

// Dialog Data
    enum { IDD = IDD_FORMAT_OBJECT };
public:
    CArray<CTblBase*,CTblBase*>* m_pArrTblBase;
    CFmt*   m_pFmt;
    CFmt*   m_pDefFmt;
    FMT_ID  m_eGridComp;
    CIMSAString m_sTitle;
    COLORREF    m_colorText;
    COLORREF    m_colorFill;
    bool        m_bDisableHide;
    bool        m_bDisableZeroHide;
    int         m_iSetMultipleFonts;
    CTabView*   m_pTabView;
    int         m_iDecimal4Kludge;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    BOOL m_bShowDefaults; // true when using to set indiv, false when setting defaults
    BOOL m_bHide;
    BOOL m_bCustom;
    BOOL m_bExtendFont;
    BOOL m_bExtendLines;
    BOOL m_bExtendTxtColor;
    BOOL m_bExtendFillColor;
    BOOL m_bExtendIndent;
    BOOL m_bDefaultIndentation;
    afx_msg void OnBnClickedChngfont();
    CComboBox m_BorderL;
    CComboBox m_BorderR;
    CComboBox m_BorderT;
    CComboBox m_BorderB;
    CComboBox m_decimals;
    afx_msg void OnBnClickedButtonTxtcolr();
    afx_msg void OnBnClickedButtonFillcolr();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedReset();
    virtual BOOL OnInitDialog();

    void PrepareLinesControls(LINE eLineDef, LINE eLine , CComboBox* pComboBox);
    void PrepareAlignControls(TAB_HALIGN eHAlignDef, TAB_HALIGN eHAlign , CComboBox* pComboBox);
    void PrepareAlignControls(TAB_VALIGN eVAlignDef, TAB_VALIGN eVAlign , CComboBox* pComboBox);
    void PrepareDecimalCtl();
    void InitMultiStuff();
    void Switch3WayOff();
    void UpdateMultiFmtsOnOk();

    //helper functioms for multifmt
    CFmt* CreateNewFmt(CTblBase* pTblBase);
    void SetHorzAlign(TAB_HALIGN eHAlign);
    void SetVertAlign(TAB_VALIGN eVAlign);
    void SetCustom(CUSTOM custom);
    void SetFillColor(FMT_COLOR fmtColor);
    void SetTextColor(FMT_COLOR fmtColor);
    void SetNumDecimals(NUM_DECIMALS eNumDecimals);
    void SetSpanCells(bool bSpanCells);
    void SetHidden(HIDDEN eHide);
    void SetZeroHidden(bool bHide);
    void SetIndents();
    void SetMultipleFonts();

    void SetLinesExtend();
    void SetFontExtends();
    void SetTextColorExtends();
    void SetFillColorExtends();
    void SetIndentationExtends();
    void SetLines();


    CComboBox m_AlignH;
    CComboBox m_AlignV;

    bool m_bDefTxtColor;
    bool m_bDefFillColor;
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    CString m_sFontDesc;
    CString m_sUnitsR;
    CString m_sUnitsL;
    float m_fIndentLeft;
    float m_fIndentRight;
    afx_msg void OnBnClickedDefaultIndentation();
    BOOL m_bSpanCells;
    void UpdateFontDescription();
    CButton m_HideZero;
    BOOL m_bHideZero;
};
