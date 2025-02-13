#include "StandardSystemIncludes.h"
#include "INTERPRE.H"


double CIntDriver::exmaxocc_pre80(int iExpr) {
    FNC_NODE* pMaxOccNode = (FNC_NODE*) (PPT(iExpr));
    Symbol* pSymbol=(pMaxOccNode->isymb>0) ? NPT(pMaxOccNode->isymb) : NULL;
    double dMaxOcc=0;

    if( pSymbol == NULL ) { // No args used
        if( m_iExSymbol <= 0 )
            return DEFAULT;

        pSymbol = NPT(m_iExSymbol);
        GROUPT* pGroupT;

        if( pSymbol->IsA(SymbolType::Group) )
            pGroupT = (GROUPT*) pSymbol;
        else if( pSymbol->IsA(SymbolType::Variable) ) {
            pGroupT = ( (VART*)pSymbol)->GetParentGPT();
            if( pGroupT == NULL ) // Single var use owner
                pGroupT = ( (VART*)pSymbol)->GetOwnerGPT();
        }
        else {
            ASSERT(0);
            return 0;
        }

        dMaxOcc = pGroupT->GetMaxOccs();
    }
    else if( pSymbol->IsA(SymbolType::Group) ) {
        GROUPT* pGroupT=(GROUPT*) pSymbol;
        dMaxOcc = pGroupT->GetMaxOccs();
    }
    else if( pSymbol->IsA(SymbolType::Variable) ) {
        VART* pVarT=(VART*) pSymbol;
        dMaxOcc = pVarT->GetMaxOccs();
    }
    else if( pSymbol->IsA(SymbolType::Section) ) {
        SECT* pSecT=(SECT*) pSymbol;
        dMaxOcc = pSecT->GetMaxOccs();
    }
    else
        ASSERT(0);

    return dMaxOcc;
}


double CIntDriver::exsoccurs_pre80( int ioccurs ) {           // victor Aug 10, 99
    FN4_NODE*   poccurs = (FN4_NODE*) (PPT(ioccurs));
    SECT*       pSecT   = SIP(poccurs->sect_ind);
    int         iNumOccs;

    // set section occurrences from related Groups
    // RHF COM Feb 19, 2002   iNumOccs = m_pEngineDriver->SetupSectOccsFromGroups( iSymSec );
    iNumOccs = exsoccurs( pSecT );// RHF May 14, 2003

    return( (double) iNumOccs );
}
