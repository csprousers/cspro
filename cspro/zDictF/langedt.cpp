//***************************************************************************
//  File name: condedt.cpp
//
//  Description:
//       Data Dictionary edit controls implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   bmd     Created for CSPro 2.0
//
//***************************************************************************

#include "StdAfx.h"
#include "langedt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit2
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CLabelEdit2, CEdit)
    //{{AFX_MSG_MAP(CLabelEdit2)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit2::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CLabelEdit2::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CLangGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_MULTILINE | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit2::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit2::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if ((uChar == VK_DOWN || uChar == VK_UP) && m_pParent) {
        m_pParent->EditChange(uChar);
    }
    else  {
        CEdit::OnKeyDown (uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit2::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit2::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if ((uChar == VK_ESCAPE || uChar == VK_RETURN || uChar == VK_TAB) && m_pParent)  {
        m_pParent->EditChange(uChar);
    }
    else if (uChar == 10) {
        if (m_pParent) {
            m_pParent->EditChange(VK_CANCEL);
        }
        else {
            ::MessageBeep(0xFFFFFFFF);
        }
    }
    else {
        CEdit::OnChar(uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit2::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit2::OnSetFocus(CWnd* pOldWnd) {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit2::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit2::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

    CRect rect;
    m_pParent->GetCellRect(m_iCol, m_iRow, &rect);
    CRect rectWnd;
    GetWindowRect(&rectWnd);
    CRect rectEdit;
    GetRect(&rectEdit);
    rectEdit.left = 2;
    rectEdit.right = rectEdit.left + (rect.right - rect.left - 4);
    MoveWindow(&rect, FALSE);
    SetRect(&rectEdit);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CNameEdit2
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNameEdit2, CEdit)
    //{{AFX_MSG_MAP(CNameEdit2)
    ON_WM_GETDLGCODE()
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CNameEdit2::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CLangGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_MULTILINE | ES_UPPERCASE  | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);  // 11 Feb 2003
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::OnGetDlgCode
//
/////////////////////////////////////////////////////////////////////////////

UINT CNameEdit2::OnGetDlgCode() {

    return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_HASSETSEL;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit2::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if ((uChar == VK_DOWN || uChar == VK_UP || uChar == VK_TAB) && m_pParent) {
        ASSERT_VALID(m_pParent);
        m_pParent->EditChange(uChar);
    }
    else  {
        CEdit::OnKeyDown (uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit2::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

    ASSERT_VALID(m_pParent);
    if ((uChar >= _T('A') && uChar <= _T('Z')) || (uChar >= _T('a') && uChar <= _T('z')) || (uChar >= _T('0') && uChar <= _T('9')) ||
             uChar == _T('_') || uChar == VK_BACK || uChar == VK_LEFT || uChar == VK_RIGHT) {
        CEdit::OnChar(uChar, uRepCnt, uFlags);
    }
    else if (uChar == VK_ESCAPE || uChar == VK_RETURN ) {
        if (m_pParent) {
            m_pParent->EditChange(uChar);
        }
        else  {
            CEdit::OnChar(uChar, uRepCnt, uFlags);
        }
    }
    else if (uChar == 10) {
        if (m_pParent) {
            m_pParent->EditChange(VK_CANCEL);
        }
        else {
            ::MessageBeep(0xFFFFFFFF);
        }
    }
    else  {
        ::MessageBeep(0xFFFFFFFF);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit2::OnSetFocus(CWnd* pOldWnd)  {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit2::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit2::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

    CRect rect;
    m_pParent->GetCellRect(m_iCol, m_iRow, &rect);
    CRect rectWnd;
    GetWindowRect(&rectWnd);
    CRect rectEdit;
    GetRect(&rectEdit);
    rectEdit.left = 2;
    rectEdit.right = rectEdit.left + (rect.right - rect.left - 4);
    MoveWindow(&rect, FALSE);
    SetRect(&rectEdit);
    InvalidateRect(NULL);
}


