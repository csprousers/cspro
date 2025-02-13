#pragma once

/////////////////////////////////////////////////////////////////////////////
// CFFOptionsDlg dialog

#include <zAppO/Application.h>


class CFFOptionsDlg : public CDialog
{
// Construction
public:
    CFFOptionsDlg(CWnd* pParent, CDEFormFile* pFormFile );

    CDEFormFile*    m_pFormFile;

// Dialog Data
    //{{AFX_DATA(CFFOptionsDlg)
    enum { IDD = IDD_DE_OPTIONS };
    int     m_iPath;
    BOOL    m_bAskOpID;
    BOOL    m_bShowEndCaseMsg;
    BOOL    m_bAllowPartialSave;
    UINT    m_iVerifyFreq;
    int     m_iVerifyStart;
    BOOL    m_bRandom;
    BOOL    m_bUseQuestionText;
    BOOL    m_bShowRefusals;
    BOOL    m_bCenterForms; // 20100422
    BOOL    m_bDecimalComma; // 20120306
    BOOL    m_bRTLRosters;
    CComboBox m_comboCaseTree;
    CaseTreeType m_caseTreeType;
    BOOL    m_bAutoAdvanceOnSelection;
    BOOL    m_bDisplayCodesAlongsideLabels;
    BOOL    m_bShowFieldLabels;
    BOOL    m_bShowErrorMessageNumbers;
    BOOL    m_bComboBoxShowOnlyDiscreteValues;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFFOptionsDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CFFOptionsDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnRndstart();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
