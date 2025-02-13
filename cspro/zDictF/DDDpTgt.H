#pragma once
//***************************************************************************
//  File name: DDDpTgr.h
//
//  Description:
//       Data Dictionary tree control's drop target definition
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <afxole.h>

class CLASS_DECL_ZDICTF CDictOleDropTarget : public COleDropTarget
{
public:
    CDictOleDropTarget();
    virtual ~CDictOleDropTarget();
    virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
    virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
    virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );

private:
    CString         m_sString;
};
