/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGDropTarget
    Purpose
        This class handles the grid's drag and
        drop client operations.
    Details
         An instance of this class is created
         inside of the main grid class. This class
         then sends notifications to the main grid
         class then performs the appropriate actions
         based on the return value of the notification.
************************************************/

#include "StdAfx.h"
#include "Ugctrl.h"

#ifdef UG_ENABLE_DRAGDROP

/***************************************************
****************************************************/
CUGDropTarget::CUGDropTarget(){
}
/***************************************************
****************************************************/
CUGDropTarget::~CUGDropTarget(){
}

/***************************************************
****************************************************/
DROPEFFECT CUGDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point ){
    return m_ctrl->OnDragEnter(pDataObject);
}
/***************************************************
****************************************************/
DROPEFFECT CUGDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point ){
    int col;
    long row;
    m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row);
    return m_ctrl->OnDragOver(pDataObject,col,row);
}
/***************************************************
****************************************************/
DROPEFFECT CUGDropTarget::OnDropEx( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point ){

    int col;
    long row;
    int rt;
    m_ctrl->GetCellFromPoint(point.x,point.y,&col,&row);
    rt = m_ctrl->OnDragDrop(pDataObject,col,row);

    if(rt != DROPEFFECT_NONE){
        if(pDataObject->IsDataAvailable(CF_TEXT,NULL))
            m_ctrl->GotoCell(col,row);
            CString string;
            HGLOBAL global;
            LPTSTR   buf;
            global = pDataObject->GetGlobalData(CF_TEXT,NULL);
            buf = (LPTSTR)GlobalLock(global);
            string = buf;

            m_ctrl->Paste(string);
    }

    return rt;
}

void CUGDropTarget::OnDragLeave(CWnd* pWnd)
{
    m_ctrl->OnDragLeave(pWnd);
}
DROPEFFECT CUGDropTarget::OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point)
{
    return m_ctrl->OnDragScroll(pWnd, dwKeyState,point);
}
#endif
// the next line must be put in so the MSVC++1.5x will not give an
// unexpected end of file found error, MSVC++1.5x compiler does not
// compile properly if an #endif is the last line
#define UGDRGDRPCPP
