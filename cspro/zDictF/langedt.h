#pragma once
//***************************************************************************
//  File name: langedt.h
//
//  Description:
//       Header for edit controls classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit2
//
/////////////////////////////////////////////////////////////////////////////
#include <zDictF/langgrid.h>
class CLabelEdit2 : public CEdit  {

// Implementation
private:
    CLangGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CLabelEdit2 (void) : m_pParent(NULL) {}
    void SetParent (CLangGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CLabelEdit2)
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
        dc << _T("CLabelEdit2");
    }
    #endif
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CNameEdit2
//
/////////////////////////////////////////////////////////////////////////////

class CNameEdit2 : public CEdit  {

// Implementation
private:
    CLangGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CNameEdit2 (void) : m_pParent(NULL) {}
    void SetParent (CLangGrid* pParent) {
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CNameEdit2)
    afx_msg UINT OnGetDlgCode();
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
        dc << _T("CNameEdit2");
    }
    #endif
};
