//---------------------------------------------------------------------------
//  File name: CFlow.cpp
//
//  Description:
//          Header for flow descriptors
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Dec 99   vc      Created for IMSA' 2nd release of Mar/00
//              30 Jan 00   vc      Expansion to best manage Enter'ed flows
//                                  (xxxJustReturned data/methods were added)
//              21 Feb 00   vc      Adding checkpoint for id-fields
//              20 Jul 00   vc      Tailoring for recent methods added outside this class
//              08 Jan 01   vc      Adding link to new FlowCore
//
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "CFlow.h"
#include <engine/Engine.h>
#include <engine/Engdrv.h>
#include "FlowCore.h"                                   // victor Jan 08, 01


const Logic::SymbolTable& CSymbolFlow::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
/////////////////////////////////////////////////////////////////////////////

CSymbolFlow::CSymbolFlow(std::wstring name, CEngineArea* pEngineArea)
    :   Symbol(std::move(name), SymbolType::Pre80Flow),
        m_pEngineArea(pEngineArea),
        m_engineData(&m_pEngineArea->GetEngineData())
{
    // --- related objects
    m_pFormFile     = NULL;
    m_pFlowCore     = new CFlowCore;                    // victor Jan 08, 01
    m_pFlowCore->SetSymbolFlow( this );                 // victor Jan 08, 01

    // --- basic info
    SetNumDim(0);

    m_iNumVisible   = 0;
    m_iNumInvisible = 0;
    m_pGroupTRoot   = NULL;

    // --- checkpoint for verifying id-fields
    for( int iLevel = 0; iLevel <= (int)MaxNumberLevels; iLevel++ ) {
        m_iIdCheckFlowOrder[iLevel] = 0;
        m_iIdCheckPending  [iLevel] = 0;
    }

    // --- parent flow and all its execution environment (saved & restored)
    m_pParentFlow   = NULL;

    // --- info saved to allow later returning to this flow
    m_Progbase      = NULL;
    m_Prognext      = 0;
    m_ProgType      = -1;
    m_ExLevel       = -1;
    m_ExSymbol      = 0;

    // --- additional info from entry driver
    m_Decurlevl     = 0;
}


CSymbolFlow::~CSymbolFlow()
{
    delete m_pFlowCore;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- basic info
//
/////////////////////////////////////////////////////////////////////////////

void FLOW::AddDic( int iSymDic ) {      // attach a Dic to this Flow
    DICT*   pDicT = DPT(iSymDic);

    m_aSymDic.emplace_back(iSymDic);
    pDicT->SetFlow( this );

    SetNumDim(std::max( GetNumDim(), pDicT->GetNumDim() ) );
}

bool FLOW::IsMemberDic( int iSymDic ) {
    bool    bFound = false;

    for( int iNumDic = 0; !bFound && iNumDic < GetNumberOfDics(); iNumDic++ )
        bFound = ( iSymDic == GetSymDicAt( iNumDic ) );

    return bFound;
}

void FLOW::AddForm( int iSymForm ) {    // attach a Form to this Flow
    m_aSymForm.emplace_back(iSymForm);
}

bool FLOW::IsMemberForm( int iSymForm ) {
    bool    bFound = false;

    for( int iNumForm = 0; !bFound && iNumForm < GetNumberOfForms(); iNumForm++ )
        bFound = ( iSymForm == GetSymFormAt( iNumForm ) );

    return bFound;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- checkpoint for verifying id-fields
//
/////////////////////////////////////////////////////////////////////////////

void FLOW::BuildIdCheckPoints() {                       // victor Feb 21, 00
    if( m_pEngineArea->m_pEngineDriver->Issamod != ModuleType::Entry )
        return; // RHF Mar 14, 2000
    // set id-fields checkpoint for Level 1
    CEntryDriver*  pEntryDriver = (CEntryDriver*) (m_pEngineArea->m_pEngineDriver);
    GROUPT* pGroupTRoot = pEntryDriver->GetGroupTRootInProcess();
    bool    bIsPrimaryFlow = ( GetSubType() == SymbolSubType::Primary );
    int     iNumLevels = pGroupTRoot->GetLevelGPT(0)->GetNumItems();

    for( int iLevel = 0; iLevel <= iNumLevels; iLevel++ ) {
        SetIdCheckPoint( iLevel, 0 );

        if( bIsPrimaryFlow ) {
            int     iSymLevel = pGroupTRoot->GetLevelSymbol( iLevel );

            ScanIdCheckInGroup( iSymLevel );
        }
    }
}

void FLOW::ScanIdCheckInGroup( int iSymGroup ) {        // victor Feb 21, 00
    GROUPT* pGroupT = GPT(iSymGroup);
    int     iLevel = pGroupT->GetLevel();
    int     iNumGroupItems = pGroupT->GetNumItems();

    for( int iItem = 0; iItem < iNumGroupItems; iItem++ ) {
        int     iSymItem = pGroupT->GetItemSymbol( iItem );
        int     iFlowOrder = pGroupT->GetFlowOrder( iItem );
        VART*   pVarT;
        SECT*   pSecT;
        bool    bInCommon;

        switch( m_pEngineArea->GetTypeGRorVA( iSymItem ) ) {
            case SymbolType::Variable:
                pVarT = VPT(iSymItem);
                pSecT = pVarT->GetSPT();
                bInCommon = pSecT->IsCommon();

                // the field is in common:
                if( pVarT->IsInAForm() && bInCommon ) {
                    // ... increase IdCheckPoint to this FlowOrder
                    if( GetIdCheckPoint( iLevel ) < iFlowOrder )
                        SetIdCheckPoint( iLevel, iFlowOrder );
                }
                break;

            case SymbolType::Group:
                ScanIdCheckInGroup( iSymItem );
                break;

            default:
                // unknown item type - ignored
                break;
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// --- saving/restoring environment of parent flow
//
/////////////////////////////////////////////////////////////////////////////

void FLOW::RestoreAfterEnter()
{
    ASSERT( m_pEngineArea->m_pEngineDriver->Issamod == ModuleType::Entry );

    // reinstall parent flow as "in process"
    m_pEngineArea->m_pEngineDriver->SetFlowInProcess( m_pParentFlow );
    m_pParentFlow = NULL;

    // processing info: restore the environment prior to enter this flow

    // info saved to allow later returning to this flow
    m_pEngineArea->m_pEngineDriver->m_pIntDriver->m_iProgType  = m_ProgType;
    m_pEngineArea->m_pEngineDriver->m_pIntDriver->m_iExLevel   = m_ExLevel;
    m_pEngineArea->m_pEngineDriver->m_pIntDriver->m_iExSymbol  = m_ExSymbol;

    m_pEngineArea->m_pEngineDriver->m_pIntDriver->m_iStopExec = FALSE; // RHF Dec 19, 2000 Fix problem with enter. Enter was not executing the instructions after ENTER
    //m_pEngineArea->m_pEngineDriver->m_pIntDriver->SkipStmt = m_SkipStmt;
    //m_pEngineArea->m_pEngineDriver->m_pIntDriver->ExitProc = m_ExitProc;
    //m_pEngineArea->m_pEngineDriver->m_pIntDriver->m_bStopProc = m_StopProc;
    //m_pEngineArea->m_pEngineDriver->m_pIntDriver->SkipCase = m_SkipCase;

    // additional info from entry driver
    CEntryDriver*  pEntryDriver = (CEntryDriver*) (m_pEngineArea->m_pEngineDriver);

    pEntryDriver->SetActiveLevel(  m_Decurlevl );
}
