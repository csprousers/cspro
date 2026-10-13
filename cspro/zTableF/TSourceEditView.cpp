#include "StdAfx.h"
#include "TSourceEditView.h"
#include "TabChWnd.h"


IMPLEMENT_DYNCREATE(CTSourceEditView, LogicView)


BEGIN_MESSAGE_MAP(CTSourceEditView, LogicView)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_VIEW_TABLE, OnViewTable)
END_MESSAGE_MAP()


void CTSourceEditView::OnContextMenu(CWnd* pWnd, CPoint point)
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
