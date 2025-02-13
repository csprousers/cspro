//---------------------------------------------------------------------------
//      Wexentry.cpp: Windows Data Entry Kernel and Support methods
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/EXENTRY.H>
#include <engine/Entdrv.h>
#include <engine/EXENTRY.H>
#include <engine/Engine.h>
#include <Cexentry/Entifaz.h>
#include <zUtilO/AppLdr.h>
#include <zCapiO/capi.h>
#include <zCapiO/CapiQuestionManager.h>
#include <ZBRIDGEO/npff.h>
#include <zDataO/DataRepository.h>


void CEntryDriver::reset_lastopenlevels()
{
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        DICX* pDicX = pDicT->GetDicX();
        pDicX->LastOpenLevel = 0;
    }
}


void CEntryDriver::dedriver_start() {
    // --- allowing expansion of cases' tree    <begin> // victor Dec 10, 01
    ASSERT( m_pEngineDriver != NULL );
    FLOW*   pFlowInProcess = m_pEngineDriver->GetFlowInProcess();
    ASSERT( m_pIntDriver != NULL );
    bool    bUsing3D = m_pIntDriver->IsUsing3D_Driver();

    if( bUsing3D ) {
        if( pFlowInProcess == NULL )
            m_pEngineDriver->SetFlowInProcess( m_pEngineDriver->GetPrimaryFlow() );
    }
    // --- allowing expansion of cases' tree    <end>   // victor Dec 10, 01

    GROUPT* pGroupTRoot  = GetGroupTRootInProcess();
    int     iSymLevel;

    ASSERT( Issademode == ADD || Issademode == AUTOADD || Decorrlevl == 1 );

    m_bMustEndEntrySession = false;     // was Deend = FALSE
    SetActiveLevel( 0 );

    // set Pre of level 1 into nextprog                 // victor Jan 11, 00
    iSymLevel = pGroupTRoot->GetLevelSymbol( 0 );

    if( IsModification() ) {                            // victor Jan 28, 00
        SetActiveLevel( Decorrlevl - 1 );

        // set Pre of mod-level to nextprog         // victor Jan 11, 00
        iSymLevel = pGroupTRoot->GetLevelSymbol( Decorrlevl );
    }

    Decurmode = ( Issademode == AUTOADD ) ? ADVMODE : ADDMODE;
}


///////////////////////////////////////////////////////////////////////////////
//
// --> logic' Enter() command execution
//
///////////////////////////////////////////////////////////////////////////////
bool CEntryDriver::InEnterMode() {
    return( m_pEnteredFlow != NULL );
}

int CEntryDriver::GetEnteredSymDic() {
    return( m_pEnteredFlow == NULL ? 0 : m_pEnteredFlow->GetSymDicAt( 0 ) );
}

void CEntryDriver::SetEnterMode( FLOW* pFlow ) {
    m_pEnteredFlow = pFlow;
}

void CEntryDriver::ResetEnterMode() {
    // rebuilt to fully reverse flows -                 // victor Jan 30, 00
    // from the entered flow to its parent flow         // victor Jan 30, 00

    if( m_pEnteredFlow != NULL ) {
//      FLOW*   pParentFlow = m_pEnteredFlow->GetParentFlow();  // TODO: complete this...

        m_pEnteredFlow->RestoreAfterEnter();
        m_pEnteredFlow = NULL;

//      SetEnterMode( pParentFlow );    // TODO: complete this... A new GetMainFlow() to add to InEnterMode() is needed???
    }
}

////////////////////////////////////////////////////////////////////////////////

bool CEntryDriver::CheckIdCollision(int iSymDic)
{
    bool    bCollisionDetected = false;                 // victor Feb 21, 00
    // attn:
    // ... iProcType < 0 means "inconditional checking",
    // ... otherwise it depends on both iProctype & iCurrentObject
    int     iActiveLevel = GetActiveLevel();
    FLOW*   pCurrentFlow = GetFlowInProcess();
    int     iCheckPoint = pCurrentFlow->GetIdCheckPoint( iActiveLevel );
    int     iFlowOrder = 0;
    int     iSymItem   = 0;
    int     iSymParent = 0;

    // scanning keys (initial and current)
    DICT*   pDicT = DPT(iSymDic);
    DICX*   pDicX = DPX(iSymDic);
    csprochar    pszInitialKey[512];
    csprochar    pszCurrentKey[512];

    // setup primary-key of current data
    pDicT->SetPrimaryKey( true );

    // get both the initial and the current primary-key
    pDicT->GetPrimaryKey( pszInitialKey );
    pDicT->GetPrimaryKey( pszCurrentKey, true );

    // evaluate status of both keys
    bool    bIsNewKey = ( !*pszInitialKey );
    bool    bSameKey = ( _tcscmp( pszCurrentKey, pszInitialKey ) == 0 );

    // for either new key or changed key...
    if( bIsNewKey || !bSameKey ) {
//      bCollisionDetected = pDicX->ExistCase( pszCurrentKey );                     // victor Mar 20, 02
        // RHF COM Jul 28, 2003 bCollisionDetected = ( !IsPartial() && pDicX->ExistCase( pszCurrentKey ) ); // victor Mar 20, 02

        // RHF INIC Jul 28, 2003
        if( m_pEntryDriver->IsPartial() ) {
            if( !bSameKey )
                bCollisionDetected = pDicX->GetDataRepository().ContainsCase( pszCurrentKey );
        }
        else
            bCollisionDetected = pDicX->GetDataRepository().ContainsCase( pszCurrentKey );
        // RHF END Jul 28, 2003


        if( bCollisionDetected ) {
            if( bIsNewKey )
                issaerror( MessageType::Warning, 92101, pszCurrentKey );
            else
                issaerror( MessageType::Warning, 92102, pszCurrentKey, pszInitialKey );
        }
    }

    if( !bCollisionDetected ) {
        // for existing case, changed key...
        if( !( bIsNewKey || bSameKey ) ) {
            // issuing warning of changed key
            issaerror( MessageType::Warning, 92103, pszCurrentKey, pszInitialKey );
        }

        // mark CheckPending flag as "done"
        pCurrentFlow->SetIdCheckPending( iActiveLevel, 2 );
    }

    return bCollisionDetected;
}


int CEntryDriver::EvaluateNode( int iLevel ) {          // victor Mar 02, 00
    // Purpose: should be called from CEntryIFaz::C_IsNewNode
    //
    // Return a code evaluating the node condition as follows:
    //
    // a) Nodes having written descendants cannot be discarded (i.e. by means
    //    of a "no write" request) because of integrity constraints:
    //
    // --> 00: a node with at least one written child (coming either from keyboard
    //         or from data-file), no matter its contents
    //
    //
    // b) Nodes with no written descendants are classified depending on its
    //    contents (the term "entered field" means "a field that becomes green"):
    //
    // --> 0x: Nodes with no child
    //         -------------------
    //     01  an empty node: it has no field entered
    //     02  a virgin node: the only fields entered are either Persistent or Protected
    //     03  a non-virgin node: there is at least one (not Persistent nor Protected) field entered
    //
    // --> 1x: Empty nodes with a child
    //         ------------------------
    //     11  has an empty child
    //     12  has a virgin child
    //     13  has a non-virgin child
    //
    // --> 2x: Virgin nodes with a child
    //         -------------------------
    //     21  has an empty child
    //     22  has a virgin child
    //     23  has a non-virgin child
    //
    // --> 3x: Non-virgin nodes with a child
    //         -----------------------------
    //     31  has an empty child
    //     32  has a virgin child
    //     33  has a non-virgin child
    int     iNodeCondition = 0;         // assuming no written descendants

    if( !LevCtGetWrittenSons( iLevel ) ) {
        int     iOwnInfo = 0;           // the status of this node
        int     iSonInfo = 0;           // the status of his son, if any

        // get the status of this node
        switch( LevCtGetInfo( iLevel ) ) {
            case NoData:
                iOwnInfo = 1;           // empty
                break;
            case NoInfo:
                iOwnInfo = 2;           // virgin
                break;
            case HasInfo:
                iOwnInfo = 3;           // non-virgin
                break;
            default:
                iOwnInfo = 1;           // assuming empty
                break;
        }

        if( LevCtHasSon( iLevel ) ) {
            // get the status of his son
            switch( LevCtGetSonInfo( iLevel ) ) {
                case NoData:
                    iSonInfo = 1;       // empty
                    break;
                case NoInfo:
                    iSonInfo = 2;       // virgin
                    break;
                case HasInfo:
                    iSonInfo = 3;       // non-virgin
                    break;
                default:
                    iSonInfo = 1;       // assuming empty
                    break;
            }
        }

        // final evaluation
        if( !iSonInfo )                 // no child: giving 0x
            iNodeCondition = iOwnInfo;
        else                            // children: giving nx
            iNodeCondition = 10 * iOwnInfo + iSonInfo;
    }

    return iNodeCondition;
}
////////////////////////////////////////\\/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


void CEntryDriver::doend() {
    // fin standard, sacado desde F9-Quit (ya separado en SCO/Unix)
    // TODO: revisar si corresponde 'endfiles', parece repetido !!!

    // wipe out levels starting with active level, then the parent levels
    int     iActiveLevel = GetActiveLevel();

    for( int iLevelToWipe = iActiveLevel; iLevelToWipe > 0; iLevelToWipe-- )
        DeInitLevel( iLevelToWipe );

    // set the active level to -1 (session ended)
    SetActiveLevel( -1 );

    m_bMustEndEntrySession = true;      // was Deend = TRUE

    if( IsModification() ) {                            // RHF 25/9/98
        dedriver_start();                               // RHF 25/9/98
    }                                                   // RHF 25/9/98
}                                       // RHF 1/7/94


///////////////////////////////////////////////////////////////////////////////
//
// CEngineDriver::methods
//
///////////////////////////////////////////////////////////////////////////////

void CEngineDriver::initextdi()
{
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetSubType() == SymbolSubType::External )
        {
            for( int iSymSec = pDicT->SYMTfsec; iSymSec >= 0; iSymSec = SPT(iSymSec)->SYMTfwd )
                initsect(iSymSec);
        }
    }
}


//////////////////////// data files checking <end> ////////////////////////////
//---------------------------------------------------------------------------
//    Entry processor null functions & symbols
//---------------------------------------------------------------------------
//  Fix problem with strchr in help_linesexpand( int* text_maxlen )...
//  Don't use console functions in Windows

// Tables-CrossTabs functions
double CIntDriver::DoXtab( CTAB *, double, int, LIST_NODE*  ){ return (double) 0; }
double CIntDriver::DoOneXtab( CTAB* pCtab, double dWeight, int iTabLogicExpr, LIST_NODE*  ) { return (double) 0; }
void CIntDriver::CtPos( CTAB*, int, int*, CSubTable*, CCoordValue*, bool  ){}
#ifdef WIN_DESKTOP //TODO_PORT conflicts with inttbl.cpp check which one to use
double CIntDriver::val_coord( int i_node, double i_coord ){ return -1; }
double CIntDriver::val_high( void ) { return (double) 0; }    // BMD 13 Oct 2005
#endif

#include <engine/Ctab.h>

double CIntDriver::tblcoord(int, int) { return( DEFAULT ); }
double CIntDriver::excpttbl(int) { return( DEFAULT ); }
double CIntDriver::extblsum(int) { return( DEFAULT ); }
double CIntDriver::extblmed(int) { return( DEFAULT ); }

#ifndef WIN_DESKTOP
double CIntDriver::exxtab(int) { assert(0); return DEFAULT; }
double CIntDriver::exupdate(int) { assert(0); return DEFAULT; }
#endif

#ifdef USE_BINARY
#else
// Break nulls
bool CTbd::breakinit( const TCHAR* ) {return( FALSE );}
void CTbd::breakend(void) { return; }
void CTbd::breakclose(void) { return; }
void CTbd::breakcheckid() { return; }
void CTbd::breakmakeid( csprochar * ) { return; }
void CTbd::breaksave( const TCHAR*, CTAB * ) { return; }
short CTbdFile::GetTableNum( const csprochar*, int  ) { return 0; }
int  CTbdFile::SetTableNum( csprochar* , short, int  ) { return 0; }
#endif


// compiling forbidden objects
#ifdef WIN_DESKTOP

#include <engine/COMPILAD.H>
int  CEngineCompFunc::compctab( int, CTableDef::ETableType )    { return( SetSyntErr(601), 0 ); }

void CEngineCompFunc::CompileSubTablesList( CTAB* pCtab, int* pNodeBase[TBD_MAXDIM],
                                           std::vector<int>& aSubTables, bool bCheckUsed,
                                           std::vector<CLinkSubTable>* pLinkSubTables
                                           ) {}


int CEngineCompFunc::compexport()                { return( SetSyntErr(31002), 0 ); }

void CIntDriver::ExpWriteThisExport( void )  {}

#endif

////////////////////////////////////////////////////////////////////////////////



// --- LevCt: controlling levels operation ---- <begin> // Feb 29, 00
void CEntryDriver::LevCtInitLevel( int iLevel ) {
    if( InEnterMode() )
        return;

    m_aNodeInfo          [iLevel]     = NoData;
    m_aNodeSource        [iLevel]     = Keyboard;
    m_aCurrentSonNode    [iLevel]     = NoSon;
    m_iWrittenSons       [iLevel]     = 0;

    // ---- forced-no-write requested:
    m_bForcedNoWrite     [iLevel]     = false;
}

void CEntryDriver::LevCtStartLevel( int iLevel ) {
    if( InEnterMode() )
        return;

    // to be used each time a pre-level is executed
    LevCtInitLevel( iLevel );

    // updates the parent-node info regarding this son
    if( iLevel > 0 )                // NoData alerts of (this) son' existence
        LevCtSetSonInfo( iLevel - 1, NoData );
}

void CEntryDriver::LevCtSetInfo( int iLevel, eNodeInfo NewInfo ) {
    if( InEnterMode() )
        return;

    bool        bCanSet = false;
    eNodeInfo   CurInfo = LevCtGetInfo( iLevel );

    if( CurInfo == NoSon ) {            // invalid status - change to NoData
        // TODO: send an error message telling of this error???
        CurInfo = m_aNodeInfo[iLevel] = NoData;
    }

    switch( CurInfo ) {                 // attn: NoSon is never accepted below
        case NoData:
            bCanSet = ( NewInfo == NoInfo || NewInfo == HasInfo );
            break;
        case NoInfo:
            bCanSet = ( NewInfo == HasInfo );
            break;
        default:                        // case HasInfo:
            break;
    }

    if( bCanSet ) {
        m_aNodeInfo[iLevel] = NewInfo;

        // updates the parent-node info regarding this son
        if( iLevel > 0 )                // NoData alerts of (this) son' existence
            LevCtSetSonInfo( iLevel - 1, NewInfo );
    }
}

void CEntryDriver::LevCtSetSonInfo( int iLevel, eNodeInfo NewInfo ) {
    if( InEnterMode() )
        return;

    bool        bCanSet = false;
    eNodeInfo   CurInfo = LevCtGetSonInfo( iLevel );

    switch( CurInfo ) {
        case NoSon:
            bCanSet = ( NewInfo != NoSon );
            break;
        case NoData:
            bCanSet = ( NewInfo == NoInfo || NewInfo == HasInfo );
            break;
        case NoInfo:
            bCanSet = ( NewInfo == HasInfo );
            break;
        default:                        // case HasInfo:
            break;
    }

    if( bCanSet )
        m_aCurrentSonNode[iLevel] = NewInfo;
}

void CEntryDriver::LevCtAddWrittenSon( int iLevel ) {
    if( InEnterMode() )
        return;

    m_iWrittenSons[iLevel] += 1;
}

// --- LevCt: controlling levels operation ---- <end>   // Feb 29, 00


void CEntryDriver::BuildQuestMgr()
{
    m_pQuestMgr = std::make_shared<CapiQuestionManager>();
    m_pApplication->SetCapiQuestionManager(m_pQuestMgr);

    // the question text will be read later when reading from a .pen file
    if( m_pApplication->GetAppLoader() != nullptr && m_pApplication->GetAppLoader()->GetBinaryFileLoad() )
        return;

    if( !m_pApplication->GetQuestionTextFilename().IsEmpty() )
        m_pQuestMgr->Load(CS2WS(m_pApplication->GetQuestionTextFilename()));
}

// RHF END Nov 07, 2002

#ifdef WIN_DESKTOP
CTAB* CTAB::pCurrentCtab = NULL;

void CEngineCompFunc::GetSubTableList( int* pNodeBase[TBD_MAXDIM],
                                      std::vector<int>& aSubTables, bool bCheckUsed,
                                      int   iDimType[TBD_MAXDIM],
                                      int   iVar[TBD_MAXDIM][TBD_MAXDEPTH],
                                      int   iSeq[TBD_MAXDIM][TBD_MAXDEPTH], int iTheDimType ) {}

int CEngineCompFunc::CompileOneSubTableDim( int* pNodeBase[TBD_MAXDIM], int iDimNum, int iDim[TBD_MAXDIM], int iVar[TBD_MAXDIM][TBD_MAXDEPTH], int iSeq[TBD_MAXDIM][TBD_MAXDEPTH],
                              int* iCatValues/*=NULL*/,
                              int* iCatValuesSeq/*=NULL*/
                              ) { return 0; }

int CEngineCompFunc::ctopernode( int iLeft, int iRight, int iNodeType ) { return -1;}
#ifndef USE_BINARY
bool CEngineCompFunc::ScanTables() { ASSERT(0); return false; }
CMap<int,int,CString,CString> CTAB::m_aExtraNodeInfo;
#endif

bool CEngineCompFunc::CheckProcTables() {return false;}

#ifndef USE_BINARY
void CExport::ExportClose(void) {}
void CExport::ExportDescriptions(void){}
bool CExport::ExportOpen(bool* bDeleteWhenFail){return false;}

void CExport::RemoveFiles(){}

void CExport::MakeCommonRecord( CDictRecord* pDictCommonRecord, int iFromLevel, int iToLevel ) { ASSERT(0); }
CString CExport::GetDcfExpoName() const { ASSERT(0); return CString(); }

int CTRANGE::getNumCells() { ASSERT(0); return 0; } // rcl, Jun 2005
int CTRANGE::getNumCells(double rLow, double rHigh, int iCollapsed) { ASSERT(0); return 0; } // rcl, Jun 2005
bool CTRANGE::fitInside( double rValue ) { ASSERT(0); return false; }
#endif // !USE_BINARY

#endif
