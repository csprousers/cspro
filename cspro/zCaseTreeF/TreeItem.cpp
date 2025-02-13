// TreeItem.cpp: implementation of the CTreeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TreeItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTreeItem::CTreeItem(  CTreeCtrl*       pParentTree,
                       CTreeItemInfo *  pTreeItemInfo,
                       //CIMSAString        CIMSA_Label,
                       CString          csLabel,
                       HTREEITEM        hGenericTreeParentItem,
                       HTREEITEM        hInsertAfter,
                       int              iNonSelectedIconIndex,
                       int              iSelectedIconIndex )
{
    m_pParentTreeCtrl       = pParentTree;
    m_csLabel               = csLabel;
    m_hParentItem           = hGenericTreeParentItem;
    m_hInsertAfter          = hInsertAfter;
    m_iNonSelectedIconIndex = iNonSelectedIconIndex;
    m_iSelectedIconIndex    = iSelectedIconIndex;
    m_pInfo                 = pTreeItemInfo;
}

CTreeItem::~CTreeItem()
{
    m_childItemsArray.RemoveAll();
}

void CTreeItem::AddChild(HTREEITEM hChildItem)
{
    int iIdx = GetIdx( hChildItem );
    if( iIdx==-1 ){
        m_childItemsArray.Add( hChildItem );
    }
}

int CTreeItem::getNumChilds()
{
    return m_childItemsArray.GetSize();
}


void CTreeItem::Remove(HTREEITEM hItem, bool bDeleteFromTree, bool* bRemoved, bool* bDeleted)
{
    *bRemoved = false;
    *bDeleted = false;

    int iIdx = GetIdx( hItem );
    if( iIdx!=-1 ){

        //The item is removed from m_childItemsArray
        int iSize = m_childItemsArray.GetSize();
        CArray<HTREEITEM, HTREEITEM> auxArray;
        int i=0;
        for(i=0; i<iIdx; i++){
            auxArray.Add( m_childItemsArray.GetAt(i) );
        }
        for(i=iIdx+1; i<iSize; i++){
            auxArray.Add( m_childItemsArray.GetAt(i) );
        }
        m_childItemsArray.RemoveAll();
        for(i=0; i<iSize-1; i++){
            m_childItemsArray.Add(auxArray.GetAt(i));
        }
        *bRemoved = true;

        //The item is deleted when bDeleteFromTree==true
        if(bDeleteFromTree){
            *bDeleted = m_pParentTreeCtrl && m_pParentTreeCtrl->DeleteItem(hItem);
        }
    }
}

int CTreeItem::GetIdx(HTREEITEM hItem)
{
    int iIdx = -1;

    if(hItem){
        int iSize = m_childItemsArray.GetSize();
        for(int i=0; iIdx==-1 && i<iSize; i++){
            if(m_childItemsArray.GetAt(i)==hItem){
                iIdx = i;
            }
        }
    }

    return iIdx;
}
