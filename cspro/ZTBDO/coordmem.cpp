// CoordMem.cpp: implementation of the CCoordMember class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "coordmem.h"
#include <zLogicO/SymbolTable.h>
#include "cttree.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
void CCoordMember::Dump( CString& csMsg, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable  ) {
    //int*    pCtNodebase=m_pMyEngineDriver->m_pEngineArea->m_CtNodebase;

    CTNODE*  pLeftNode =  (m_iLeft >= 0) ? (CTNODE *) ( pCtNodebase + m_iLeft ) : NULL;
    CTNODE*  pRightNode = (m_iRight >= 0) ? (CTNODE *) ( pCtNodebase + m_iRight ) : NULL;

    Symbol* pSymbolLeft=NULL;
    Symbol* pSymbolRight=NULL;

    if( pLeftNode )
    {
        ASSERT( pLeftNode->isVarNode() );
        pSymbolLeft = &pSymbolTable->GetAt(pLeftNode->m_iSymbol);
    }

    if( pRightNode )
    {
        ASSERT( pLeftNode->isVarNode() );
        pSymbolRight = &pSymbolTable->GetAt(pRightNode->m_iSymbol);
    }

    csMsg.Format( _T("Left=%s  Right=%s, CellLeft=%d"), (pSymbolLeft==NULL) ? _T("none") : pSymbolLeft->GetName().c_str(),
                                                        (pSymbolRight==NULL) ? _T("none") : pSymbolRight->GetName().c_str(),
                                                        m_iCellLeft);
}
#endif
