#pragma once
//***************************************************************************
//  File name: FindDlg.h
//
//  Description:
//       Interface for the CFindDlg class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//
//***************************************************************************

#define  SEARCH_FORWARD        1
#define  SEARCH_BACKWARD       2

#define HISTORY_SIZE 15

class CFindDlg : public CDialog
{
// Construction
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    CFindDlg (CWnd* pParent = NULL); // standard constructor
    ~CFindDlg ();
    BOOL Create ();
    void SetCurrView (CView*  pV)   { m_pCurrView = pV;  }

// Dialog Data
    //{{AFX_DATA(CFindDlg)
    enum { IDD = IDD_DLG_FIND };
    BOOL    m_bCaseSensitive;
    CString m_csSearchText;
    //}}AFX_DATA


// Implementation
private:
    void    EnableButtons (BOOL);          // turn the "Find Next" and "Find Prev" buttons on (active) or off (grayed)

    CString m_csRecentFindSelection[HISTORY_SIZE];   // 15 recent find texts
    UINT    m_uDirection;

public:
    CView* m_pCurrView;

    size_t  GetFindLen (void)      { return _tcslen (m_csSearchText);   }
    void UpdateHistoryList (void);  // recent combo box choices
    void SetDirection (UINT uD)    { m_uDirection = uD;  }
    UINT GetDirection (void)       { return m_uDirection; }
    const CString& GetCurrFindSel(void) const { return m_csSearchText; }   // csc 4 jan 04


protected:
    CToolTipCtrl m_tooltip;
    virtual void DoDataExchange(CDataExchange* pDX);       // DDX/DDV support
    virtual void OnOK ();
    virtual void OnCancel ();

    // Generated message map functions
    //{{AFX_MSG(CFindDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnPrev();
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
