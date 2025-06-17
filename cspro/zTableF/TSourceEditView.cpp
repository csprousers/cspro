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
#include "TSview.h"
#include "TabChWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTSourceEditView

IMPLEMENT_DYNCREATE(CTSourceEditView, CLogicView)

CTSourceEditView::CTSourceEditView()
{
}

CTSourceEditView::~CTSourceEditView()
{
}

void CTSourceEditView::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CView::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CTSourceEditView, CLogicView)
    //{{AFX_MSG_MAP(CTSourceEditView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_VIEW_TABLE, OnViewTable)
    ON_COMMAND(ID_TBL_EDIT_FIND, OnFrmEditFind)
    ON_UPDATE_COMMAND_UI(ID_TBL_EDIT_FIND, OnUpdateFrmEditFind)
    ON_COMMAND(ID_FIND_NEXT, OnFindNext)
    ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
    ON_COMMAND(ID_REPLACE, OnReplace)
    ON_UPDATE_COMMAND_UI(ID_REPLACE, OnUpdateReplace)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCutOrCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)


    ON_WM_MOUSEWHEEL()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTSourceEditView drawing

void CTSourceEditView::OnDraw(CDC* pDC)
{
/*
    BOOL b = GetEditCtrl()->IsIndicatorBarVisible();

    CDocument* pDoc = GetDocument();

    CRect rc;
    GetClientRect(&rc);

    CString sTest = "<Order View goes here>";
    CSize sz = pDC->GetTextExtent(sTest);

    int x = (rc.Width() - sz.cx) / 2;
    int y = (rc.Height() - sz.cy) / 2;
    pDC->TextOut(x, y, sTest);
*/
}

/////////////////////////////////////////////////////////////////////////////
// CTSourceEditView diagnostics

#ifdef _DEBUG
void CTSourceEditView::AssertValid() const
{
    CView::AssertValid();
}

void CTSourceEditView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
//
//                            CTSourceEditView::OnUpdate
//
/////////////////////////////////////////////////////////////////////////////

void CTSourceEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) {

}


/////////////////////////////////////////////////////////////////////////////
//
//                            CTSourceEditView::OnSize
//
/////////////////////////////////////////////////////////////////////////////

void CTSourceEditView::OnSize(UINT nType, int cx, int cy) {
    CLogicView::OnSize(nType, cx, cy);
}

void CTSourceEditView::OnEditCopy()
{
    GetEditCtrl()->CopySelection();
}

void CTSourceEditView::OnUpdateEditCutOrCopy(CCmdUI* pCmdUI)
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

void CTSourceEditView::OnEditCut()
{
    GetEditCtrl()->Cut();
}

void CTSourceEditView::OnEditPaste()
{
    GetEditCtrl()->Paste();
}

void CTSourceEditView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(!GetEditCtrl()->GetReadOnly() &&::IsClipboardFormatAvailable(CF_TEXT));
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


void CTSourceEditView::OnFrmEditFind()
{
    CWnd* pWnd = GetFocus();
    // SAVY 05/18/00 fixed the Ctrl+F bug crash reported by BMD .
    if (pWnd == this)
        pWnd = this->GetLogicCtrl(); // If you dont do this it will go into an infinite loop
    if (pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_FIND);
}

void CTSourceEditView::OnUpdateFrmEditFind(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}

void CTSourceEditView::OnFindNext()
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPEAT);
}

void CTSourceEditView::OnUpdateFindNext(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(TRUE);
    }
}

void CTSourceEditView::OnReplace()
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->GetSafeHwnd())
        pWnd->SendMessage(WM_COMMAND, ID_EDIT_REPLACE);
}

void CTSourceEditView::OnUpdateReplace(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CLogicCtrl))){
        pCmdUI->Enable(TRUE);
    }
}

void CTSourceEditView::OnEditUndo()
{
    GetEditCtrl()->Undo();
}

void CTSourceEditView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if (pWnd && GetLogicCtrl() == pWnd) {
        pCmdUI->Enable(GetEditCtrl()->CanUndo());
    }
}

void CTSourceEditView::OnEditRedo()
{
    GetEditCtrl()->Redo();
}

void CTSourceEditView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetFocus();
    if(pWnd && GetLogicCtrl() == pWnd){
        pCmdUI->Enable(GetEditCtrl()->CanRedo());
    }

}

BOOL CTSourceEditView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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


void CTSourceEditView::OnContextMenu(CWnd* pWnd, CPoint point)
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
    popup.AppendMenu(MF_SEPARATOR);
    popup.AppendMenu(MF_STRING, ID_VIEW_TABLE, _T("View Table"));

    popup.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CTSourceEditView::OnViewTable()
{
    if (GetParentFrame()->IsKindOf(RUNTIME_CLASS(CTableChildWnd))) {
        CTableChildWnd* pTableChildWnd = (CTableChildWnd*)GetParentFrame();
        pTableChildWnd->OnViewTable();
    }
}
