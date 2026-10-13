#include "StdAfx.h"
#include "OSourceEditView.h"


IMPLEMENT_DYNCREATE(COSourceEditView, LogicView)


BEGIN_MESSAGE_MAP(COSourceEditView, LogicView)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
END_MESSAGE_MAP()


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
