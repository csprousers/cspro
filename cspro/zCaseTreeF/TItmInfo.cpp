// TItmInfo.cpp: implementation of the CTreeItemInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TItmInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CTreeItemInfo::CTreeItemInfo(int iTypeOfInfo, CString csKey, CString csToolTip, int iNonSelectedIconIndex, int iSelectedIconIndex)
{
    m_iIndex                = -1;
    m_iTypeOfInfo           = iTypeOfInfo;
    m_iOccurrenceIndex      = -1;
    m_iMaxNumOccurrences    = -1;
    m_iMaxItemLabelLength   = -1;
    m_iNonSelectedIconIndex = iNonSelectedIconIndex;
    m_iSelectedIconIndex    = iSelectedIconIndex;
    m_pDictItem             = NULL;
    m_csKey                 = csKey;
    m_pLevel                = NULL;
    m_pItem                 = NULL;
    m_csToolTip             = csToolTip;
    m_bIsExpanded           = false;
}


CTreeItemInfo::CTreeItemInfo(int              /*iNodeIdx*/,
                             CString          csKey,
                             int              iIndex,
                             int              iTypeOfInfo,
                             int              iOccurrenceIndex,
                             int              iMaxNumOccurrences,
                             int              iMaxItemLabelLength,
                             int              iNonSelectedIconIndex,
                             int              iSelectedIconIndex,
                             CString          csToolTip,
                             const CDictItem* pDictItem)
{
    //this->m_iNodeIdx              = iNodeIdx;
    this->m_iIndex                = iIndex;
    this->m_iTypeOfInfo           = iTypeOfInfo;
    this->m_iOccurrenceIndex      = iOccurrenceIndex;
    this->m_iMaxNumOccurrences    = iMaxNumOccurrences;
    this->m_iMaxItemLabelLength   = iMaxItemLabelLength;
    this->m_iNonSelectedIconIndex = iNonSelectedIconIndex;
    this->m_iSelectedIconIndex    = iSelectedIconIndex;
    this->m_pDictItem             = pDictItem;
    this->m_csKey                 = csKey;

    //
    this->m_pLevel      = NULL;
    this->m_pItem       = NULL;

    m_csToolTip         = csToolTip;
    m_bIsExpanded       = false;
}

CTreeItemInfo::CTreeItemInfo()
{
}

CTreeItemInfo::~CTreeItemInfo()
{
    if(m_dwArray.GetSize()>0){
        m_dwArray.RemoveAll();
    }
}

//
void CTreeItemInfo::SetIndex(int iIndex)
{
    this->m_iIndex = iIndex;
}
int CTreeItemInfo::GetIndex()
{
    return this->m_iIndex;
}

//
void CTreeItemInfo::SetTypeOfInfo(int iTypeOfInfo)
{
    this->m_iTypeOfInfo = iTypeOfInfo;
}

int CTreeItemInfo::GetTypeOfInfo()
{
    return this->m_iTypeOfInfo;
}

//
void CTreeItemInfo::SetLevel(CDELevel* pLevel)
{
    this->m_pLevel = pLevel;
}

CDELevel* CTreeItemInfo::GetLevel()
{
    if( m_pLevel==NULL && m_pItem!=NULL ){
        m_pLevel = (CDELevel*) m_pItem;
    }
    return this->m_pLevel;
}

//
void CTreeItemInfo::SetItem(CDEItemBase* pItem)
{
    this->m_pItem = pItem;
}

CDEItemBase* CTreeItemInfo::GetItem()
{
    return m_pItem;
}

//
void CTreeItemInfo::SetOccurrence( int iOccurrenceIndex)
{
    this->m_iOccurrenceIndex = iOccurrenceIndex;
}
int CTreeItemInfo::GetOccurrence()
{
    return this->m_iOccurrenceIndex;
}

//
void CTreeItemInfo::SetMaxNumOccurrences( int iMaxNumOccurrences )
{
    this->m_iMaxNumOccurrences = iMaxNumOccurrences;
}
int CTreeItemInfo::GetMaxNumOccurrences()
{
    return this->m_iMaxNumOccurrences;
}

//
void CTreeItemInfo::SetMaxItemLabelLength( int iMaxItemLabelLength )
{
    this->m_iMaxItemLabelLength = iMaxItemLabelLength;
}
int CTreeItemInfo::GetMaxItemLabelLength()
{
    return this->m_iMaxItemLabelLength;
}

//
void CTreeItemInfo::SetNonSelectedIconIndex(int iNonSelectedIconIndex)
{
    this->m_iNonSelectedIconIndex = iNonSelectedIconIndex;
}
int CTreeItemInfo::GetNonSelectedIconIndex()
{
    return this->m_iNonSelectedIconIndex;
}

//FABN INIT 14/Aug/2002
void CTreeItemInfo::SetSelectedIconIndex(int iSelectedIconIndex)
{
    this->m_iSelectedIconIndex = iSelectedIconIndex;
}

int CTreeItemInfo::GetSelectedIconIndex()
{
    return this->m_iSelectedIconIndex;
}
//FABN END 14/Aug/2002

//FABN 19/Aug/2002
bool CTreeItemInfo::equalKey( CTreeItemInfo * pOtherItemInfo )
{
    CString csForeignKey;
    if( pOtherItemInfo!=NULL ){
        csForeignKey = pOtherItemInfo->GetKey();
    }

    return m_csKey == csForeignKey;
}



//
const CDictItem* CTreeItemInfo::GetDictItem()
{
    return this->m_pDictItem;
}


const CString& CTreeItemInfo::GetKey()
{
    return m_csKey;
}


void CTreeItemInfo::setExpanded( bool bExpanded )
{
        this->m_bIsExpanded = bExpanded;

/*      if( bExpanded ){
                AfxMessageBox(GetKey() + " Expanded");
        } else AfxMessageBox(GetKey() + " Not Expanded"); */
}

bool CTreeItemInfo::IsExpanded()
{
    return this->m_bIsExpanded;
}


void CTreeItemInfo::SetLabel( CString csLabel )
{
    this->m_csLabel = csLabel;
}

CString CTreeItemInfo::GetLabel()
{
    return this->m_csLabel;
}


void CTreeItemInfo::SetToolTip(CString csToolTip)
{
    m_csToolTip = csToolTip;
}

CString& CTreeItemInfo::GetToolTip()
{
    return m_csToolTip;
}


void CTreeItemInfo::SetItemParent(HTREEITEM hItem)
{
    m_hitemParent = hItem;
}

HTREEITEM CTreeItemInfo::GetItemParent()
{
    return m_hitemParent;
}

void CTreeItemInfo::SetItemInfoParent(CTreeItemInfo *pTreeItemInfo)
{
    this->m_pItemInfoParent = pTreeItemInfo;
}

CTreeItemInfo* CTreeItemInfo::GetTreeItemInfoParent()
{
    return this->m_pItemInfoParent;
}

bool CTreeItemInfo::Match(CDEField* pField, int iOcc)
{
    return ( m_pItem && m_pItem->GetItemType() == CDEFormBase::Field && (((CDEField*)m_pItem)==pField) && m_iOccurrenceIndex==iOcc );
}
