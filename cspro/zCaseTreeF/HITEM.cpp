// HITEM.cpp: implementation of the CHITEM class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "HITEM.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHITEM::CHITEM(CHITEM* pParentItem)
{
    m_pParent   = pParentItem;
}

CHITEM::~CHITEM()
{
}

int CHITEM::GetLevelIdx()
{
    return m_iLevelIdx;
}
