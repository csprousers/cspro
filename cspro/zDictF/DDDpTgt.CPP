//***************************************************************************
//  File name: DDDpTgr.cpp
//
//  Description:
//       Data Dictionary tree control's drop target implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              19 Mar 99   gsf     Created
//
//***************************************************************************
// To do:
// - make a convention for the global memory string, so we know where it came from
// - get more specific about the return messages
//***************************************************************************

#include "StdAfx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDictOleDropTarget::CDictOleDropTarget()
{

}

CDictOleDropTarget::~CDictOleDropTarget()
{

}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDictOleDropTarget::OnDragEnter
//
/////////////////////////////////////////////////////////////////////////////

DROPEFFECT CDictOleDropTarget::OnDragEnter( CWnd* /*pWnd*/, COleDataObject* pDataObject,
                                  DWORD /*dwKeyState*/, CPoint /*point*/ )
{
    // Get text data from COleDataObject
    HGLOBAL hGlobal=pDataObject->GetGlobalData(_tCF_TEXT);

    m_sString =(TCHAR*)GlobalLock(hGlobal);

    // Unlock memory
    GlobalUnlock(hGlobal);

    if(m_sString.IsEmpty())
        return DROPEFFECT_NONE;
    else

        return DROPEFFECT_COPY;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDictOleDropTarget::OnDrop
//
//  sends a generic message back to the active view with the global memory string
//
/////////////////////////////////////////////////////////////////////////////

BOOL CDictOleDropTarget::OnDrop( CWnd* /*pWnd*/, COleDataObject* pDataObject, DROPEFFECT /*dropEffect*/, CPoint /*point*/ )
{
    HGLOBAL hGlobal=pDataObject->GetGlobalData(_tCF_TEXT);

    m_sString =(TCHAR*)GlobalLock(hGlobal);

    // Unlock memory
    GlobalUnlock(hGlobal);

    if(m_sString.IsEmpty()) {
        return FALSE;
    }

    // tell the active view that there's been a drop
    CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive();
    CWnd* pView = pActiveWnd->GetActiveView();

    pView->SendMessage(UWM::Dictionary::DraggedOffTable, 0, (LPARAM)(LPCTSTR)m_sString);

    return TRUE;

}

/////////////////////////////////////////////////////////////////////////////
//
//                        CDictOleDropTarget::OnDragOver
//
/////////////////////////////////////////////////////////////////////////////

DROPEFFECT CDictOleDropTarget::OnDragOver( CWnd* /*pWnd*/, COleDataObject* pDataObject, DWORD /*dwKeyState*/, CPoint /*point*/ )
{
    // Get text data from COleDataObject
    HGLOBAL hGlobal=pDataObject->GetGlobalData(_tCF_TEXT);


    m_sString =(TCHAR*)GlobalLock(hGlobal);

    // Unlock memory
    GlobalUnlock(hGlobal);

    if(m_sString.IsEmpty())
        return DROPEFFECT_NONE;
    else

        return DROPEFFECT_COPY;

}

