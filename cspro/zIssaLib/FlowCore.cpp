//------------------------------------------------------------------------
//  File name: FlowCore.cpp
//
//  Description:
//             Implementation for the core of flow control
//
//  History: Date       Author  Comment
//           -------------------------------------------------------------
//           08 Jan 01   vc     Created for final expansion to 3 dimensions
//           26 Jan 01   vc     Adding NullOcc (as HeadOcc & TailOcc) to single, scalar groups
//           08 May 01   vc     Adding aIndex to Item' atoms
//           30 May 01   vc     Adding support for level' processing
//           07 Jun 01   vc     New methods getting atom' information on its Level
//           14 Jun 01   vc     Add methods getting atom' information on its Groups & Item
//           20 May 04   rcl    Fix (forward way) in FlowStripBuildOneGroup() & FlowStripBuildHeadOcc()
//           28 May 04   rcl    Fix (backwrd way) in FlowStripBuildOneGroup() & FlowStripBuildTailOcc()
//           29 Sep 04   rcl    Adds toString() method to ease the debugging process.
//
//------------------------------------------------------------------------
#include "StdAfx.h"
#include "FlowCore.h"
#include "CFlow.h"
#include <engine/Engdrv.h>
#include <engine/Engine.h>


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

#define BASE_TO_USE ONE_BASED   // are atoms ONE_BASED or ZERO_BASED ?

//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG


std::wstring CFlowIndexFwd::toString()
{
    return CFlowAtom::toString() + _T(" [CFlowIndexFwd] ") + m_theObject.getIndexes().toStringBare();
}

std::wstring CFlowIndexFwdBwd::toString()
{
    return CFlowIndexFwd::toString() + _T(" [CFlowIndexFwdBwd] ") + m_IndexBwd.toStringBare();
}

#endif
/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
/////////////////////////////////////////////////////////////////////////////

CFlowCore::CFlowCore()
{
    FlowStripRestart();

    // --- support for level' processing                // victor May 30, 01
    m_iMinLevel     = -1;               // minimum level at this strip
    m_iMaxLevel     = -1;               // maximum level at this strip
    for( int iLevel = 0; iLevel <= m_iMaxLevel; iLevel++ )
        m_aLevelHeadIndex[iLevel] = m_aLevelTailIndex[iLevel] = -1;

    // --- engine links
    m_pEngineDriver = NULL;
    m_pEngineArea   = NULL;
    m_engineData    = nullptr;
}


CFlowCore::~CFlowCore()
{
    FlowStripRestart();
    CFlowAtom*              pAtom = ( FlowStripGetSize() > 0 ) ? FlowStripCurr() : NULL;
    CFlowAtom::AtomType     xAtomType;

    while( pAtom != NULL ) {
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType ) {
            case CFlowAtom::AtomType::BeforeStrip:      // before the actual strip
            case CFlowAtom::AtomType::GroupHead:        // head of Group
            case CFlowAtom::AtomType::GroupTail:        // tail of Group
            case CFlowAtom::AtomType::HTOcc:            // Head/Tail of occurrence
            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
            case CFlowAtom::AtomType::Item:             // item
            case CFlowAtom::AtomType::BeyondStrip:      // beyond the actual strip
                delete pAtom;
                break;
            default:
                ASSERT( 0 );            // unknown atom!!!
                break;
        }

        // get next atom
        pAtom = ( xAtomType != CFlowAtom::AtomType::BeyondStrip ) ? FlowStripNext() : NULL;
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// --- attached FLOW
//
/////////////////////////////////////////////////////////////////////////////

void CFlowCore::SetSymbolFlow( CSymbolFlow* pFlow ) {
    m_pSymbolFlow = pFlow;
    pFlow->SetFlowCore( this );
}



/////////////////////////////////////////////////////////////////////////////
//
// --- moving accross the flow-strip
//
/////////////////////////////////////////////////////////////////////////////

CFlowAtom* CFlowCore::FlowStripNext( void ) {
    // can't move beyond the BeyondStrip' atom
    if( m_iCurAtom < FlowStripGetSize() - 1 )
        m_iCurAtom++;

    return m_aAtomStrip[m_iCurAtom];
}

CFlowAtom* CFlowCore::FlowStripPrev( void ) {
    // can't move before the BeforeStrip' atom
    if( m_iCurAtom > 0 )
        m_iCurAtom--;

    return m_aAtomStrip[m_iCurAtom];
}



/////////////////////////////////////////////////////////////////////////////
//
// --- flow-strip management
//
/////////////////////////////////////////////////////////////////////////////

    // index' counters for HTocc atoms
    static
    int     aIndexFwd[DIM_MAXDIM];
    static
    int     aIndexBwd[DIM_MAXDIM];
    static
    int     aIndexNul[DIM_MAXDIM];
    // index' counters of current-occ for Item' atoms   // victor May 08, 01
    static                                              // victor May 08, 01
    int     aIndexOcc[DIM_MAXDIM];                      // victor May 08, 01
    // mark of lowest-field-atom in the flow-strip
    static
    bool    bFieldAtomFound;

int CFlowCore::FlowStripBuild( void ) {
    CSymbolFlow* pFlow = m_pSymbolFlow;
    ASSERT( pFlow != NULL );
    bool        bIsPrimaryFlow = ( pFlow->GetSubType() == SymbolSubType::Primary );
    GROUPT*     pGroupTRoot    = pFlow->GetGroupTRoot();
    int         iMaxLevel      = pGroupTRoot->GetNumItems();
    int         iLevel;
    int         iSymLevel;
    int         iAtom;                                  // victor May 30, 01

    // reset index' counters (in all axis) to zero
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ ) {
         aIndexFwd[iDim] = aIndexBwd[iDim] = 0;
         aIndexNul[iDim] = -1;
    }
    bFieldAtomFound = false;            // mark of lowest-field-atom in the flow-strip

    // starts with the BeforeStrip' atom
    FlowStripAddBefore();

    // head for level 0
    if( bIsPrimaryFlow ) {
        iLevel = 0;
        iSymLevel = pGroupTRoot->GetLevelSymbol( iLevel );
        FlowStripAddGroupHead( iSymLevel, aIndexNul, aIndexNul );

        iAtom = FlowStripGetSize() - 1;                 // victor May 30, 01
        SetLevelHeadIndex( iLevel, iAtom );             // victor May 30, 01
    }

    // flow members
    GROUPT*     pGroupTLevel;
    bool        bIsVisible;

    for( iLevel = 1; iLevel <= iMaxLevel; iLevel++ ) {
        iSymLevel    = pGroupTRoot->GetLevelSymbol( iLevel );
        pGroupTLevel = GPT(iSymLevel);
        bIsVisible   = ( pGroupTLevel->GetSource() == GROUPT::Source::FrmFile );
        if( bIsVisible ) {              // only for FrmFile (visible) groups
            FlowStripAddGroupHead( iSymLevel, aIndexNul, aIndexNul );

            iAtom = FlowStripGetSize() - 1;             // victor May 30, 01
            SetLevelHeadIndex( iLevel, iAtom );         // victor May 30, 01

            FlowStripBuildGroupMembers( iSymLevel );
        }
    }
    for( iLevel = iMaxLevel; iLevel > 0; iLevel-- ) {
        iSymLevel    = pGroupTRoot->GetLevelSymbol( iLevel );
        pGroupTLevel = GPT(iSymLevel);
        bIsVisible   = ( pGroupTLevel->GetSource() == GROUPT::Source::FrmFile );
        if( bIsVisible ) {              // only for FrmFile (visible) groups
            FlowStripAddGroupTail( iSymLevel, aIndexNul );

            iAtom = FlowStripGetSize() - 1;             // victor May 30, 01
            SetLevelTailIndex( iLevel, iAtom );         // victor May 30, 01
        }
    }

    // tail for level 0
    if( bIsPrimaryFlow ) {
        iLevel = 0;
        iSymLevel = pGroupTRoot->GetLevelSymbol( iLevel );
        FlowStripAddGroupTail( iSymLevel, aIndexNul );

        iAtom = FlowStripGetSize() - 1;                 // victor May 30, 01
        SetLevelTailIndex( iLevel, iAtom );             // victor May 30, 01
    }

    // ends with the BeyondStrip' atom
    FlowStripAddBeyond();

    return FlowStripGetSize();
}

void CFlowCore::FlowStripBuildGroupMembers( int iSymGroup )
{
    GROUPT* pGroupT = GPT(iSymGroup);

    for( int iMember = 0; iMember < pGroupT->GetNumItems(); iMember++ )
    {
        int iSymMember = pGroupT->GetItemSymbol( iMember );

        if( iSymMember <= 0 ) // avoid empty members
            continue;

        SymbolType symbol_type = NPT(iSymMember)->GetType();

        if( symbol_type == SymbolType::Variable )
            FlowStripAddItem(iSymMember, aIndexOcc);

        else if( symbol_type == SymbolType::Block )
            FlowStripAddBlock(iSymMember, aIndexOcc, &iMember);

        else if( symbol_type == SymbolType::Group )
            FlowStripBuildOneGroup(iSymMember);

        else
            ASSERT(false);
    }
}

void CFlowCore::FlowStripBuildOneGroup( int iSymGroup ) {
    GROUPT*                 pGroupT = GPT(iSymGroup);
    CDimension::VDimType    xDimType = pGroupT->GetDimType();
    bool                    bIsVisible = ( pGroupT->GetSource() == GROUPT::Source::FrmFile );

    if( !bIsVisible )                   // only for FrmFile (visible) groups
        return;

    if( xDimType == CDimension::VoidDim ) {
        FlowStripAddGroupHead( iSymGroup, aIndexFwd, aIndexFwd );
        FlowStripBuildNullOcc( iSymGroup, CFlowAtom::OccType::Head );
        FlowStripBuildGroupMembers( iSymGroup );
        FlowStripBuildNullOcc( iSymGroup, CFlowAtom::OccType::Tail );
        FlowStripAddGroupTail( iSymGroup, aIndexFwd );
    }
    else {
        // initializes index' counter (in the group' axis)
        aIndexFwd[xDimType] = 1;
        aIndexBwd[xDimType] = 0;

        FlowStripAddGroupHead( iSymGroup, aIndexFwd, aIndexBwd );
        int iMaxOccs = pGroupT->GetMaxOccs();

        // BUCEN INIT Aug 13, 2003
         if(m_pEngineDriver && m_pEngineDriver->GetApplication()) {
            if(m_pEngineDriver->GetApplication()->GetOptimizeFlowTree()) {
                iMaxOccs = 1;
            }
        }
        // BUCEN END Aug 13, 2003

        // Copy information from higher dimensions, rcl, May 28, 04
        for( int i = 0; i < xDimType; i++ )   // rcl, May 28, 04
            aIndexBwd[i] = aIndexFwd[i];      // rcl, May 28, 04

        int iOccur;
        for( iOccur = 1; iOccur <= iMaxOccs; iOccur++ ) {

            // Bug fix:
            // aIndexFwd[xDimType] setting used to be done inside
            // FlowStripBuildHeadOcc() by unconditional incrementation,
            // now it is unconditionally assigned, not incremented,
            // in the line below. Problem detected when using 2 dimensions.
            //
            // rcl, May 20, 04
            aIndexFwd[xDimType] = iOccur;

            // head-occurrence has Bwd occurrence' counter 1 less than Fwd
            aIndexBwd[xDimType] = iOccur - 1;

            FlowStripBuildHeadOcc( iSymGroup, ( iOccur == 1 ) ?
                                   CFlowAtom::OccType::First : CFlowAtom::OccType::Inner );
            FlowStripBuildGroupMembers( iSymGroup );
        }
        iOccur--;
        aIndexFwd[xDimType] = iOccur;
        aIndexBwd[xDimType] = iOccur;
        FlowStripBuildTailOcc( iSymGroup );
        FlowStripAddGroupTail( iSymGroup, aIndexFwd );

        // reset index' counter (in the group' axis) to zero
        aIndexFwd[xDimType] = aIndexBwd[xDimType] = 0;

        // index' counters of current-occ for Item'atoms// victor Jun 14, 01
        for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )  // victor Jun 14, 01
             aIndexOcc[iDim] = aIndexFwd[iDim];         // victor Jun 14, 01
    }
}

void CFlowCore::FlowStripBuildNullOcc( int iSymGroup, CFlowAtom::OccType xOccType ) {
    FlowStripAddHTOcc( iSymGroup, aIndexFwd, aIndexFwd, xOccType );
}

void CFlowCore::FlowStripBuildHeadOcc( int iSymGroup, CFlowAtom::OccType xOccType ) {

    FlowStripAddHTOcc( iSymGroup, aIndexFwd, aIndexBwd, xOccType );
}

void CFlowCore::FlowStripBuildTailOcc( int iSymGroup ) {
    FlowStripAddHTOcc( iSymGroup, aIndexFwd, aIndexBwd, CFlowAtom::OccType::Last );
}

void CFlowCore::FlowStripAddBefore( void ) {
    CFlowBefore*    pFlowBefore = new CFlowBefore();

    m_aAtomStrip.emplace_back(pFlowBefore);
}

void CFlowCore::FlowStripAddGroupHead( int iSymGroup, int* aIndexFwd, int* aIndexBwd ) {
    CNDIndexes theIndexFwd( BASE_TO_USE, aIndexFwd );
    CNDIndexes theIndexBwd( BASE_TO_USE, aIndexBwd );
    CFlowGroupHead* pGroupHead = new CFlowGroupHead( iSymGroup, theIndexFwd, theIndexBwd );

    m_aAtomStrip.emplace_back(pGroupHead);
}

void CFlowCore::FlowStripAddGroupTail( int iSymGroup, int* aIndex ) {
    CNDIndexes theIndex( BASE_TO_USE, aIndex );
    CFlowGroupTail* pGroupTail = new CFlowGroupTail( iSymGroup, theIndex );

    m_aAtomStrip.emplace_back(pGroupTail);
}

void CFlowCore::FlowStripAddHTOcc( int iSymGroup, int* aIndexFwd, int* aIndexBwd, CFlowAtom::OccType xOccType ) {
    CNDIndexes theIndexFwd( BASE_TO_USE, aIndexFwd );
    CNDIndexes theIndexBwd( BASE_TO_USE, aIndexBwd );
    CFlowHTOcc*     pHTOcc = new CFlowHTOcc( iSymGroup, theIndexFwd, theIndexBwd, xOccType );

    // ASSERT( pHTOcc->GetIndexBwd().isZeroBased() );
    // ASSERT( pHTOcc->GetIndexFwd().isZeroBased() );
    ASSERT( pHTOcc->GetIndexBwd().isValid() );
    ASSERT( pHTOcc->GetIndexFwd().isValid() );

    m_aAtomStrip.emplace_back(pHTOcc);

    // index' counters of current-occ for Item' atoms   // victor May 08, 01
    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )      // victor May 08, 01
         aIndexOcc[iDim] = aIndexFwd[iDim];             // victor May 08, 01
}

void CFlowCore::FlowStripAddItem( int iSymItem, int* aIndex ) {
    // ... now including aIndex                         // victor May 08, 01
    CNDIndexes theIndex( BASE_TO_USE, aIndex );
    CFlowItem* pFlowItem = new CFlowItem( iSymItem, theIndex );

    ASSERT( pFlowItem->GetIndex().isValid() );
    m_aAtomStrip.emplace_back(pFlowItem);

    if( !bFieldAtomFound ) {
        // marking lowest-field-atom in the flow-strip
        SetLowestFieldAtom( (CFlowAtom*) pFlowItem );
        bFieldAtomFound = true;
    }
}


void CFlowCore::FlowStripAddBlock(int block_symbol_index, int* aIndex, int* group_iterator)
{
    const EngineBlock& engine_block = GetSymbolEngineBlock(block_symbol_index);
    GROUPT* pGroupT = engine_block.GetGroupT();
    ASSERT(pGroupT->GetItemSymbol(*group_iterator) == block_symbol_index);

    FlowStripAddBlockHead(block_symbol_index, aIndexOcc);

    for( const VART* pVarT : engine_block.GetVarTs() )
    {
        (*group_iterator)++;
        ASSERT(pGroupT->GetItemSymbol(*group_iterator) == pVarT->GetSymbolIndex());

        FlowStripAddItem(pVarT->GetSymbolIndex(), aIndexOcc);
    }

    FlowStripAddBlockTail(block_symbol_index, aIndexOcc);
}

void CFlowCore::FlowStripAddBlockHead(int block_symbol_index, int* aIndex)
{
    CNDIndexes theIndex(BASE_TO_USE, aIndex);
    CFlowBlockHead* pFlowBlockHead = new CFlowBlockHead(block_symbol_index, theIndex);

    m_aAtomStrip.emplace_back(pFlowBlockHead);
}

void CFlowCore::FlowStripAddBlockTail(int block_symbol_index, int* aIndex)
{
    CNDIndexes theIndex(BASE_TO_USE, aIndex);
    CFlowBlockTail* pFlowBlockTail = new CFlowBlockTail(block_symbol_index, theIndex);

    m_aAtomStrip.emplace_back(pFlowBlockTail);
}


void CFlowCore::FlowStripAddBeyond( void ) {
    CFlowBeyond*    pFlowBeyond = new CFlowBeyond();

    m_aAtomStrip.emplace_back(pFlowBeyond);
}

/////////////////////////////////////////////////////////////////////////////
//
// --- support for level' processing                    // victor May 30, 01
//
/////////////////////////////////////////////////////////////////////////////

int CFlowCore::GetLevelHeadIndex( int iLevel ) {
    int         iAtom = -1;
    bool        bDone = ( iLevel >= 0 && iLevel <= (int)MaxNumberLevels);

    if( bDone )
        iAtom = m_aLevelHeadIndex[iLevel];

    return iAtom;
}

int CFlowCore::GetLevelTailIndex( int iLevel ) {
    int         iAtom = -1;
    bool        bDone = ( iLevel >= 0 && iLevel <= (int)MaxNumberLevels );

    if( bDone )
        iAtom = m_aLevelTailIndex[iLevel];

    return iAtom;
}

bool CFlowCore::SetLevelHeadIndex( int iLevel, int iAtom ) {
    bool        bDone = ( iLevel >= 0 && iLevel <= (int)MaxNumberLevels );

    if( bDone ) {
        m_aLevelHeadIndex[iLevel] = iAtom;

        // refresh min/max levels if needed
        if( m_iMinLevel > iLevel || m_iMinLevel < 0 )
            m_iMinLevel = iLevel;
        if( m_iMaxLevel < iLevel )
            m_iMaxLevel = iLevel;
    }

    return bDone;
}

bool CFlowCore::SetLevelTailIndex( int iLevel, int iAtom ) {
    bool        bDone = ( iLevel >= 0 && iLevel <= (int)MaxNumberLevels );

    if( bDone )
        m_aLevelTailIndex[iLevel] = iAtom;

    return bDone;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- getting atom' information on its Level           // victor Jun 07, 01
//
/////////////////////////////////////////////////////////////////////////////

bool CFlowCore::IsLevelHeadTail( CFlowAtom* pAtom, bool bAskForHead ) { // victor Jun 07, 01
    if( pAtom == NULL )                 // the current-atom by default
        pAtom = FlowStripCurr();

    bool    bMatchQuery  = (
                             bAskForHead  && pAtom->GetAtomType() == CFlowAtom::AtomType::GroupHead ||
                             !bAskForHead && pAtom->GetAtomType() == CFlowAtom::AtomType::GroupTail
                           );

    if( bMatchQuery ) {

        CNDIndexes aIndex = ( bAskForHead ) ? ((CFlowGroupHead*) pAtom)->GetIndexFwd() : ((CFlowGroupTail*) pAtom)->GetIndex();

        for( int iDim = 0; bMatchQuery && iDim < DIM_MAXDIM; iDim++ )
            bMatchQuery = ( aIndex.getIndexValue(iDim) == aIndexNul[iDim] );
    }

    return bMatchQuery;
}

int CFlowCore::GetAtomLevelInfo( CFlowAtom* pAtom, bool bAskForLevel ) { // victor Jun 07, 01
    if( pAtom == NULL )                 // the current-atom by default
        pAtom = FlowStripCurr();

    int     iLevel    = -1;
    int     iSymLevel = 0;
    int     iSymGroup = 0;
    int     iSymVar;

    switch( pAtom->GetAtomType() ) {
        case CFlowAtom::AtomType::GroupHead:      // head of Group
            iSymGroup = ((CFlowGroupHead*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::GroupTail:      // tail of Group
            iSymGroup = ((CFlowGroupTail*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::HTOcc:          // Head/Tail of occurrence
            iSymGroup = ((CFlowHTOcc*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::BlockHead:
            iSymGroup = GetSymbolEngineBlock(((CFlowBlockHead*)pAtom)->GetSymbol()).GetGroupT()->GetSymbol();
            break;
        case CFlowAtom::AtomType::BlockTail:
            iSymGroup = GetSymbolEngineBlock(((CFlowBlockTail*)pAtom)->GetSymbol()).GetGroupT()->GetSymbol();
            break;
        case CFlowAtom::AtomType::Item:           // item
            iSymVar   = ((CFlowItem*) pAtom)->GetSymbol();
            iSymGroup = VPT(iSymVar)->GetOwnerGroup();
            break;
        case CFlowAtom::AtomType::BeforeStrip:    // before the actual strip
        case CFlowAtom::AtomType::BeyondStrip:    // beyond the actual strip
            break;
        default:
            ASSERT(false);
            break;
    }

    if( iSymGroup ) {
        GROUPT*     pGroupTRoot = m_pSymbolFlow->GetGroupTRoot();
        GROUPT*     pGroupT     = GPT(iSymGroup);

        iLevel    = pGroupT->GetLevel();
        iSymLevel = pGroupTRoot->GetLevelSymbol( iLevel );
    }

    return( bAskForLevel ? iLevel : iSymLevel );
}

int CFlowCore::GetAtomItemSymbol( CFlowAtom* pAtom ) {  // victor Jun 14, 01
    // GetAtomGroupSymbol: return the Group' symbol of the atom
    int     iSymVar = GetAtomGroupInfo( pAtom, &iSymVar );

    return iSymVar;
}

int CFlowCore::GetAtomGroupInfo( CFlowAtom* pAtom, int* pSymVar ) { // victor Jun 14, 01
    // GetAtomGroupInfo: return the Group' symbol of the atom and, optionally, places the Item' symbol of the atom into a given integer (0 if not an item-atom)
    if( pAtom == NULL )                 // the current-atom by default
        pAtom = FlowStripCurr();

    int     iSymGroup = 0;
    int     iSymVar   = 0;

    switch( pAtom->GetAtomType() ) {
        case CFlowAtom::AtomType::GroupHead:      // head of Group
            iSymGroup = ((CFlowGroupHead*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::GroupTail:      // tail of Group
            iSymGroup = ((CFlowGroupTail*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::HTOcc:          // Head/Tail of occurrence
            iSymGroup = ((CFlowHTOcc*) pAtom)->GetSymbol();
            break;
        case CFlowAtom::AtomType::BlockHead:
            iSymGroup = GetSymbolEngineBlock(((CFlowBlockHead*)pAtom)->GetSymbol()).GetGroupT()->GetSymbol();
            break;
        case CFlowAtom::AtomType::BlockTail:
            iSymGroup = GetSymbolEngineBlock(((CFlowBlockTail*)pAtom)->GetSymbol()).GetGroupT()->GetSymbol();
            break;
        case CFlowAtom::AtomType::Item:           // item
            iSymVar   = ((CFlowItem*) pAtom)->GetSymbol();
            iSymGroup = VPT(iSymVar)->GetOwnerGroup();
            break;
        case CFlowAtom::AtomType::BeforeStrip:    // before the actual strip
        case CFlowAtom::AtomType::BeyondStrip:    // beyond the actual strip
            break;
        default:
            ASSERT(false);
            break;
    }

    // if requested, copy iSymvar (only item-atoms will have it)
    if( pSymVar != NULL )
        *pSymVar = iSymVar;

    return iSymGroup;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- engine links
//
/////////////////////////////////////////////////////////////////////////////

void CFlowCore::SetEngineDriver( CEngineDriver* pEngineDriver )
{
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea   = pEngineDriver->m_pEngineArea;
    m_engineData    = m_pEngineDriver->m_engineData;
}


const Logic::SymbolTable& CFlowCore::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}
