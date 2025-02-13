#pragma once
//***************************************************************************
//  File name: DDEdit.h
//
//  Description:
//       Header for edit controls classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zDictF/DDGrid.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit
//
/////////////////////////////////////////////////////////////////////////////

class CLabelEdit : public CEdit  {

// Implementation
private:
    CDDGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CLabelEdit (void) : m_pParent(NULL) {}
    void SetParent (CDDGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CLabelEdit)
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
        dc << _T("CLabelEdit");
    }
    #endif
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CNameEdit
//
/////////////////////////////////////////////////////////////////////////////

class CNameEdit : public CEdit  {

// Implementation
private:
    CDDGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CNameEdit (void) : m_pParent(NULL) {}
    void SetParent (CDDGrid* pParent) {
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CNameEdit)
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
        dc << _T("CNameEdit");
    }
    #endif
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CNumEdit
//
/////////////////////////////////////////////////////////////////////////////

class CNumEdit : public CEdit  {

// Implementation
private:
    CDDGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;
    bool        m_bSigned;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CNumEdit (bool bSigned=false) : m_pParent(NULL), m_bSigned(bSigned) {}
    void SetParent (CDDGrid* pParent) {
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CNumEdit)
    afx_msg void OnChar(UINT uChar, UINT uRepCnt, UINT uFlags);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    #ifdef _DEBUG
    virtual void Dump (CDumpContext& dc) const  {
        CObject::Dump(dc);
        dc << _T("CNumEdit");
    }
    #endif
};


/////////////////////////////////////////////////////////////////////////////
//
//                             CFromToEdit
//
/////////////////////////////////////////////////////////////////////////////

class CFromToEdit : public CEdit  {

// Implementation
private:
    CDDGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    CFromToEdit (void) : m_pParent(NULL) {}
    void SetParent (CDDGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CFromToEdit)
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
        dc << _T("CLabelEdit");
    }
    #endif
};


/////////////////////////////////////////////////////////////////////////////
//
//                          CDDComboBox
//
/////////////////////////////////////////////////////////////////////////////

class CDDComboBox : public CComboBox  {

    // Implementation
private:
    CDDGrid*    m_pParent;
    int         m_iRow;
    int         m_iCol;

public:
    CDDComboBox(void) : m_pParent(NULL) {}
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID);
    void SetParent (CDDGrid* pParent) {
        ASSERT_VALID(pParent);
        m_pParent = pParent;
    }
    void SetRowCol (int iRow, int iCol)  { m_iRow = iRow; m_iCol = iCol; }

protected:
    // Generated message map functions
    //{{AFX_MSG(CDDComboBox)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSelchange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    #ifdef _DEBUG
    virtual void Dump (CDumpContext& dc) const  {
        CObject::Dump(dc);
        dc << _T("CDDComboBox");
    }
    #endif
};
