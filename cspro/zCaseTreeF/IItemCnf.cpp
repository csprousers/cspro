// IItemCnf.cpp: implementation of the InsertItemConfig class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "IItemCnf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InsertItemConfig::InsertItemConfig(  int            iNodeIdx,
                                     CDEItemBase*   pItem,          //
                                     int            iItemIndex,
                                     int            iItemOcc,
                                     HTREEITEM      hItemParent,
                                     HTREEITEM      hInsertAfter,
                                     CString        csParentKey,
                                     CDEField*      pWantedField,
                                     int            iWantedOcc,
                                     int            iMaxItemLabelLength,
                                     bool           bSpecialInsert,
                                     bool           bOccIcon)
{
    m_iNodeIdx          = iNodeIdx;
    m_pItem             = pItem;
    m_iItemIndex        = iItemIndex;
    m_iItemOcc          = iItemOcc;
    m_hItemParent       = hItemParent;
    m_hInsertAfter      = hInsertAfter;
    m_csParentKey       = csParentKey;
    m_pWantedField      = pWantedField;
    m_iWantedOcc        = iWantedOcc;
    m_iMaxItemLength    = iMaxItemLabelLength;
    m_bSpecialInsert    = bSpecialInsert;
    m_bOccIcon          = bOccIcon;
}

InsertItemConfig::~InsertItemConfig()
{

}



