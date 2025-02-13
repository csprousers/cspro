#pragma once
//***************************************************************************
//  File name: FindDlg.h
//
//  Description:
//       Header for Find Dialog Box
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

class CFindDlg : public CDialog {

// Members
protected:
    CString m_csRecentFindSelection[15];    // 15 most recent find string
    bool    m_bNext;                        // Next or Prev
    CRect   m_rcPos;                        // Last location of find dialog (starts centered)
    CView*  m_pCurrView;                    // View of active dictionary

// Dialog Data
    BOOL    m_bCaseSensitive;
    CString m_csSearchText;

// Construction
public:
    CFindDlg(CWnd* pParent = NULL);

    BOOL Create(void);
    void SetCurrView(CView* pView) { m_pCurrView = pView; }

    void SetNext(bool bNext)        { m_bNext = bNext; }
    bool IsNext(void)               { return m_bNext; }
    BOOL IsCaseSensitive(void)      { return m_bCaseSensitive; }
    CIMSAString GetFindText(void)   { return m_csSearchText;   }

    void EnableButtons(bool);       // Turn the "Find Next" and "Find Prev" buttons on (active) or off (grayed)
    void UpdateHistoryList(void);   // Update recent combo box choices

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFindDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK ();
    virtual void OnCancel ();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CFindDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnEditchangeSearchText();
    afx_msg void OnSelchangeSearchText();
    afx_msg void OnNext();
    afx_msg void OnPrev();
    afx_msg void OnClose();
    // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
