#include "StdAfx.h"
#include "cLinkVar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CLinkVar::CLinkVar() {
    Init();
}

CLinkVar::CLinkVar(const CLinkVar& rLinkVar ) {
    Copy( rLinkVar );
}

void CLinkVar::operator=(const CLinkVar& rLinkVar ) {
    Copy( rLinkVar );
}

void CLinkVar::Copy(const CLinkVar& rLinkVar ) {
    Init();
    SetName( rLinkVar.GetName() );
    SetOcc( rLinkVar.GetOcc() );

    for( int iRange=0; iRange < rLinkVar.GetNumRanges(); iRange++ ) {
        const CSubRange&  rSubRange=rLinkVar.GetRange(iRange);

        AddRange( rSubRange );
    }
}

CLinkVar::~CLinkVar() {
}

void CLinkVar::Init() {
    m_csName.Empty();
    m_csOcc.Empty();
    m_aRanges.RemoveAll();
}

// Get methods
CString CLinkVar::GetName() const {
    return m_csName;
}

CString CLinkVar::GetOcc() const {
    return m_csOcc;
}

int CLinkVar::GetNumRanges() const {
    return m_aRanges.GetSize();
}

CSubRange& CLinkVar::GetRange( int i ) {
    return m_aRanges.ElementAt(i);
}

const CSubRange& CLinkVar::GetRange( int i ) const {
    return m_aRanges.ElementAt(i);
}

// Set methods
void CLinkVar::SetName( CString csName ) {
    m_csName = csName;
}

void CLinkVar::SetOcc( CString csOcc ) {
    m_csOcc = csOcc;
}

void CLinkVar::AddRange( const CSubRange& cRange ) {
   m_aRanges.Add( cRange );
}

int CLinkVar::GetNumCells() {
    int     iNumCells=0;

    for( int i=0; i < GetNumRanges(); i++ ) {
        CSubRange&  cRange=GetRange(i);

        iNumCells += cRange.GetNumCells();
    }

    return iNumCells;
}


