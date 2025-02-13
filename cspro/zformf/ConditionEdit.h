#pragma once

//***************************************************************************
//
//  Description:
//       Header for edit controls classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zformf/ConditionGrid.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit1
//
/////////////////////////////////////////////////////////////////////////////

class CLabelEdit1 : public CEdit
{
// Implementation
private:
    CCondGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CLabelEdit1 (void) : m_pParent(NULL) {}
    void SetParent (CCondGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CLabelEdit1)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    #ifdef _DEBUG
    virtual void Dump (CDumpContext& dc) const  {
        CObject::Dump(dc);
        dc << _T("CLabelEdit1");
    }
    #endif
    afx_msg void OnKillFocus(CWnd* pNewWnd);
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CFromToEdit1
//
/////////////////////////////////////////////////////////////////////////////

class CFromToEdit1 : public CEdit
{
// Implementation
private:
    CCondGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CFromToEdit1 (void) : m_pParent(NULL) {}
    void SetParent (CCondGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CFromToEdit1)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    #ifdef _DEBUG
    virtual void Dump (CDumpContext& dc) const  {
        CObject::Dump(dc);
        dc << _T("CLabelEdit1");
    }
    #endif
    afx_msg void OnKillFocus(CWnd* pNewWnd);
};
