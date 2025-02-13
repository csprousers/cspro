#pragma once

// UGEdit.h : header file
//

#include <zGridO/zGridO.h>

/////////////////////////////////////////////////////////////////////////////
// CUGMaskedEdit window

class CUGCtrl;

class CLASS_DECL_ZGRIDO CUGMaskedEdit : public CEdit
{
// Construction
public:
    CUGMaskedEdit();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGMaskedEdit)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGMaskedEdit();


    // Generated message map functions
protected:
    //{{AFX_MSG(CUGMaskedEdit)
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnUpdate();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()


protected:

    int         m_MaskKeyInProgress;

    int         m_useMask;
    csprochar       m_blankChar;
    BOOL        m_storeLiterals;
    csprochar*      m_maskChars;

    CString     m_origMask;
    CString     m_mask;
    CString     m_literals;

    int  MaskKeyStrokes(UINT *vKey);
    void ShowInvalidEntryMessage();


    BOOL m_cancel;
    BOOL m_continueFlag;
    int  m_continueCol;
    int  m_continueRow;

public:

    //pointer to the main class
    CUGCtrl *   m_ctrl;

    int SetMask(LPCTSTR string);
    int MaskString(CString *string);
};
/////////////////////////////////////////////////////////////////////////////
