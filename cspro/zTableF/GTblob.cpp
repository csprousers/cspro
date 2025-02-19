﻿// GTblob.cpp: implementation of the CGTblOb class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GTblob.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CGTblOb, CObject);
IMPLEMENT_DYNAMIC(CGRowColOb, CGTblOb);
IMPLEMENT_DYNAMIC(CGTblCell, CGTblOb);
IMPLEMENT_DYNAMIC(CGTblRow, CGRowColOb);
IMPLEMENT_DYNAMIC(CGTblCol, CGRowColOb);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGTblOb::CGTblOb()
    :   m_bBlocked(false),
        m_pTblOb(nullptr),
        m_pParent(nullptr)
{
}


//////////////////////////////////////////////////////////////////////
// CGRowColOb Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGRowColOb::CGRowColOb()
{
    m_pTabValue = NULL;
    m_pTabVar = NULL;
//    m_pParent = NULL;
    m_iCurLevel =0; //hierarchy level

}

CGRowColOb::~CGRowColOb()
{
    RemoveAllChildren();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CGRowColOb::RemoveAllChildren(bool bDelete /*= true*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CGRowColOb::RemoveAllChildren(bool bDelete /*= true*/)
{//Recursive
    int iNumChildren = m_arrChildren.GetSize();
    for(int iIndex =0; iIndex < iNumChildren ; iIndex++ ){
        if(bDelete) {
            CGRowColOb* pRowColOb = m_arrChildren[iIndex];
            if(pRowColOb != NULL) {
                pRowColOb->RemoveAllChildren();
                delete m_arrChildren[iIndex];
                m_arrChildren[iIndex] = NULL;
            }
        }
    }
    m_arrChildren.RemoveAll();
}
//////////////////////////////////////////////////////////////////////
// CGTblCol Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGTblCol::CGTblCol()
{

}

CGTblCol::~CGTblCol()
{

}

//////////////////////////////////////////////////////////////////////
// CGTblRow Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGTblRow::CGTblRow()
{

}

CGTblRow::~CGTblRow()
{

}

//////////////////////////////////////////////////////////////////////
// CGTblCell Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGTblCell::CGTblCell()
{
    m_bDirty = true;
    m_iData=0;
}

CGTblCell::~CGTblCell()
{

}
