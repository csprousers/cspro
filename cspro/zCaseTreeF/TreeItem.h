#pragma once
// TreeItem.h: interface for the CTreeItem class.
//
//////////////////////////////////////////////////////////////////////

#include <zCaseTreeF/zCaseTreeF.h>
#include <zCaseTreeF/TItmInfo.h>


class ZCASETREEF_API CTreeItem
{
public:

    CTreeItem(  CTreeCtrl*      pParentTree,
                CTreeItemInfo * pItemInfo,
                CString         csLabel,
                //CIMSAString       CIMSA_Label,
                HTREEITEM       hGenericTreeParentItem,
                HTREEITEM       hInsertAfter,
                int             iNonSelectedIconIndex,
                int             iSelectedIconIndex );

    virtual ~CTreeItem();

public :

    int     GetIdx( HTREEITEM hItem );
    int     getNumChilds();

    void    Remove( HTREEITEM hItem, bool bDeleteFromTree, bool* bRemoved, bool* bDeleted );
    void    AddChild( HTREEITEM hChildItem );

    CString                         m_csLabel;
    HTREEITEM                       m_hParentItem;
    HTREEITEM                       m_hInsertAfter;
    int                             m_iNonSelectedIconIndex;
    int                             m_iSelectedIconIndex;
    CTreeItemInfo   *               m_pInfo;
    CArray<HTREEITEM, HTREEITEM>    m_childItemsArray;
    CTreeCtrl*                      m_pParentTreeCtrl;
};
