//////////////////////////////////////////////////////////////////////////
// CTab_Helper
//            is a collection of useful functions and templates
//   to make Ctab.cpp code easier to debug/read.
//
// rcl, Nov 2004
//
#include "StandardSystemIncludes.h"
#include "Ctab_Helper.h"
#include <zLogicO/Symbol.h>
#include "VarT.h"

bool hasAnyDynamicDim( MVAR_NODE* pNode, int iNumDim )
{
    ASSERT( iNumDim > 0 && iNumDim <= DIM_MAXDIM );

    bool bRet = false;

    for( int i = 0; i < iNumDim; i++ )
    {
        if( pNode->m_iVarSubindexType[i] == MVAR_USE_DYNAMIC_CALC )
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

bool hasAnyGroupDim( MVAR_NODE* pNode, int iNumDim )
{
    ASSERT( iNumDim > 0 && iNumDim <= DIM_MAXDIM );

    bool bRet = false;

    for( int i = 0; i < iNumDim; i++ )
    {
        if( pNode->m_iVarSubindexType[i] == MVAR_GROUP )
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

int getFirstGroupDim( MVAR_NODE* pNode, int iNumDim )
{
    ASSERT( iNumDim > 0 && iNumDim <= DIM_MAXDIM );

    int iGroupDim = -1;

    for( int i = 0; i < iNumDim; i++ )
    {
        if( pNode->m_iVarSubindexType[i] == MVAR_GROUP )
        {
            iGroupDim = i;
            break;
        }
    }

    return iGroupDim;
}

int getSymbolToUseAsUnitForFirstDim( VART* pVarT )
{
    ASSERT( pVarT->IsArray() );

    int iSymbol = MAGIC_NUMBER;

    SECT* pSecT = pVarT->GetSPT();
    // Multiple Record
    if( pSecT->GetMaxOccs() >= 2 )
    {
        ASSERT( pVarT->GetDimType(0) == CDimension::Record );
        iSymbol = pSecT->GetSymbolIndex();
    }
    else
    {
        // Single record, Multi item or subitem
        switch( pVarT->GetNumDim() )
        {
        case 1:
            // Given that record is not multiple, and variable is multiple
            // the only option here is to use the same variable.
            iSymbol = pVarT->GetSymbolIndex();
            break;
        case 2:
            // Record is not multiple and variable uses 2 dimensions
            // -> item must be multiple and variable must be a subitem
            iSymbol = pVarT->GetOwnerSymItem();
            ASSERT( iSymbol > 0 );
            break;
        case 3:
            // Record must be multiple -> error
            ASSERT(0);
            break;
        }
    }

    ASSERT( iSymbol != MAGIC_NUMBER );
    return iSymbol;
}

int getSymbolToUseAsUnitForSecondDim( VART* pVarT )
{
    ASSERT( pVarT->IsArray() );
    ASSERT( pVarT->GetNumDim() > 1 );

    int iSymbol = MAGIC_NUMBER;
    SECT* pSecT = pVarT->GetSPT();

    // Multiple Record
    if( pSecT->GetMaxOccs() >= 2 )
    {
        switch( pVarT->GetNumDim() )
        {
        case 1: ASSERT(0); break; // already checked
        case 2:
            // variable has 2 dims, and we are asking for 2nd dim
            // -> use the same variable
            iSymbol = pVarT->GetSymbolIndex();
            break;
        case 3:
            // variable must be a subitem,
            // -> get the item symbol
            ASSERT( pVarT->GetDimType(3) == CDimension::SubItem );
            iSymbol = pVarT->GetOwnerSymItem();
            ASSERT( iSymbol > 0 );
            break;
        }
    }
    else
    {
        // Single record, Multi item or subitem
        switch( pVarT->GetNumDim() )
        {
        case 1: ASSERT(0); break; // already checked
        case 2:
            // Record is not multiple and variable uses 2 dimensions
            // we need 2nd dimension -> use variable
            iSymbol = pVarT->GetSymbolIndex();
            ASSERT( iSymbol > 0 );
            break;
        case 3:
            // Record must be multiple -> error
            ASSERT(0);
            break;
        }
    }

    ASSERT( iSymbol != MAGIC_NUMBER );
    return iSymbol;
}