//***************************************************************************
//
//  Description:
//
//  History:    Date       Author   Comment
//              ---------------------------
//
//***************************************************************************

#include "StdAfx.h"
#include "OSview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView

IMPLEMENT_DYNCREATE(COSourceEditView, CLogicView)

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


BEGIN_MESSAGE_MAP(COSourceEditView, CLogicView)
    //{{AFX_MSG_MAP(COSourceEditView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_ORD_EDIT_FIND, OnOrdEditFind)
    ON_UPDATE_COMMAND_UI(ID_ORD_EDIT_FIND, OnUpdateOrdEditFind)
    ON_COMMAND(ID_FIND_NEXT, OnFindNext)
    ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
    ON_COMMAND(ID_REPLACE, OnReplace)
    ON_UPDATE_COMMAND_UI(ID_REPLACE, OnUpdateReplace)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
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
    CLogicView::OnSize(nType, cx, cy);
}

void COSourceEditView::OnEditCopy()
{
    GetEditCtrl()->CopySelection();
}

void COSourceEditView::OnUpdateEditCutOrCopy(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd) {
        if (GetEditCtrl() && GetEditCtrl()->GetSafeHwnd()) {
            const Sci_Position nStartChar = GetEditCtrl()->GetSelectionStart();
            const Sci_Position nEndChar = GetEditCtrl()->GetSelectionEnd();
            pCmdUI->Enable(nStartChar != nEndChar);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
}

void COSourceEditView::OnEditCut()
{
    GetEditCtrl()->Cut();
}

void COSourceEditView::OnEditPaste()
{
    GetEditCtrl()->Paste();
}

void COSourceEditView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(!GetEditCtrl()->GetReadOnly() &&::IsClipboardFormatAvailable(CF_TEXT));
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


void COSourceEditView::OnOrdEditFind()
{
    CWnd* pWnd = GetFocus();
    // SAVY 05/18/00 fixed the Ctrl+F bug crash reported by BMD .
    if (pWnd == this)
        pWnd = this->GetLogicCtrl(); // If you dont do this it will go into an infinite loop
    if (pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_FIND);

    /*
    CWnd* pWnd = GetFocus();

    if (pWnd && pWnd->GetSafeHwnd())
        CLogicView::OnEditFind();
    */
}

void COSourceEditView::OnUpdateOrdEditFind(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}

void COSourceEditView::OnFindNext()
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPEAT);
}

void COSourceEditView::OnUpdateFindNext(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}

void COSourceEditView::OnReplace()
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPLACE);
}

void COSourceEditView::OnUpdateReplace(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}

void COSourceEditView::OnEditUndo()
{
    GetEditCtrl()->Undo();
}

void COSourceEditView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(GetEditCtrl()->CanUndo());
    }
}

void COSourceEditView::OnEditRedo()
{
    GetEditCtrl()->Redo();
}

void COSourceEditView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CLogicCtrl))){
        pCmdUI->Enable(GetEditCtrl()->CanRedo());
    }

}

BOOL COSourceEditView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
// TODO: Add your message handler code here and/or call default
    if (zDelta > 0) {
         //OnVScroll(SB_LINEUP, 0, NULL);
         GetEditCtrl()->SendMessage( WM_VSCROLL, SB_LINEUP);
    }
    else {
        //OnVScroll(SB_LINEDOWN, 0, NULL);
        GetEditCtrl()->SendMessage( WM_VSCROLL, SB_LINEDOWN);
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
        GetEditCtrl()->PostMessage(WM_RBUTTONDOWN, 0, lParam);
    }
}


void COSourceEditView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    CMenu popup;
    popup.CreatePopupMenu();
    CLogicCtrl* pEdit = GetEditCtrl();

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
