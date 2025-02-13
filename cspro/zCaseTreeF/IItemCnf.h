#pragma once
// IItemCnf.h: interface for the InsertItemConfig class.
//
//////////////////////////////////////////////////////////////////////

class InsertItemConfig
{
public:
    InsertItemConfig(int            iNodeIdx,
                     CDEItemBase*   pItem,
                     int            iItemIndex,
                     int            iItemOcc,
                     HTREEITEM      hItemParent,
                     HTREEITEM      hInsertAfter,
                     CString        csParentKey,
                     CDEField*      pWantedField,
                     int            iWantedOcc,
                     int            iMaxItemLabelLength,
                     bool           bSpecialInsert,
                     bool           bOccIcon);

    virtual ~InsertItemConfig();



    //
    int             m_iNodeIdx;
    CDEItemBase*    m_pItem;
    int             m_iItemIndex;
    int             m_iItemOcc;
    HTREEITEM       m_hItemParent;
    HTREEITEM       m_hInsertAfter;
    CString         m_csParentKey;
    CDEField*       m_pWantedField;
    int             m_iWantedOcc;
    int             m_iMaxItemLength;
    bool            m_bSpecialInsert;
    bool            m_bOccIcon;
};
