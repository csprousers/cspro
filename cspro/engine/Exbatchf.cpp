#include "StandardSystemIncludes.h"

#undef  GENCODE
#define GENCODE

#include "Exappl.h"
#include "COMPILAD.H"
#include "Engine.h"
#include "Batdrv.h"
#include "runmodes.h"
#include "Ctab.h"
#include <zEngineO/Block.h>
#include <zToolsO/Tools.h>
#include <zAppO/Application.h>
#include <zTableO/zTableO.h>
#include <zTableO/Table.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////
//
// --- Construction/Destruction
//
//////////////////////////////////////////////////////////////////////////////

CBatchDriverBase::CBatchDriverBase(Application* pApplication)
    :   CEngineDriver(pApplication, true)
{
    SetBatchMode( CRUNAPL_NONE );
    SetHasAnyTable( false );

    // program-strip initialization
    m_iBatchProgIndex = -1;

    for( int iLevel = 0; iLevel <= (int)MaxNumberLevels; iLevel++ )
        m_iLevProg[iLevel] = -1;           // no starting proc
}

CBatchDriverBase::~CBatchDriverBase()
{
    for( auto pPrSlot : m_apBatchProg )
        delete pPrSlot;
}

CBatchDriver::CBatchDriver(Application* pApplication)
    :   CBatchDriverBase(pApplication)
{
    m_csExprtab = _T("EXPRTAB");

    m_Tbd.SetBatchDriver( this );                      // RHF 29/12/1999
    m_Tbd.SetNewTbd( CSettings::m_bNewTbd );
}


CBatchDriver::~CBatchDriver() {

}


//////////////////////////////////////////////////////////////////////////////
//
// --- Program strip management
//
//////////////////////////////////////////////////////////////////////////////

void CBatchDriverBase::BatchCreateProg( void ) {
    // BatchCreateProg: build the batch program-strip
    // ... formerly 'makebprog', now 'BatchCreateProg'
    // set primary flow as current                      // victor May 08, 00
    m_pEngineDriver->SetFlowInProcess( Appl.GetFlowAt( 0 ) );

    // build the program strip for each level
    GROUPT* pGroupT = GetGroupTRootInProcess()->GetLevelGPT( 0 );
    int     iNumLevels = pGroupT->GetNumItems();

    // RHF INIC May 07, 2003
    bool    bAsCtab= (GetBatchMode() == CRUNAPL_CSTAB );
    bool    bAsCsCalc= (GetBatchMode() == CRUNAPL_CSCALC );
    if( bAsCtab || bAsCsCalc )
        MakeCtabExecOrder();
    // RHF END May 07, 2003

    for( int iLevel = 0; iLevel <= iNumLevels; iLevel++ )
        CreateProgLevel( iLevel );      // was 'bdicprog'

    // reset flow in process                            // victor May 08, 00
    m_pEngineDriver->ResetFlowInProcess();
}


void CBatchDriver::CreateProgLevel( int iLevel ) {
    // CreateProgLevel: build the program-strip partition for a given level
    // ... formerly 'bdicprog', now 'CreateProgLevel'
    int     iSymLevel = GetGroupTRootInProcess()->GetLevelSymbol( iLevel );
    ASSERT( iSymLevel > 0 );
    GROUPT* pGroupT = GPT(iSymLevel);

    // adding Level' PROCTYPE_PRE-block
    CreateProgSlot( iLevel, CPrSlot::LEVELslot, ProcType::PreProc, iSymLevel );

    bool    bAsCtab= (GetBatchMode() == CRUNAPL_CSTAB );

    if( iLevel > 0 ) {                  // for levels 1+:
        // adding all members in this Level

        if( bAsCtab ) {
            for( int iTabSlot = 0; iTabSlot < m_aCtabExecOrder.GetSize(); iTabSlot++ ) {
                int     iCtab=m_aCtabExecOrder.GetAt(iTabSlot);
                CTAB*   pCtab = XPT(iCtab);

                if( pCtab->GetTableLevel() == iLevel ) {
                    CreateProgTable( iLevel, iCtab );
                }
            }
        }
        else {
            int     iNumLevelItems = pGroupT->GetNumItems();

            for( int iItem = 0; iItem < iNumLevelItems; iItem++ ) {
                int     iSymItem = pGroupT->GetItemSymbol( iItem );

                switch( m_pEngineArea->GetTypeGRorVAorBlock( iSymItem ) ) {
                case SymbolType::Group:
                    CreateProgGroup( iLevel, iSymItem );
                    break;
                case SymbolType::Block:
                    CreateProgBlock( iLevel, iSymItem );
                    iItem += GetSymbolEngineBlock(iSymItem).GetVarTs().size();
                    break;
                case SymbolType::Variable:
                    CreateProgItem( iLevel, iSymItem );
                    break;
                default:
                    // unknown item type - ignored, not added to prog chain
                    break;
                }
            }
        }
    }

    // adding Level' POS-block
    CreateProgSlot( iLevel, CPrSlot::LEVELslot, ProcType::PostProc, iSymLevel );

    // marking "no more progs at this level"
    CreateProgSlot( iLevel, CPrSlot::EMPTYslot );
}

int CBatchDriver::CreateProgGroup( int iLevel, int iSymGroup, bool bDoCreate ) {
    // CreateProgGroup: build the program-strip block for a given group
    ASSERT( iSymGroup );
    GROUPT* pGroupT = GPT(iSymGroup);
    int     iNumGroupItems = pGroupT->GetNumItems();
    bool    bIsNamedGroup = ( pGroupT->GetGroupType() != GROUPT::MultItem );
    bool    bIsLevelGroup = ( pGroupT->GetGroupType() == GROUPT::Level );
    int     iNumItemProgSlots = 0;

    if( !bIsLevelGroup ) {
        // count item' prog-slots in Group
        for( int iItem = 0; iItem < iNumGroupItems; iItem++ ){
            int     iSymItem = pGroupT->GetItemSymbol( iItem );

            switch( m_pEngineArea->GetTypeGRorVAorBlock( iSymItem ) ) {
                case SymbolType::Group:
                    iNumItemProgSlots += CreateProgGroup( iLevel, iSymItem, false );
                    break;
                case SymbolType::Block:
                    iNumItemProgSlots += CreateProgBlock( iLevel, iSymItem, false );
                    iItem += GetSymbolEngineBlock(iSymItem).GetVarTs().size();
                    break;
                case SymbolType::Variable:
                    iNumItemProgSlots += CreateProgItem( iLevel, iSymItem, false );
                    break;
                default:
                    // unknown item type - ignored, not added to prog chain
                    if( iSymItem != -1 ) // BINARY_TYPES_TO_ENGINE_TODO remove this if (keeping the assert)
                        ASSERT(false);
                    break;
            }
        }
    }

    if( bDoCreate ) {
        // adding Group' PROCTYPE_PRE-block (for named groups only)
        if( bIsNamedGroup )
            CreateProgSlot( iLevel, CPrSlot::GRslot, ProcType::PreProc, iSymGroup );

        //// if( iNumItemProgSlots > 0 ) {   GHM 20100118
        //// removed so that the preproc and postproc of groups
        //// are executed even when the items within the group don't have code
        //// (for instance in the case that a item that repeats on a record has code
        //// in X000 but not in X)

            // generate head of group-iterator (GIslot/PROCTYPE_PRE)
            int     iHeadIndex = CreateProgSlot( iLevel, CPrSlot::GIslot, ProcType::PreProc, iSymGroup );

            // generate for each item in Group
            for( int iItem = 0; iItem < iNumGroupItems; iItem++ ){
                int     iSymItem = pGroupT->GetItemSymbol( iItem );

                switch( m_pEngineArea->GetTypeGRorVAorBlock( iSymItem ) ) {
                    case SymbolType::Group:
                        CreateProgGroup( iLevel, iSymItem );
                        break;
                    case SymbolType::Block:
                        CreateProgBlock( iLevel, iSymItem );
                        iItem += GetSymbolEngineBlock(iSymItem).GetVarTs().size();
                        break;
                    case SymbolType::Variable:
                        CreateProgItem( iLevel, iSymItem );
                        break;
                    default:
                        // unknown item type - ignored, not added to prog chain
                        break;
                }
            }

            // generate tail of group-iterator (GIslot/PROCTYPE_POST)
            int     iTailIndex = CreateProgSlot( iLevel, CPrSlot::GIslot, ProcType::PostProc, iSymGroup );

            // setup reciprocal indexes in head & tail of group-iterator
            CPrSlot*    pHeadSlot = GetBatchProgSlotAt( iHeadIndex );
            CPrSlot*    pTailSlot = GetBatchProgSlotAt( iTailIndex );

            pHeadSlot->SetSlotIndex( iTailIndex );
            pTailSlot->SetSlotIndex( iHeadIndex );
        //// } GHM 20100118

        // adding Group' POS-block (for named groups only)
        if( bIsNamedGroup )
            CreateProgSlot( iLevel, CPrSlot::GRslot, ProcType::PostProc, iSymGroup );
    }

    return iNumItemProgSlots;
}


int CBatchDriver::CreateProgBlock(int iLevel, int iSymBlock, bool bDoCreate/* = true*/)
{
    const EngineBlock& engine_block = GetSymbolEngineBlock(iSymBlock);
    int iNumItemProgSlots = 0;

    for( VART* pVarT : engine_block.GetVarTs() )
        iNumItemProgSlots += CreateProgItem(iLevel, pVarT->GetSymbolIndex(), false);

    if( bDoCreate )
    {
        if( engine_block.HasProcIndex(ProcType::PreProc) )
            CreateProgSlot(iLevel, CPrSlot::Blockslot, ProcType::PreProc, iSymBlock);

        if( engine_block.HasProcIndex(ProcType::OnFocus) )
            CreateProgSlot(iLevel, CPrSlot::Blockslot, ProcType::OnFocus, iSymBlock);

        for( VART* pVarT : engine_block.GetVarTs() )
            CreateProgItem(iLevel, pVarT->GetSymbolIndex());

        if( engine_block.HasProcIndex(ProcType::KillFocus) )
            CreateProgSlot(iLevel, CPrSlot::Blockslot, ProcType::KillFocus, iSymBlock);

        if( engine_block.HasProcIndex(ProcType::PostProc) )
            CreateProgSlot(iLevel, CPrSlot::Blockslot, ProcType::PostProc, iSymBlock);
    }

    return iNumItemProgSlots;
}


int CBatchDriver::CreateProgItem( int iLevel, int iSymVar, bool bDoCreate ) {
    // CreateProgItem: build the program-strip for an item
    ASSERT( iSymVar > 0 );
    VART* pVarT = VPT(iSymVar);
    int slots_added = 0;

    auto add_slot = [&](ProcType proc_type)
    {
        if( bDoCreate )
            CreateProgSlot(iLevel, CPrSlot::VAslot, proc_type, iSymVar);

        ++slots_added;
    };

    // for SkipStruc, add slots no matter if the proc is present // victor Mar 14, 01

    if( m_pEngineSettings->HasSkipStruc() || pVarT->HasProcIndex(ProcType::PreProc) || pVarT->HasProcIndex(ProcType::OnFocus) )
        add_slot(ProcType::PreProc);

    if( m_pEngineSettings->HasSkipStruc() || pVarT->HasProcIndex(ProcType::PostProc) || pVarT->HasProcIndex(ProcType::KillFocus) )
        add_slot(ProcType::PostProc);

    return slots_added;
}

int CBatchDriver::CreateProgTable( int iLevel, int iCtab, bool bDoCreate )
{
    ASSERT( GetBatchMode() == CRUNAPL_CSTAB );

    // RHF INIC Jan 22, 2003
    // CreateProgTable: build the program-strip for a Table
    ASSERT( iCtab > 0 );
    CTAB*   pCtab = XPT(iCtab);
    int     iNumProgSlots = 0;

    // adding Var' PROCTYPE_TALLY-block only if Tally present
    if( pCtab->HasProcIndex(ProcType::Tally) ) {
        iNumProgSlots++;
        if( bDoCreate )
            CreateProgSlot( iLevel, CPrSlot::CTslot, ProcType::Tally, iCtab );
    }

    return iNumProgSlots;
    // RHF END Jan 22, 2003
}

int CBatchDriverBase::CreateProgSlot( int iLevel, CPrSlot::eSlotType iSlotType, ProcType proc_Type, int iSymbol ) {
    // CreateProgSlot: prepare & add one proc-slot for the program-strip
    // ... formerly 'baddprog', now 'CreateProgSlot'
    CPrSlot*    pNewPrSlot = new CPrSlot;
    ASSERT( pNewPrSlot );

    // setup this program slot
    pNewPrSlot->SetSlotType(iSlotType);
    pNewPrSlot->SetProcType(proc_Type);
    pNewPrSlot->SetSlotSymbol(iSymbol);

    // add to program-strip
    int     iProgIndex = AddToBatchProg( pNewPrSlot );

    // if no proc in this Level yet, set this as the starting proc
    if( m_iLevProg[iLevel] < 0 )
        m_iLevProg[iLevel] = iProgIndex;

    return iProgIndex;
}


int CBatchDriverBase::AddToBatchProg( CPrSlot* pPrSlot ) {  // victor Jun 29, 00
    // AddToBatchProg: add a new prog-slot to the program-strip
    m_apBatchProg.push_back( pPrSlot );
    m_iBatchProgIndex++;

    return m_iBatchProgIndex;
}

CPrSlot* CBatchDriverBase::GetCurBatchProg( void ) {        // victor Jun 29, 00
    // GetCurBatchProg: return a pointer to the current prog-slot in the program-strip
    return m_apBatchProg[m_iBatchProgIndex];
}

CPrSlot* CBatchDriverBase::GetNextBatchProg( int* pIndex ) {// victor Jun 29, 00
    // GetNextBatchProg: advance the current prog-slot and return a pointer to
    CPrSlot*    pPrSlot = NULL;
    int         iPrIndx = -1;

    if( ( m_iBatchProgIndex + 1 ) < (int)m_apBatchProg.size() ) {
        m_iBatchProgIndex++;

        pPrSlot = m_apBatchProg[m_iBatchProgIndex];
        iPrIndx = m_iBatchProgIndex;
    }

    if( pIndex != NULL )
        *pIndex = iPrIndx;

    return pPrSlot;
}

CPrSlot* CBatchDriverBase::GetBatchProgSlotAt( int iIndex ) {   // victor Jun 29, 00
    // GetBatchProgSlotAt: return a pointer to prog-slot for a given position in the program-strip
    CPrSlot*    pPrSlot = NULL;

    if( iIndex >= 0 && iIndex < (int)m_apBatchProg.size() )
        pPrSlot = m_apBatchProg[iIndex];

    return pPrSlot;
}

#ifdef  _DEBUG
void CBatchDriverBase::DumpBatchProg() {
    // dumping program-strip by level
    TRACE( _T("\n* Program-strip:\n") );

    for( int iLevel = 0; iLevel <= (int)MaxNumberLevels; iLevel++ ) {
        int     iIndex = m_iLevProg[iLevel];
        bool    bMoreAtLevel = ( iIndex >= 0 );

        TRACE( _T("... Program-strip for Level %d: starting at [%3d]\n"), iLevel, iIndex );

        for( ; bMoreAtLevel; iIndex++ ) {
            CPrSlot* pPrSlot = m_apBatchProg[iIndex];
            CString csSlotType;

            switch( pPrSlot->GetSlotType() )
            {
                case CPrSlot::LEVELslot:
                    csSlotType = _T("LEVELslot");
                    break;
                case CPrSlot::GRslot:
                    csSlotType = _T("GRslot");
                    break;
                case CPrSlot::GIslot:
                    csSlotType = _T("GIslot");
                    break;
                case CPrSlot::Blockslot:
                    csSlotType = _T("Blockslot");
                    break;
                case CPrSlot::VAslot:
                    csSlotType = _T("VAslot");
                    break;
                    // RHF INIC Jan 22, 2003
                case CPrSlot::CTslot:
                    csSlotType = _T("CTslot");
                    break;
                    // RHF END Jan 22, 2003
                case CPrSlot::EMPTYslot:
                    csSlotType = _T("EMPTYslot");
                    bMoreAtLevel = false;       // no more progs at this level
                    break;
                default:
                    csSlotType = _T("no slot-type");
                    break;
            }

            const TCHAR* proc_type_name = GetProcTypeName(pPrSlot->GetProcType());
            CString csSymbol;
            CString csRecipr;

            if( pPrSlot->GetSlotSymbol() > 0 ) {
                csSymbol.Format( _T("%s (%d)"), NPT(pPrSlot->GetSlotSymbol())->GetName().c_str(), pPrSlot->GetSlotSymbol() );
            }
            else {
                csSymbol.Format( _T("none (%d)"), pPrSlot->GetSlotSymbol() );
            }
            if( pPrSlot->GetSlotIndex() >= 0 ) {
                csRecipr.Format( _T("Reciprocal %d"), pPrSlot->GetSlotIndex() );
            }

            TRACE( _T("   [%3d] %s: %s, %s {%s}\n"), iIndex, csSlotType.GetString(), proc_type_name, csSymbol.GetString(), csRecipr.GetString() );
        }
    }
    TRACE( _T("* Program-strip completed...\n") );
}
#endif//_DEBUG


int CBatchDriverBase::MakeCtabExecOrder() {

    int         iNumWarnings=0;

    CMap<int,int,int,int>   aMapCtab;
    int     iDummy=0, iCtab;

    m_aCtabExecOrder.RemoveAll();

    std::shared_ptr<CTabSet> pTabSet = GetApplication()->GetTabSpec();

    if( pTabSet != NULL ) {
        for( int iTabNum=0; iTabNum < pTabSet->GetNumTables(); iTabNum++ ) {
            CString csTableName=pTabSet->GetTable(iTabNum)->GetName();

            iCtab = m_pEngineArea->SymbolTableSearch(csTableName, { SymbolType::Crosstab });

            if( iCtab > 0 ) {
                aMapCtab.SetAt( iCtab, iDummy );
                m_aCtabExecOrder.Add( iCtab );
            }
            else {
                iNumWarnings++;
            }
        }
    }

    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        iCtab = pCtab->GetSymbolIndex();

        if( aMapCtab.Lookup( iCtab, iDummy ) )
            continue;

        if( pTabSet != NULL )
            iNumWarnings++; // All ctabs should be is pTabSet
        aMapCtab.SetAt( iCtab, iDummy );
        m_aCtabExecOrder.Add( iCtab );
    }

    return iNumWarnings;
}
