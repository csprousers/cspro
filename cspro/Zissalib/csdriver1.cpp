//---------------------------------------------------------------------------
//  File name: CsDriver1.cpp
//
//  Description:
//          csdriver.cpp extension
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/Engine.h>

#pragma warning (disable:4482)
#include "CsDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


void CsDriver::PassTo3D( C3DObject* p3DObject, int iSymbol, int iOccur )
{
    ASSERT( p3DObject != NULL && iSymbol > 0 && iOccur >= 0 );
    Symbol* symbol = NPT(iSymbol);

    p3DObject->SetEmpty();
    p3DObject->SetSymbol( iSymbol );

    if( symbol->IsOneOf(SymbolType::Group, SymbolType::Block) )
    {
        GROUPT* pGroupT = symbol->IsA(SymbolType::Group) ? assert_cast<GROUPT*>(symbol) :
                                                           assert_cast<const EngineBlock*>(symbol)->GetGroupT();

        if( pGroupT->GetNumDim() > 0 )
        {
            CDimension::VDimType xDimType = pGroupT->GetDimType();
            p3DObject->setIndexValue( xDimType, iOccur );
            // TODO how to pass all the indexes involved? (i.e. occs of parent froups?)
        }
    }

    else if( symbol->IsA(SymbolType::Variable) )
    {
        VART* pVarT = assert_cast<VART*>(symbol);

        if( pVarT->IsArray() )
        {
            ASSERT( iOccur >= 1 && iOccur <= pVarT->GetDimSize( 0 ) );
            CDimension::VDimType xDimType = pVarT->GetDimType( 0 );
            p3DObject->setIndexValue( xDimType, iOccur );
            // TODO how to pass all the indexes involved? (i.e. occs of parent froups?)
        }
    }

    else
    {
        ASSERT(false);
    }
}

void CsDriver::PassTo3D( C3DObject* p3DObject, int iSymbol, const UserIndexesArray& theArray ) {
    ASSERT( p3DObject != NULL && iSymbol > 0 );

    PassTo3D( p3DObject, NPT(iSymbol), theArray );
}

/*static*/ void CsDriver::PassTo3D( C3DObject* p3DObject, const Symbol* pSymbol, const UserIndexesArray& theArray ) {
    ASSERT( p3DObject != NULL && pSymbol != NULL );
    int                     iSymbol=pSymbol->GetSymbolIndex();
    SymbolType     eSymType = pSymbol->GetType();
    CDimension::VDimType    xDimType;

    p3DObject->SetEmpty();
    p3DObject->SetSymbol( iSymbol );

    if( eSymType == SymbolType::Group || eSymType == SymbolType::Block )
    {
        GROUPT* pGroupT = ( eSymType == SymbolType::Group ) ? (GROUPT*)pSymbol :
                                                              assert_cast<const EngineBlock*>(pSymbol)->GetGroupT();

        if( pGroupT->GetNumDim() > 0 )
        {
            // AfxDebugMesssage("RCL: Not implemented, could work, could not work" );

            xDimType = pGroupT->GetDimType();
            p3DObject->setIndexValue( xDimType, (int) theArray[0] );
            // TODO how to pass all the indexes involved? (i.e. occs of parent froups?)
            p3DObject->getIndexes().setAsInitialized(); // could be superfluous
        }
    }

    else if( eSymType == SymbolType::Variable )
    {
        VART* pVarT = (VART*) pSymbol;
        if( pVarT->IsArray() )
        {
            int iIndexNumber = pVarT->GetNumDim();
            for( int i = 0; i < iIndexNumber; i++ )
            {
                int iOccur = (int) theArray[i];

                // It is possible that iOccur become 0 in some ocassions
                if( iOccur < 1 ) iOccur = 1;

                ASSERT( iOccur >= 1 && iOccur <= pVarT->GetDimSize( i ) );
                xDimType = pVarT->GetDimType( i );
                p3DObject->setIndexValue( xDimType, iOccur );
            }
            p3DObject->getIndexes().setAsInitialized(); // could be superfluous
        }
    }

    else
        ASSERT( 0 );                // impossible - 3D-objects can be GR, Block, or VA only
}


void CsDriver::PassFrom3DToDeFld( C3DObject* p3DObject, DEFLD3* pDeFld, bool bFullDim ) { // victor Jun 14, 01
    // PassFrom3DToDeFld: pass a given 3D-object to an equivalent, given pDeFld (only if a Var)
    if( pDeFld != NULL ) {
        int         iSymVar = 0;
        CNDIndexes  aIndex( ONE_BASED, DEFAULT_INDEXES );

        // ... pass current 3D-object to a couple iSymVar/aIndex (restricted to Vars)
        if( !p3DObject->IsEmpty() ) {
            int     iSymbol = p3DObject->GetSymbol();
            bool    bIsItem = NPT(iSymbol)->IsA(SymbolType::Variable);

            if( bIsItem ) {
                iSymVar = iSymbol;

                if( bFullDim )          // HANDSHAKING3D, multi-index version
                    p3DObject->getIndexes( aIndex );
                else {                  // TRANSITION, 1-index version
                    int     iOccur;

                    PassFrom3D( p3DObject, &iSymbol, &iOccur );
                    if( !VPT(iSymVar)->IsArray() && iOccur <= 0 )
                        iOccur = 1;     // adjusting index
                    aIndex.setIndexValue(0,iOccur);
                }
            }
        }

        // ... install iSymVar/aIndex into the given pDeFld
        pDeFld->SetSymbol( iSymVar );
        pDeFld->setIndexes( aIndex );
    }
}

void CsDriver::PassFrom3D( C3DObject* p3DObject, int* pSymbol, int* pOccur, int iDefaultSingleOcc )
{
    ASSERT( p3DObject != NULL && pSymbol != NULL );
    Symbol* symbol = NPT(p3DObject->GetSymbol());
    int iOccur = iDefaultSingleOcc; // assuming not an array

    if( symbol->IsOneOf(SymbolType::Group, SymbolType::Block) )
    {
        GROUPT* pGroupT = symbol->IsA(SymbolType::Group) ? assert_cast<GROUPT*>(symbol) :
                                                           assert_cast<const EngineBlock*>(symbol)->GetGroupT();
        CDimension::VDimType xDimType = pGroupT->GetDimType();
        iOccur = p3DObject->getIndexValue( xDimType );
    }

    else if( symbol->IsA(SymbolType::Variable) )
    {
        VART* pVarT = assert_cast<VART*>(symbol);

        if( pVarT->IsArray() )
        {
            ASSERT( pVarT->GetNumDim() > 0 );
            CDimension::VDimType xDimType = pVarT->GetDimType( pVarT->GetNumDim() - 1 ); // rcl Jun 11, 2004
            iOccur = p3DObject->getIndexValue( xDimType );
        }
    }

    else
    {
        ASSERT(false);
    }

    *pSymbol = p3DObject->GetSymbol();
    *pOccur  = iOccur;
}
