//***************************************************************************
//
//  Description:
//
//  History:    Date       Author   Comment
//              ---------------------------
//
//***************************************************************************

#include "StdAfx.h"
#include "OSourceEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView

IMPLEMENT_DYNCREATE(COSourceEditView, LogicView)

COSourceEditView::COSourceEditView()
{
}

COSourceEditView::~COSourceEditView()
{
}

void COSourceEditView::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CView::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(COSourceEditView, LogicView)
    //{{AFX_MSG_MAP(COSourceEditView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_MOUSEWHEEL()
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView drawing

void COSourceEditView::OnDraw(CDC* /*pDC*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView diagnostics

#ifdef _DEBUG
void COSourceEditView::AssertValid() const
{
    CView::AssertValid();
}

void COSourceEditView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
//
//                            COSourceEditView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void COSourceEditView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/) {

}


/////////////////////////////////////////////////////////////////////////////
//
//                            COSourceEditView::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void COSourceEditView::OnSize(UINT nType, int cx, int cy) {
    LogicView::OnSize(nType, cx, cy);
}

BOOL COSourceEditView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
// TODO: Add your message handler code here and/or call default
    if (zDelta > 0) {
         //OnVScroll(SB_LINEUP, 0, NULL);
         GetLogicCtrl()->SendMessage( WM_VSCROLL, SB_LINEUP);
    }
    else {
        //OnVScroll(SB_LINEDOWN, 0, NULL);
        GetLogicCtrl()->SendMessage( WM_VSCROLL, SB_LINEDOWN);
    }
    return FALSE;
}

void COSourceEditView::OnShiftF10()
{
    CWnd* pWnd = GetFocus();
    if (pWnd->IsKindOf(RUNTIME_CLASS(COrderTreeCtrl))) {
        pWnd->PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
    else {
        CPoint point = POINT();
        GetCursorPos(&point);
        CRect rect = RECT();
        pWnd->GetWindowRect(&rect);
        LPARAM lParam = MAKELONG(point.x - rect.left, point.y - rect.top);
        GetLogicCtrl()->PostMessage(WM_RBUTTONDOWN, 0, lParam);
    }
}


void COSourceEditView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    CMenu popup;
    popup.CreatePopupMenu();
    LogicCtrl* pEdit = GetLogicCtrl();

    const bool writable = !pEdit->GetReadOnly();

    popup.AppendMenu(writable && pEdit->CanUndo() ? MF_STRING : MF_GRAYED, ID_EDIT_UNDO, _T("Undo"));
    popup.AppendMenu(writable && pEdit->CanRedo() ? MF_STRING : MF_GRAYED, ID_EDIT_REDO, _T("Redo"));
    popup.AppendMenu(MF_SEPARATOR);
    popup.AppendMenu(writable && !pEdit->GetSelectionEmpty() ? MF_STRING : MF_GRAYED, ID_EDIT_CUT, _T("Cut"));
    popup.AppendMenu(!pEdit->GetSelectionEmpty() ? MF_STRING : MF_GRAYED, ID_EDIT_COPY, _T("Copy"));
    popup.AppendMenu(writable && pEdit->CanPaste() ? MF_STRING : MF_GRAYED, ID_EDIT_PASTE, _T("Paste"));
    //popup.AppendMenu(writable && !pEdit->CanPaste() ? MF_STRING : MF_GRAYED, ID_EDIT_DELETE, _T("Delete"));
    //AddToPopUp("Delete", idcmdDelete, writable && !sel.Empty());
    popup.AppendMenu(MF_SEPARATOR);
    popup.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, _T("Select All"));

    popup.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}
