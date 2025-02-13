#include "StdAfx.h"
#include "cLinkTer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CLinkTerm::CLinkTerm() {
    Init();
}

CLinkTerm::CLinkTerm( CLinkTerm& rLinkTerm ) {
    Copy( rLinkTerm );
}


CLinkTerm::~CLinkTerm() {
}

void CLinkTerm::Copy ( CLinkTerm& rLinkTerm ) {

    for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
        int iNumLinkVar=rLinkTerm.GetNumLinkVar(iDepth);

        for( int iLinkVar=0; iLinkVar < iNumLinkVar; iLinkVar++ ) {
            CLinkVar&  rLinkVar= rLinkTerm.GetLinkVar( iDepth, iLinkVar );
            AddLinkVar( rLinkVar, iDepth );
        }
    }
}

void CLinkTerm::operator=( CLinkTerm& rLinkTerm ) {
    Copy( rLinkTerm );
}

void CLinkTerm::Init() {
   for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ )
        m_CoorExpr[iDepth].RemoveAll();
}

int CLinkTerm::GetNumLinkVar( int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );
    return m_CoorExpr[iDepth].GetSize();
}

CLinkVar& CLinkTerm::GetLinkVar( int iDepth, int iVar ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );

    return m_CoorExpr[iDepth].ElementAt( iVar );

}

void CLinkTerm::AddLinkVar( CLinkVar& cLinkVar, int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );

    m_CoorExpr[iDepth].Add( cLinkVar );
}

int CLinkTerm::GetNumCells() {
    int     iNumCells[TBD_MAXDEPTH];

    for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
        iNumCells[iDepth] = 0;

        for( int i = 0; i < m_CoorExpr[iDepth].GetSize(); i++ ) {
            CLinkVar& rLinkVar= m_CoorExpr[iDepth].ElementAt(i);

            iNumCells[iDepth] += rLinkVar.GetNumCells();
        }
    }

    ASSERT( iNumCells[0] > 0 );

    if( iNumCells[1] == 0 )
        iNumCells[1] = 1;

    return iNumCells[0] * iNumCells[1];
}


