#pragma once
// HITEM.h: interface for the CHITEM class.
//
//////////////////////////////////////////////////////////////////////

#include <zCaseTreeF/TItmInfo.h>

class CHITEM
{
public:
    int GetLevelIdx();

    int                     m_iLevelIdx;    //no parents => 0, 1 parent => 1, etc..
    int                     m_iSelectedIconIdx;
    int                     m_iNonSelectedIconIdx;
    CString                 m_csToolTip;
    CString                 m_csLabel;
    CTreeItemInfo*          m_pInfo;

    CHITEM*                 m_pParent;
    CArray<CHITEM*,CHITEM*> m_pChild;

    CHITEM(CHITEM* pParent);
    virtual ~CHITEM();

};
