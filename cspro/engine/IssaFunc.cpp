//-----------------------------------------------------------------------//
//  ISSAFUNC.CPP    general purpose functions                            //
//-----------------------------------------------------------------------//
#include "StandardSystemIncludes.h"
#include "Tables.h"
#include "Engine.h"
#include "Comp.h"
#include <zToolsO/Tools.h>


void  CEngineArea::GroupTtrip( GROUPT* pGroupT, pGroupTripFunc func, pGroupTripFunc2 func2, int iInfo, void* pInfo ) {
    GROUPT* pGroupX = pGroupT;      // this group!
    bool    bIsRoot = (
        pGroupX->GetGroupType() == 1 && // 1: Level, 2: Group/Record, 3-Mult.item
        pGroupX->GetLevel()     == 0 && // Group Level (0: process, 1-4: visible hierarchy)
        pGroupX->GetSymbol()    > 0     // Group symbol
        );

    if( bIsRoot ) {
        if( func != NULL )
            (m_pEngineDriver->m_pEngineCompFunc.get()->*func)(pGroupT); // PROC LEVEL_0!
        if( func2 != NULL )
            (pGroupT->*func2)(pGroupT, iInfo, pInfo); // PROC LEVEL_0!
    }

    for( int iItem = 0; iItem < pGroupX->GetNumItems(); iItem++ )
    {
        std::vector<Symbol*> symbols;
        int iSymItem = pGroupX->GetItemSymbol(iItem);
        bool bDoGroupTtrip = false;

        if( iSymItem > 0 )
        {
            Symbol* symbol = NPT(iSymItem);

            if( symbol->IsOneOf(SymbolType::Variable, SymbolType::Block) )
                symbols.push_back(symbol);

            else if( symbol->IsA(SymbolType::Group) )
            {
                pGroupT = assert_cast<GROUPT*>(symbol);
                bDoGroupTtrip = true;
                symbols.push_back(pGroupT);
            }
        }

        for( auto symbol : symbols )
        {
            if( func != nullptr )
                (m_pEngineDriver->m_pEngineCompFunc.get()->*func)(symbol);

            if( func2 != nullptr )
                (pGroupT->*func2)(symbol, iInfo, pInfo);
        }

        if( bDoGroupTtrip )
            GroupTtrip( pGroupT, func, func2, iInfo, pInfo );    // for each item in Group
    }
}

void CEngineArea::dicttrip( DICT *dp, pDictTripFunc func ) {
    int     is, iv;
    SECT    *sp;
    VART    *vp;

    (this->*func)( dp );
    is = dp->SYMTfsec;
    while( is >= 0 )
    {
        sp = SPT( is );
        is = sp->SYMTfwd;
        (this->*func)( sp );

        iv = sp->SYMTfvar;
        while( iv >= 0 )
        {
            vp = VPT( iv );
            iv = vp->SYMTfwd;
            (this->*func)( vp );
        }
    }
}
