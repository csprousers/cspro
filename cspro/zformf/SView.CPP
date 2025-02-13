//***************************************************************************
//  File name: MyView.cpp
//
//  Description:
//       My customized CHHGenericFormater object
//
//  History:    Date       Author   Comment
//              ---------------------------
//              08 Jun 98   gsf     Created for Measure 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "Sview.h"
#include <zUtilO/BCMenu.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFSourceEditView

IMPLEMENT_DYNCREATE(CFSourceEditView, CLogicView)

CFSourceEditView::CFSourceEditView()
{
}


CFSourceEditView::~CFSourceEditView()
{
}


void CFSourceEditView::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CView::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CFSourceEditView, CLogicView)
    //{{AFX_MSG_MAP(CFSourceEditView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_MOUSEWHEEL()
    ON_COMMAND(ID_FRM_EDIT_FIND, OnFrmEditFind)
    ON_UPDATE_COMMAND_UI(ID_FRM_EDIT_FIND, OnUpdateFrmEditFind)
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
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_QSF_EDITOR, OnQsfEditor)
    ON_WM_SETFOCUS()
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_UPDATE_COMMAND_UI(ID_LOGIC_FORMAT, OnUpdateLogicIsShowing)
    ON_COMMAND(ID_LOGIC_FORMAT, OnFormatLogic)
    ON_COMMAND(ID_VIEW_PEEK_LOGIC_WORD, OnPeekLogicWord)
    ON_COMMAND(ID_VIEW_GOTO_LOGIC_WORD, OnGotoLogicWord)
    ON_COMMAND(ID_COMMENT_CODE, OnCommentCode)
    ON_COMMAND(ID_DUPLICATE_LINE, OnDuplicateLine)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFSourceEditView drawing

void CFSourceEditView::OnDraw(CDC* /*pDC*/)
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CFSourceEditView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CFSourceEditView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                            CFSourceEditView::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CFSourceEditView::OnSize(UINT nType, int cx, int cy)
{
    CLogicView::OnSize(nType, cx, cy);
}


void CFSourceEditView::OnEditCopy()
{
    GetEditCtrl()->CopySelection();
}


void CFSourceEditView::OnUpdateEditCutOrCopy(CCmdUI* pCmdUI)
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


void CFSourceEditView::OnEditCut()
{
    GetEditCtrl()->Cut();
}


void CFSourceEditView::OnEditPaste()
{
    GetEditCtrl()->Paste();
}


void CFSourceEditView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(!GetEditCtrl()->GetReadOnly() &&::IsClipboardFormatAvailable(CF_TEXT));
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


void CFSourceEditView::OnFrmEditFind()
{
    CWnd* pWnd = GetFocus();
    // SAVY 05/18/00 fixed the Ctrl+F bug crash reported by BMD .
    if (pWnd == this)
        pWnd = this->GetLogicCtrl(); // If you dont do this it will go into an infinite loop
    if (pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_FIND);
}


void CFSourceEditView::OnUpdateFrmEditFind(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}


void CFSourceEditView::OnFindNext()
{
    CWnd* pWnd = GetFocus();
    if (pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPEAT);
}


void CFSourceEditView::OnUpdateFindNext(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}


void CFSourceEditView::OnReplace()
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPLACE);
}


void CFSourceEditView::OnUpdateReplace(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}


void CFSourceEditView::OnEditUndo()
{
    GetEditCtrl()->Undo();
}


void CFSourceEditView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd) {
        pCmdUI->Enable(GetEditCtrl()->CanUndo());
    }
}


void CFSourceEditView::OnEditRedo()
{
    GetEditCtrl()->Redo();
}


void CFSourceEditView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(GetEditCtrl()->CanRedo());
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFSourceEditView::OnContextMenu(CWnd* pWnd, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
void CFSourceEditView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    if (point.x < 0 || point.y < 0) {
        point.x = point.y = 5;
        ClientToScreen(&point);
    }
     UpdateData(FALSE);
    // show and track dundas menu
    BCMenu popMenu;
    popMenu.CreatePopupMenu();

    CLogicCtrl* pEdit = GetEditCtrl();

    const bool writable = !pEdit->GetReadOnly();

    popMenu.AppendMenu(writable && pEdit->CanUndo() ? MF_STRING : MF_GRAYED, ID_EDIT_UNDO, _T("Undo"));
    popMenu.AppendMenu(writable && pEdit->CanRedo() ? MF_STRING : MF_GRAYED, ID_EDIT_REDO, _T("Redo"));
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(writable && !pEdit->GetSelectionEmpty() ? MF_STRING : MF_GRAYED, ID_EDIT_CUT, _T("Cut"));
    popMenu.AppendMenu(!pEdit->GetSelectionEmpty() ? MF_STRING : MF_GRAYED, ID_EDIT_COPY, _T("Copy"));
    popMenu.AppendMenu(writable && pEdit->CanPaste() ? MF_STRING : MF_GRAYED, ID_EDIT_PASTE, _T("Paste"));
    //popMenu.AppendMenu(writable && !pEdit->CanPaste() ? MF_STRING : MF_GRAYED, ID_EDIT_DELETE, _T("Delete"));
    //AddTopopMenu("Delete", idcmdDelete, writable && !sel.Empty());
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, _T("Select All"));
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu (MF_STRING, ID_VIEW_FORM, _T("View &Form"));

    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();

    if( pParentFrame != nullptr && pParentFrame->GetUseQuestionText() ) {
        popMenu.AppendMenu (MF_STRING, ID_QSF_EDITOR, _T("View CAPI &Question"));
    }
    else {
        popMenu.AppendMenu (MF_STRING|MF_GRAYED, ID_QSF_EDITOR, _T("View CAPI &Question"));
    }

    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CFSourceEditView::OnViewForm()
{
    ((CFormChildWnd*)GetParentFrame())->OnViewForm();
}


void CFSourceEditView::OnQsfEditor()
{
    ((CFormChildWnd*)GetParentFrame())->OnQsfEditor();
}


void CFSourceEditView::OnSetFocus(CWnd* pOldWnd)
{
    CView::OnSetFocus(pOldWnd);
    if(GetEditCtrl() && GetEditCtrl()->GetSafeHwnd()){
        GetEditCtrl()->SetFocus();
    }
}


BOOL CFSourceEditView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
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


void CFSourceEditView::OnShiftF10()
{
    CWnd* pWnd = GetFocus();
    if (pWnd->IsKindOf(RUNTIME_CLASS(CFormTreeCtrl))) {
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


void CFSourceEditView::OnUpdateLogicIsShowing(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GetFocus() == GetLogicCtrl());
}


void CFSourceEditView::OnFormatLogic()
{
    GetDocument()->SetModifiedFlag(true);
    GetEditCtrl()->FormatLogic();
}


void CFSourceEditView::OnPeekLogicWord()
{
    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, ZEDIT2O_LOGIC_REFERENCE, (LPARAM)GetEditCtrl());
}


void CFSourceEditView::OnGotoLogicWord()
{
    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, ZEDIT2O_LOGIC_REFERENCE_GOTO, (LPARAM)GetEditCtrl());
}


void CFSourceEditView::OnCommentCode() // 20101215
{
    GetDocument()->SetModifiedFlag(true);
    GetEditCtrl()->CommentCode();
}


void CFSourceEditView::OnDuplicateLine()
{
    GetDocument()->SetModifiedFlag(true);
    if (GetEditCtrl()->GetSelectionEmpty()) {
        GetEditCtrl()->LineDuplicate();
    }
    else {
        GetEditCtrl()->SelectionDuplicate();
    }
}
