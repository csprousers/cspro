//***************************************************************************
//  File name: SourceEditView.cpp
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
#include "SourceEditView.h"
#include <zUtilO/BCMenu.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFSourceEditView

IMPLEMENT_DYNCREATE(CFSourceEditView, LogicView)


BEGIN_MESSAGE_MAP(CFSourceEditView, LogicView)
    //{{AFX_MSG_MAP(CFSourceEditView)
    ON_WM_CREATE()
    ON_WM_MOUSEWHEEL()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_QSF_EDITOR, OnQsfEditor)
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_COMMAND(ID_LOGIC_FORMAT, OnFormatLogic)
    ON_UPDATE_COMMAND_UI(ID_LOGIC_FORMAT, OnUpdateFormatLogic)
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

    LogicCtrl* pEdit = GetLogicCtrl();

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
        popMenu.AppendMenu (MF_STRING, ID_QSF_EDITOR, _T("View &Question Text"));
    }
    else {
        popMenu.AppendMenu (MF_STRING|MF_GRAYED, ID_QSF_EDITOR, _T("View &Question Text"));
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
    if(GetLogicCtrl() && GetLogicCtrl()->GetSafeHwnd()){
        GetLogicCtrl()->SetFocus();
    }
}


BOOL CFSourceEditView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
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
        GetLogicCtrl()->PostMessage(WM_RBUTTONDOWN, 0, lParam);
    }
}


void CFSourceEditView::OnFormatLogic()
{
    GetDocument()->SetModifiedFlag(true);
    GetLogicCtrl()->FormatLogic();
}


void CFSourceEditView::OnUpdateFormatLogic(CCmdUI* const pCmdUI)
{
    LogicCtrl* const logic_ctrl = GetLogicCtrl();

    pCmdUI->Enable(( logic_ctrl == GetFocus() &&
                     Lexers::IsCSProLogic(logic_ctrl->GetLexer()) ));
}
