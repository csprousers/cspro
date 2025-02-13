#include "StdAfx.h"
#include "ccross.h"
#include "coordmem.h"
#include <zToolsO/Tools.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


CCrossTable::CCrossTable() : CBaseTable2() {
    Init();
}

CCrossTable::CCrossTable( CArray<int,int>& aDim, CTableDef::ETableType eTableType, int iCellSize, CBaseTable2* pParentRelatedTable ) :
      CBaseTable2( aDim, eTableType, iCellSize, 0, pParentRelatedTable )  {
    Init();
}

CCrossTable::~CCrossTable() {
}

void CCrossTable::Init() {
    SetTableLevel( 0 );
    SetApplDeclared( false );

    SetTitleLen( 0 );
    SetStubLen( 0 );
    SetPageWidth( 132 );
    SetStubWidth( 32 );
    SetPageLen( 60 );
    SetPercentDecs( 2 );
}

void CCrossTable::MakeSubExpresion( CArray<int,int>& aNodeExpr, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable ) {
    CArray<CCoordMember,CCoordMember>     aCoordMember[TBD_MAXDIM];
    CSubExpresion       cSubExpresion;
    CSubRange           cSubRange;
    CString             csParam;
    int                 iSymCoord;
    int                 iNumCells;

    m_aSubExpresion.RemoveAll();

    int                 iNumDim=aNodeExpr.GetSize();
    //int                 iNumDim=pCtab->m_iNumDim;

    for( int iDim=0; iDim < iNumDim; iDim++ ) {
        aCoordMember[iDim].RemoveAll();

        ExpandPolinom( aCoordMember[iDim], pCtNodebase, pSymbolTable, aNodeExpr.GetAt(iDim) );

#ifdef _DEBUG
        {
            for( int i = 0; i < aCoordMember[iDim].GetSize(); i++ ) {
                CCoordMember&   rCoordMember=aCoordMember[iDim].ElementAt(i);

                CString csMsg;

                rCoordMember.Dump( csMsg, pCtNodebase, pSymbolTable );

                TRACE( csMsg );
            }
        }
#endif
    }

    int     iNumSubExpresions=1;

    int     iMax[TBD_MAXDIM];

    for( int iDim = 0; iDim < TBD_MAXDIM; iDim++ )
        iMax[iDim] = 1;

    // Producto cruz
    for( int iDim=0; iDim < iNumDim; iDim++ ) {
        if( aCoordMember[iDim].GetSize() > 0 )
            iNumSubExpresions *= aCoordMember[iDim].GetSize();
        iMax[iDim] = std::max( 1, aCoordMember[iDim].GetSize() );
    }

    //m_aSubExpresion.SetSize( iNumSubExpresions );


    CCoordMember*   rCoordMember[TBD_MAXDIM];

    for( int i = 0; i < iMax[0]; i++ ) {
        if( i >= aCoordMember[0].GetSize() )
            rCoordMember[0] = NULL;
        else
            rCoordMember[0] = &aCoordMember[0].ElementAt(i);

        for( int j = 0; j < iMax[1]; j++ ) {
            if( j >= aCoordMember[1].GetSize() )
                rCoordMember[1] = NULL;
            else
                rCoordMember[1] = &aCoordMember[1].ElementAt(j);

            for( int k = 0; k < iMax[2]; k++ ) {
                if( k >= aCoordMember[2].GetSize() )
                    rCoordMember[2] = NULL;
                else
                    rCoordMember[2] = &aCoordMember[2].ElementAt(k);


                int iSubExpresionNumber = i + j * (iMax[0]-1) + k * ( (iMax[0]-1) + (iMax[1]-1) );

                //CSubExpresion&     cSubExpresion=m_aSubExpresion.ElementAt( iSubExpresionNumber );

                // Init the sub-expresion
                cSubExpresion.Init();

                // SetNumDim
                cSubExpresion.SetNumDim( iNumDim );

                for( int iDim=0; iDim < iNumDim; iDim++ ) {
                    ASSERT( rCoordMember[iDim] != NULL );
                    int     iLeft = rCoordMember[iDim]->m_iLeft;
                    int     iRight = rCoordMember[iDim]->m_iRight;

                    ASSERT( iLeft >= 0 );

                    CTNODE*     pLeftNode = (CTNODE *) ( pCtNodebase + iLeft );
                    CTNODE*     pRightNode =  (iRight >= 0) ? (CTNODE *) ( pCtNodebase + iRight ) : NULL;

                    // TODO     pLeftNode->m_iCtOcc;

                    // Make the sub-expresion
                    for( int iDepth=0; iDepth < TBD_MAXDEPTH; iDepth++ ) {
                        // Symbols
                        iSymCoord = (iDepth==0) ? pLeftNode->m_iSymbol : (pRightNode!=NULL) ? pRightNode->m_iSymbol : 0;
                        cSubExpresion.SetSymCoord( iDim, iDepth, iSymCoord );

                        // Depth
                        if( iSymCoord > 0 )
                            cSubExpresion.SetDepth( iDim, iDepth+1 );

                        // Number of cells
                        iNumCells = (iDepth==0) ? pLeftNode->m_iNumCells :
                        (pRightNode!=NULL) ? pRightNode->m_iNumCells : 1;
                        cSubExpresion.SetNumCells( iDim, iDepth, iNumCells );


                        if( iDepth == 0 ) { // Don't depend on iDepth
                            int     iBaseIndex=rCoordMember[iDim]->m_iCellLeft;

                            if( pRightNode != NULL ) { // Has right element ?
                                // Search the Mult Parent
                                CTNODE*     pMultNode = (CTNODE*)( pCtNodebase + iRight );
                                CTNODE*     pAuxMultNode = NULL; // RHF Nov 21, 2001
                                int         iMultNodeIndex=iRight;

                                while( pMultNode->m_iParentIndex >= 0 ) {
                                    iMultNodeIndex = pMultNode->m_iParentIndex;
                                    pMultNode = (CTNODE*) (pCtNodebase + iMultNodeIndex);

                                    // RHF INIC Nov 21, 2001
                                    if( pMultNode == pAuxMultNode )
                                        break;
                                    else
                                        pAuxMultNode = pMultNode;
                                    // RHF END Nov 21, 2001
                                    if( pMultNode->isOperNode(CTNODE_OP_MUL) )
                                        break;
                                }

                                ASSERT( pMultNode != NULL );
                                ASSERT( pMultNode->m_iCtRight >= 0 );

                                //CTNODE*     pNodeRight = (CTNODE*)( pCtNodebase + pMultNode->m_iCtRight );
                                //int         iRightCells = pNodeRight->m_iNumCells;

                                int         iRightCells = ctcell(pCtNodebase,pMultNode->m_iCtRight);


                                for( int iNumLeftCell=0; iNumLeftCell < pLeftNode->m_iNumCells; iNumLeftCell++ )  {
                                    // (A+X)*(B+C+D)
                                    //         a1                      a2                  x1                       x2                      x3
                                    // b1 b2 c1 c2 c3 d1 d2 |  b1 b2 c1 c2 c3 d1 d2 | b1 b2 c1 c2 c3 d1 d2 | b1 b2 c1 c2 c3 d1 d2 | b1 b2 c1 c2 c3 d1 d2
                                    // D Start in 5 and 12 for submatrix A*D. 12 is equivalent to (5+NumCells(B+C+D)).
                                    // D Start in 19, 26 and 33 for submatrix X*D. 26 is equivalent to (19+NumCells(B+C+D)).
                                    if( iNumLeftCell > 0 )
                                        iBaseIndex += iRightCells;

                                    cSubExpresion.AddBaseIndex( iDim, iBaseIndex );

                                }
                            }
                            else {
                                cSubExpresion.AddBaseIndex( iDim, iBaseIndex );
                            }
                        }

                        int     iStartRange, iEndRange;

                        // SubRanges
                        if( iDepth == 0 ) {
                            iStartRange = iLeft + CTNODE_SLOTS;
                            iEndRange = iStartRange + CTRANGE_SLOTS * pLeftNode->m_iCtNumRanges;
                        }
                        else {
                            iStartRange = iRight + CTNODE_SLOTS;
                            if( pRightNode != NULL )
                                iEndRange = iStartRange + CTRANGE_SLOTS * pRightNode->m_iCtNumRanges;
                            else
                                iEndRange = -1;
                        }

                        for( int iRange = iStartRange; iRange < iEndRange; iRange += CTRANGE_SLOTS ) { // Variable Mapping
                            CTRANGE *pSourceRange = (CTRANGE *) ( pCtNodebase + iRange );

                            double rLow = pSourceRange->m_iRangeLow;
                            double rHigh = pSourceRange->m_iRangeHigh;
                            bool bHighImplicit = ( rLow == rHigh );
                            int iCollapsed = pSourceRange->m_iRangeCollapsed;

                            cSubRange.Init( rLow, rHigh, bHighImplicit, iCollapsed );

                            cSubExpresion.AddSubRange( iDim, iDepth, cSubRange );
                        }

                        csParam.Format( _T("Dim %d, iDepth %d"), iDim+1, iDepth+1);
                        cSubExpresion.AddFunParam( iDim, iDepth, csParam );
                    }



                } //for( int iDim=0; iDim < iNumDim; iDim++ )

                // Add the sub-expresion
                m_aSubExpresion.Add( cSubExpresion );

            } // for( int k = 0; k < iMax[2]; k++ )
        } // for( int j = 0; j < iMax[1]; j++ )
    } // for( int i = 0; i < iMax[0]; i++ )
}



void CCrossTable::ExpandPolinom( CArray<CCoordMember,CCoordMember>& aCoordMember, int* pCtNodebase, const Logic::SymbolTable* pSymbolTable, int iOffset ) {
    CTNODE*             pSourceNode;
    CArray<CCoordMember,CCoordMember>     aCoordMemberLeft;
    CArray<CCoordMember,CCoordMember>     aCoordMemberRight;
    int                 iCellLeft;
#ifdef _DEBUG
    CString             csMsg;
#endif

    pSourceNode = (CTNODE *) ( pCtNodebase + iOffset );

    if( pSourceNode->isVarNode() )
    {
        CCoordMember rCoordMember;

        rCoordMember.m_iLeft = iOffset;
        rCoordMember.m_iRight = -1;
        rCoordMember.m_iCellLeft = 0;

        aCoordMember.Add( rCoordMember );

#ifdef _DEBUG
        rCoordMember.Dump( csMsg, pCtNodebase, pSymbolTable );
        TRACE( csMsg );
#endif
    }
    else
    {
        ASSERT( pSourceNode->isOperNode() );

        switch( pSourceNode->getOperator() )
        {
        case CTNODE_OP_ADD:
            {
                ExpandPolinom( aCoordMemberLeft, pCtNodebase, pSymbolTable, pSourceNode->m_iCtLeft );
                iCellLeft = ctcell(pCtNodebase,pSourceNode->m_iCtLeft);

                ExpandPolinom( aCoordMemberRight, pCtNodebase, pSymbolTable, pSourceNode->m_iCtRight );


                for( int i=0; i < aCoordMemberLeft.GetSize(); i++ ) {
                    CCoordMember& rCoordMember=aCoordMemberLeft.ElementAt(i);

                    aCoordMember.Add( rCoordMember );

#ifdef _DEBUG
                    rCoordMember.Dump( csMsg, pCtNodebase, pSymbolTable );
                    TRACE( csMsg );
#endif
                }

                for( int i=0; i < aCoordMemberRight.GetSize(); i++ ) {
                    CCoordMember& rCoordMember=aCoordMemberRight.ElementAt(i);

                    rCoordMember.m_iCellLeft += iCellLeft;
                    aCoordMember.Add( rCoordMember );

#ifdef _DEBUG
                    rCoordMember.Dump( csMsg, pCtNodebase, pSymbolTable );
                    TRACE( csMsg );
#endif
                }
            }


            break;

        case CTNODE_OP_MUL:
            {
                CCoordMember rCoordMember;

                ExpandPolinom( aCoordMemberLeft,  pCtNodebase, pSymbolTable, pSourceNode->m_iCtLeft );
                iCellLeft = ctcell(pCtNodebase,pSourceNode->m_iCtLeft);

                ExpandPolinom( aCoordMemberRight, pCtNodebase, pSymbolTable, pSourceNode->m_iCtRight );

                // RHF INIC Jul 25, 2001
                int iCellRight = ctcell(pCtNodebase,pSourceNode->m_iCtRight);
                for( int k=0; k < aCoordMemberLeft.GetSize(); k++ ) {
                    CCoordMember& rCoordMemberLeft=aCoordMemberLeft.ElementAt(k);

                    // In the expresion: (A+B+C+D)*(E+F)
                    // C begins after A*(E+F)+B*(E+F)
                    if( k > 0 )
                        rCoordMemberLeft.m_iCellLeft = rCoordMemberLeft.m_iCellLeft * iCellRight;
                }
                // RHF END Jul 25, 2001

                for( int i=0; i < aCoordMemberLeft.GetSize(); i++ ) {
                    CCoordMember& rCoordMemberLeft=aCoordMemberLeft.ElementAt(i);

                    for( int j=0; j < aCoordMemberRight.GetSize(); j++ ) {
                        CCoordMember& rCoordMemberRight=aCoordMemberRight.ElementAt(j);


                        ASSERT( rCoordMemberLeft.m_iLeft >= 0 );
                        ASSERT( rCoordMemberRight.m_iLeft >= 0 );

                        rCoordMember.m_iLeft = rCoordMemberLeft.m_iLeft;
                        rCoordMember.m_iRight = rCoordMemberRight.m_iLeft;
                        rCoordMember.m_iCellLeft = rCoordMemberLeft.m_iCellLeft + rCoordMemberRight.m_iCellLeft;

                        aCoordMember.Add( rCoordMember );

#ifdef _DEBUG
                        rCoordMember.Dump( csMsg, pCtNodebase, pSymbolTable );
                        TRACE( csMsg );
#endif
                    }
                }

            }
            break;

        default:
            break;
        }
    }
}

//The next methods must be here because there are linking problems!!!
bool CCrossTable::IsApplDeclared()                               { return m_bApplDeclared;  }
void CCrossTable::SetApplDeclared( bool bApplDeclared )          { m_bApplDeclared = bApplDeclared; }


CArray<CString,CString>&    CCrossTable::GetTitle()              { return m_csTitle; }
CArray<CString,CString>&    CCrossTable::GetStubTitle()          { return m_csStubTitle; }


int   CCrossTable::GetTitleLen()                                 { return m_iTitleLen; }
void  CCrossTable::SetTitleLen( int iTitleLen )                  { m_iTitleLen = iTitleLen; }

int   CCrossTable::GetStubLen()                                  { return m_iStubLen; }
void  CCrossTable::SetStubLen( int iStubLen )                    { m_iStubLen = iStubLen; }

int  CCrossTable::GetPageWidth()                                 { return m_iPageWidth; }
void CCrossTable::SetPageWidth( int iPageWidth )                 { m_iPageWidth  =  iPageWidth; }

int  CCrossTable::GetStubWidth()                                 { return m_iStubWidth;}
void CCrossTable::SetStubWidth( int iStubWidth )                 { m_iStubWidth =  iStubWidth;}

int  CCrossTable::GetPageLen()                                   { return m_iPageLen;}
void CCrossTable::SetPageLen( int iPageLen )                     { m_iPageLen =  iPageLen; }

int  CCrossTable::GetPercentDecs()                               { return m_iPercentDecs; }
void CCrossTable::SetPercentDecs( int iPercentDecs )             { m_iPercentDecs =  iPercentDecs; }
