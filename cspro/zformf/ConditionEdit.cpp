#include "StdAfx.h"
#include "ConditionEdit.h"
#include "ConditionGrid.h"


/////////////////////////////////////////////////////////////////////////////
//
//                             CLabelEdit1
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CLabelEdit1, CEdit)
    ON_WM_SETFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_SIZE()
    ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit1::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CLabelEdit1::Create(DWORD dwStyle, const RECT& rect, CWnd* pParent, UINT uID)
{
    m_pParent = (CCondGrid*)pParent;
    BOOL bRetVal = CWnd::Create(_T("EDIT"), NULL, ES_AUTOHSCROLL | ES_MULTILINE | dwStyle, rect, ((CUGCtrl*)pParent)->m_CUGGrid, uID);
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit1::OnKeyDown
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit1::OnKeyDown(UINT uChar, UINT uRepCnt, UINT uFlags)
{
    if ((uChar == VK_DOWN || uChar == VK_UP) && m_pParent) {
        m_pParent->EditChange(uChar);
    }
    else {
        CEdit::OnKeyDown(uChar, uRepCnt, uFlags);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit1::OnChar
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit1::OnChar(UINT uChar, UINT uRepCnt, UINT uFlags)
{
    if ((uChar == VK_ESCAPE || uChar == VK_RETURN || uChar == VK_TAB) && m_pParent) {
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
//                        CLabelEdit1::OnSetFocus
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit1::OnSetFocus(CWnd* pOldWnd)
{
    CEdit::OnSetFocus(pOldWnd);
    InvalidateRect(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CLabelEdit1::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CLabelEdit1::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
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

void CLabelEdit1::OnKillFocus(CWnd* pNewWnd)
{
    CEdit::OnKillFocus(pNewWnd);

    CWnd* pNewWndParent = NULL;
    pNewWnd ? pNewWndParent = pNewWnd->GetParent() : pNewWndParent = NULL;

    CWnd* pParentFrame = NULL;
    pNewWnd ? pParentFrame = pNewWnd->GetParentFrame() : pParentFrame = NULL;


    //If the user changing the frame and not from the AfxMessageDialog of the "Invalid condition"
    if (pNewWnd && !pNewWnd->IsKindOf(RUNTIME_CLASS(CFormTreeCtrl)) && !pParentFrame->IsKindOf(RUNTIME_CLASS(CFormChildWnd))) {
        if (pNewWndParent) {
            TCHAR lpszClassName[255];
            GetClassName(pNewWndParent->m_hWnd, lpszClassName, 255);
            if (_tcscmp(lpszClassName, _T("#32770")) == 0) {//From the AfxDialog of the "Invalid Condition" do not process
                return;
            }
        }
        m_pParent->EditChange(VK_ESCAPE, true);
        return;
    }
    if (pNewWnd && pNewWnd->IsKindOf(RUNTIME_CLASS(CQSFEView))) {//run in silent mode to avoid multiple messages
        m_pParent->EditChange(VK_RETURN, true);
        return;
    }
    m_pParent->EditChange(VK_RETURN);
}
