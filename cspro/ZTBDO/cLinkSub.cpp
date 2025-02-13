#include "StdAfx.h"
#include "cLinkSub.h"
//#include "cLinkUnt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CLinkSubTableDim::CLinkSubTableDim() {
    Init();
}

CLinkSubTableDim::~CLinkSubTableDim() {
}

void CLinkSubTableDim::Init() {
        m_iDim = DIM_NODIM;
        for( int i=0; i < TBD_MAXDEPTH; i++ ) {
            m_csVarName[i].Empty();
            m_iSeqNumber[i] = 0;
        }
}

void CLinkSubTableDim::SetDim( int iDim ) {
    ASSERT( iDim == DIM_ROW || iDim == DIM_COL || iDim == DIM_LAYER || iDim == DIM_NODIM );
    m_iDim = iDim;
}

void CLinkSubTableDim::SetVarName( CString csVarName, int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );
    m_csVarName[iDepth] = csVarName;
}

void CLinkSubTableDim::SetSeqNumber( int iSeqNumber, int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );
    m_iSeqNumber[iDepth] = iSeqNumber;
}

int CLinkSubTableDim::GetDim() {
    return m_iDim;
}

CString CLinkSubTableDim::GetVarName( int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );

     return m_csVarName[iDepth];
}

int CLinkSubTableDim::GetSeqNumber( int iDepth ) {
    ASSERT( iDepth >= 0 && iDepth < TBD_MAXDEPTH );

    return m_iSeqNumber[iDepth];
}




CLinkSubTable::CLinkSubTable() {
    Init();
}

CLinkSubTable::~CLinkSubTable() {
}

void CLinkSubTable::Copy( const CLinkSubTable& rLinkSubTable ) {
    for( int i =0; i < TBD_MAXDIM; i++ )
        m_cDims[i] = rLinkSubTable.m_cDims[i];
}

CLinkSubTable::CLinkSubTable( const CLinkSubTable& rLinkSubTable ) {
    Copy( rLinkSubTable );
}

void CLinkSubTable::operator=( const CLinkSubTable& rLinkSubTable ) {
    Copy( rLinkSubTable );
}


void CLinkSubTable::Init() {
    CLinkSubTableDim    m_cDims[TBD_MAXDIM];


        for( int i=0; i < TBD_MAXDIM; i++ ) {
            m_cDims[i].Init();
        }
}

void CLinkSubTable::AddSubTableDim( const CLinkSubTableDim& rLinkSubTableDim, int iDim ) {
    ASSERT( iDim == DIM_ROW || iDim == DIM_COL || iDim == DIM_LAYER );
    m_cDims[iDim-1] = rLinkSubTableDim;
}

CLinkSubTableDim& CLinkSubTable::GetSubTableDim( int iDim ) {
    ASSERT( iDim == DIM_ROW || iDim == DIM_COL || iDim == DIM_LAYER );

    return m_cDims[iDim-1];
}


CLinkSubTableDim    m_cDims[TBD_MAXDIM];

