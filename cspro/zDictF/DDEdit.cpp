//***************************************************************************
//  File name: DDEdit.cpp
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CLabelEdit, CEdit)
    //{{AFX_MSG_MAP(CLabelEdit)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CLabelEdit::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CDDGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_MULTILINE | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if ((uChar == VK_DOWN || uChar == VK_UP) && m_pParent) {
        m_pParent->EditChange(uChar);
    }
    else  {
        CEdit::OnKeyDown (uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

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
//                        CLabelEdit::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit::OnSetFocus(CWnd* pOldWnd) {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

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
//                            CNameEdit
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNameEdit, CEdit)
    //{{AFX_MSG_MAP(CNameEdit)
    ON_WM_GETDLGCODE()
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CNameEdit::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CDDGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_MULTILINE | ES_UPPERCASE  | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);  // 11 Feb 2003
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit::OnGetDlgCode
//
/////////////////////////////////////////////////////////////////////////////

UINT CNameEdit::OnGetDlgCode() {

    return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_HASSETSEL;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

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
//                        CNameEdit::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

    ASSERT_VALID(m_pParent);
    if ((uChar >= _T('A') && uChar <= 'Z') || (uChar >= _T('a') && uChar <= 'z') || (uChar >= _T('0') && uChar <= '9') ||
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
    else if( uChar == VK_TAB ) { // GHM 20120229 added VK_TAB to get rid of annoying beep
    }
    else  {
        ::MessageBeep(0xFFFFFFFF);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit::OnSetFocus(CWnd* pOldWnd)  {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNameEdit::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CNameEdit::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

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
//                             CNumEdit
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNumEdit, CEdit)
    //{{AFX_MSG_MAP(CNumEdit)
    ON_WM_CHAR()
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CNumEdit::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CNumEdit::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CDDGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_RIGHT | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);  // BMD 11 Feb 2003
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNumEdit::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CNumEdit::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags)  {

    if ((uChar == VK_DOWN || uChar == VK_UP) && m_pParent)  {
        m_pParent->EditChange(uChar);
    }
    else  {
        CEdit::OnKeyDown (uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNumEdit::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CNumEdit::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

    BOOL bValidChar = (uChar >= _T('0') && uChar <= '9');
    if (m_bSigned)  {
        bValidChar |= (uChar==PLUS || uChar==HYPHEN);
    }
    if (bValidChar || uChar == VK_BACK) {
        CEdit::OnChar(uChar, uRepCnt, uFlags);
    }
    else if (uChar == VK_ESCAPE || uChar == VK_RETURN || uChar == VK_TAB || uChar == VK_UP || uChar == VK_DOWN) {
        if (m_pParent)  {
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
    else {
        ::MessageBeep(0xFFFFFFFF);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNumEdit::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CNumEdit::OnSetFocus(CWnd* pOldWnd)  {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CNumEdit::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CNumEdit::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

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
//                             CFromToEdit
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CFromToEdit, CEdit)
    //{{AFX_MSG_MAP(CFromToEdit)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CFromToEdit::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFromToEdit::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID) {

    m_pParent = (CDDGrid*) pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFromToEdit::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CFromToEdit::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if ((uChar == VK_DOWN || uChar == VK_UP) && m_pParent) {
        m_pParent->EditChange(uChar);
    }
    else {
        CEdit::OnKeyDown (uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFromToEdit::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CFromToEdit::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

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
//                        CFromToEdit::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CFromToEdit::OnSetFocus(CWnd* pOldWnd) {

    m_pParent->m_iEditCol = m_iCol;
    SetSel(0, -1);
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFromToEdit::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CFromToEdit::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

    CRect rect;
    m_pParent->GetCellRect(m_iCol, m_iRow, &rect);
    InvalidateRect(NULL);
    MoveWindow(&rect, FALSE);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDDComboBox
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CDDComboBox, CComboBox)
    //{{AFX_MSG_MAP(CDDComboBox)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
    ON_WM_CHAR()
    ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDComboBox::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDDComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID)  {

    m_pParent = (CDDGrid*) pParent;

    int nReturn = CComboBox::Create(dwStyle, rect, ((CUGCtrl*) pParent)->m_CUGGrid, uID);
    if (nReturn == 0) {
        ASSERT(FALSE);
    }
    return nReturn;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDComboBox::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CDDComboBox::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags) {

    switch(uChar)  {
    case VK_UP:
        if (GetDroppedState())  {
            // list box is visible (dropped, quoi)
            CComboBox::OnKeyDown(uChar, uRepCnt, uFlags);
        }
        else  {
            // list box is *not* dropped
            m_pParent->EditChange(uChar);
        }
        break;
    case VK_DOWN:
        if (GetDroppedState())  {
            // list box is visible (dropped, quoi)
            CComboBox::OnKeyDown(uChar, uRepCnt, uFlags);
        }
        else  {
            // list box is *not* dropped
            CComboBox::ShowDropDown();
        }
        break;
    case VK_ESCAPE:
    case VK_RETURN:
    case VK_TAB:
        if (m_pParent)  {
            m_pParent->EditChange(uChar);
        }
        else  {
            CComboBox::OnKeyDown(uChar, uRepCnt, uFlags);
        }
        break;
    case VK_RIGHT:
        if (GetCurSel() < GetCount()-1)  {
            SetCurSel(GetCurSel()+1);
        }
        break;
    case VK_LEFT:
        if (GetCurSel() > 0)  {
            SetCurSel(GetCurSel()-1);
        }
        break;
    default:
        CComboBox::OnKeyDown(uChar, uRepCnt, uFlags);
        break;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDComboBox::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CDDComboBox::OnSetFocus(CWnd* pOldWnd)  {

    m_pParent->m_iEditCol = m_iCol;
    CWnd::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDComboBox::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CDDComboBox::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {

    CRect rect;
    m_pParent->GetCellRect(m_iCol, m_iRow, &rect);
    rect.top--;
    rect.left--;
    rect.right += 2;
    MoveWindow(&rect, FALSE);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CDDComboBox::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CDDComboBox::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags) {

    if (uChar == 10) {
        if (m_pParent) {
            m_pParent->EditChange(VK_CANCEL);
        }
        else {
            CComboBox::OnChar(uChar, uRepCnt, uFlags);
        }
    }
    else {
        CComboBox::OnChar(uChar, uRepCnt, uFlags);
    }
}

//#include "relgrid.h"

void CDDComboBox::OnSelchange()
{
    m_pParent->EditChange(DD_ONSEL);
}
