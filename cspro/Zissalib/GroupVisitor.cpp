//////////////////////////////////////////////////////////////////////////
// GroupVisitor.cpp
//
// GROUPT and SECT Visitor class implementations
//
// rcl, Sept 2005
//
#include "StdAfx.h"

#include "GroupVisitor.h"
#include "GroupT.h"
#include "SecT.h"
#include <engine/VarT.h>

//////////////////////////////////////////////////////////////////////////

void NoHiddenGroupsVisitor::visit( SECT* pSecT )
{
    GROUPT* pGroupT;

    for( int iGroupNum=0; (pGroupT = pSecT->GetGroup(iGroupNum)) != 0 ; iGroupNum++ )
    {
        bool bHiddenGroup = (pGroupT->GetSource() == GROUPT::Source::DcfFile);
        if( bHiddenGroup )
            continue;

        pGroupT->accept( this );
    }
}

//////////////////////////////////////////////////////////////////////////

//
// g_aCurrExOccur -- Map  GROUPT* -> int [m_iExOcc]
//
// needed in SaveExOccVisitor and RestoreExOccVisitor classes
// cleared in SaveExOccVisitor constructor
//
static std::map<GROUPT*,int> g_aCurrExOccur;

//////////////////////////////////////////////////////////////////////////

SaveExOccVisitor::SaveExOccVisitor()
{
    g_aCurrExOccur.clear();
}

void SaveExOccVisitor::visit( GROUPT* pGroupT )
{
    ASSERT( pGroupT != 0 );
    if( pGroupT != 0 )
        g_aCurrExOccur[pGroupT] = pGroupT->GetExOccur();
}

//////////////////////////////////////////////////////////////////////////

void ChangeExOccVisitor::visit( GROUPT* pGroupT )
{
    ASSERT( m_bReady );
    ASSERT( pGroupT != 0 );
    if( pGroupT != 0 && m_bReady )
    {
        pGroupT->SetExOccur( m_iValue );
    }
}

//////////////////////////////////////////////////////////////////////////

void RestoreExOccVisitor::visit( GROUPT* pGroupT )
{
    ASSERT( pGroupT != 0 );
    if( g_aCurrExOccur.find(pGroupT) != g_aCurrExOccur.end() )
        pGroupT->SetExOccur( g_aCurrExOccur[pGroupT] );
}

//////////////////////////////////////////////////////////////////////////
