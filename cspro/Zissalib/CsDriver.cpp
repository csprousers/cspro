//---------------------------------------------------------------------------
//  File name: CsDriver.cpp
//
//  Description:
//          Implementation for the 3-D driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              16 Jan 01   vc      Created for final expansion to 3 dimensions
//              30 Jan 01   vc      Finishing basic features, including occurrences updating
//              27 Apr 01   vc      Enhancements following preliminary run tests
//              10 May 01   vc      Makeup of basic functionality, introducing hands-shaking controls
//              21 May 01   vc      Adding DeFld' version of current object (HANDSHAKING3D),
//                                  plus methods for managing field values and colors
//              28 May 01   vc      Adding progress-bitmap management
//              30 May 01   vc      Adding support for level' processing
//              07 Jun 01   vc      Adding more controls on Level-tails, improving relationship with RunCase
//              14 Jun 01   RHF+vc  Splitting constructor in two parts (constant and reentrant data),
//                                  refining relationship' calculation
//              18 Jun 01   RHF     Add evaluation of "same branch"
//              20 Jun 01   RHF     Add trimming of empty occurrences
//              21 Jun 01   vc      New methods to manage pending-advances, dealing with volatile "field color" and "origin" marks, integrating occurrences' trim
//              10 Dec 01   vc      Full revision and remake
//              20 Feb 02   RHF+vc  Final check for 1st delivery
//              12 Mar 02   vc      Activating detection of IdCollision (for case-ids only)
//              25 Mar 02   vc      Add FurnishGroupHead/FurnishFirstField methods, and modify SolveReenter to allow for reenter to GroupHead
//
//---------------------------------------------------------------------------
#include "StdAfx.h"

//typedef unsigned char   byte;

#include "CsDriver.h"
#include "CFlow.h"
#include "CFlAdmin.h"                                   // victor Aug 02, 01
#include <engine/Engdrv.h>
#include <engine/ProgramControl.h>
#include <Cexentry/Entifaz.h>
#include <engine/Engine.h>
#include <engine/IntDrive.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/TraceMsg.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/Messages.h>
#include <zCaseO/Case.h>
#include <zParadataO/Logger.h>
#include <ZBRIDGEO/npff.h>
#include <zLogicO/SpecialFunction.h>


#ifdef WIN_DESKTOP
#define RTRACE DebugMessage
#else
#define RTRACE(...)
#endif
#ifdef NL
#undef NL
#endif

#define NL


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


const Logic::SymbolTable& CsDriver::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction
//
/////////////////////////////////////////////////////////////////////////////

CsDriver::CsDriver( void ) {
    // ... splitted in two (constant and reentrant data)// RHF+vc Jun 14, 01

    // --- current flow, current object-in-flow, way
    SetFlAdmin( NULL );                 // the Flow-administrator
    SetFlow( NULL );                    // current FLOW (and attached FlowCore)

    // --- operation control
    ResetDoorCondition();
    ResetSourceOfNodeAdvance();                         // victor Feb 23, 02

    // --- requests (and targets) from Interface and Logic
    // (2) identification of the request
    BuildRequestValid();

    // --- miscellaneous constants
    m_iNumRequestNature  = CsDriver::None;
    m_iNumAtomTypes      = (int)CFlowAtom::AtomType::BeyondStrip;

    m_iNumConditions     = 10;           // in each AtomType
    m_iDynamicBitmapSize = ( m_iNumRequestNature * m_iNumAtomTypes ) * m_iNumConditions;

    // - definitions for bit operations
    SetupBitMasks();

    // --- static bitmaps for flow-strip atoms
    BuildPresetStaticBitmaps( true );   // forward
    BuildPresetStaticBitmaps( false );  // backward

    // --- dynamic bitmaps for flow-strip atoms         // victor May 10, 01
    m_aDynamicPathOn  = (byte*) calloc( m_iDynamicBitmapSize, sizeof(byte) );
    m_aDynamicPathOff = (byte*) calloc( m_iDynamicBitmapSize, sizeof(byte) );

    ASSERT( m_aDynamicPathOn != NULL && m_aDynamicPathOff != NULL );
    BuildPresetDynamicBitmaps( true );  // PathOn
    BuildPresetDynamicBitmaps( false ); // PathOff

    // --- engine links
    m_pEngineDriver     = NULL;
    m_pEngineArea       = NULL;
    m_pIntDriver        = NULL;
    m_pEntryDriver      = NULL;
    m_engineData        = nullptr;
    m_bSolveInterEndLevel = false; //SAVY For endlevel postproc Aug 01,2003
    m_bKillFocus = false;
    m_bMidColor = false;

    // initialize reentrant conditions for a session    // RHF+vc Jun 14, 01
    InitSessionConditions();                            // RHF+vc Jun 14, 01
}

// hasMaxDEField
// RTODO
// should be a method of GROUPT class
// RCL, Sept 24, 2004
bool hasMaxDEField(GROUPT* pGroupT)
{
    CDEGroup* pCDEGroup = pGroupT->GetCDEGroup();
    bool bRet = (pCDEGroup != 0) && (pCDEGroup->GetMaxDEField() != 0);

    return bRet;
}

CsDriver::~CsDriver( void ) {
    // check that all pending advances have been solved // victor Jun 21, 01
    if( GetNumOfPendingAdvances() > 0 )                 // victor Jun 21, 01
        ASSERT( 0 );                    // can't be     // victor Jun 21, 01

    // destroying request validation array              // victor May 10, 01
    for( int iRequest = 0; iRequest < (int)m_aRequestValid.size(); iRequest++ ) {
        RequestValid*   pRequest = m_aRequestValid[iRequest];

        delete pRequest;
    }

    // destroying dynamic bitmaps                       // victor May 10, 01
    if( m_aDynamicPathOn  != NULL ) {
        free( m_aDynamicPathOn );
        m_aDynamicPathOn  = NULL;
    }
    if( m_aDynamicPathOff != NULL ) {
        free( m_aDynamicPathOff );
        m_aDynamicPathOff = NULL;
    }
}

void CsDriver::InitSessionConditions( void ) {          // RHF+vc Jun 14, 01
    // InitSessionConditions: initialize reentrant conditions for a session
    // ... remark - be careful, should be used only following an end-of-Level-zero (fatal if used amid a session)

    if( m_pFlowCore != NULL )
        RestartFlow();                  // --- restart the flow-strip

    // --- current flow, current object-in-flow, way
    SetCurObject( NULL );               // current object-in-flow
    SetForwardWay( true );
    setExtractedObject( 0 );            // object-in-flow extracted from a given atom by ExtractInfoFromAtom

    // --- operation control
    SetLastItemEntered( NULL );         // the most recent item-atom entered in the flow
    SetRefreshGroupOccsLimit( 0 );      // "0-no limit"
    SetLevelTailReached();                              // victor Jun 07, 01
    m_bNoInputField      = false;       // is almost always OFF
    m_bRemakeOrigin      = true;        // is almost always ON
    m_bEnterStarted      = false;       // is almost always false
    m_iEnterReturned     = 0;           // is almost always "no"
    SetTargetNode( CEntryDriver::NodeAdvanceNone );     // victor Dec 10, 01

    // --- requests (and targets) from Interface and Logic
    // (2) identification of the request
    SetRequestSource( true );           // from logic
    SetRequestNature( CsDriver::None );

    m_pRequestOrigin     = NULL;        // flow-atom "origin" of the request
    m_iRequestOriginIndex = 0;          // flow-atom index "origin" of the request
    SetRequestEventType( CsDriver::EventNone );
    // (3) requested from Interface and Logic
    SetNodeBoundTarget();               // target is a node-bound
    m_TgtAdvanceTo.SetEmpty();          // target for advance-to
    m_TgtSkipTo.SetEmpty();             // target for skip-to
    m_TgtReenter.SetEmpty();            // target for reenter
    // (4) requested from Logic only
    m_iTgtSymFlow        = 0;           // target for enter-flow
    m_iTgtAdvanceToNext  = 0;           // target for advance-to-next (an iSymVar)
    m_iTgtSkipToNext     = 0;           // target for skip-to-next (an iSymVar)
    // (5) requested from Interface only
    AtEndOccur_SetEnterField( false );  // behavior requested to end the current occurrence
    AtEndGroup_SetEnterField( false );  // behavior requested to end the current group
    AtEndLevel_SetParameters( false, -1, false ); // behavior requested to manage the EndLevel
    AtSkip_SetParameters( false, false ); // RHF Dec 30, 2003
    // (6) other requests from Interface
    //  --- none ---                                    // victor May 10, 01

    // --- progress bitmap (event-procs execution)      // victor May 28, 01
    SetProgressBitmapOn();                              // victor May 28, 01
}

/////////////////////////////////////////////////////////////////////////////
//
// --- current flow, current object-in-flow, way
//
/////////////////////////////////////////////////////////////////////////////

void CsDriver::SetFlAdmin( CFlAdmin* pFlAdmin ) {       // victor Aug 02, 01
    m_pFlAdmin = pFlAdmin;
}

CFlAdmin* CsDriver::GetFlAdmin( void ) {                // victor Aug 02, 01
    return m_pFlAdmin;
}

void CsDriver::SetFlow( CSymbolFlow* pFlow ) {
    m_pFlow = pFlow;
    m_pFlowCore = ( pFlow != NULL ) ? pFlow->GetFlowCore() : NULL;
}

C3DObject* CsDriver::ExtractInfoFromAtom( CFlowAtom* pAtom ) {
    C3DObject*  pExtractedObject = NULL;

    if( pAtom != NULL ) {
        pExtractedObject = &m_ExtractedObject;

        switch( pAtom->GetAtomType() ) {
            case CFlowAtom::AtomType::GroupHead:
                ExtractInfoFromGroupHeadAtom( pAtom );
                break;

            case CFlowAtom::AtomType::GroupTail:
                ExtractInfoFromGroupTailAtom( pAtom );
                break;

            case CFlowAtom::AtomType::HTOcc:
                ExtractInfoFromHTOccAtom( pAtom );
                break;

            case CFlowAtom::AtomType::BlockHead:
                ExtractInfoFromBlockHeadAtom(pAtom);
                break;

            case CFlowAtom::AtomType::BlockTail:
                ExtractInfoFromBlockTailAtom(pAtom);
                break;

            case CFlowAtom::AtomType::Item:
                ExtractInfoFromItemAtom( pAtom );
                break;

            case CFlowAtom::AtomType::BeforeStrip:
            case CFlowAtom::AtomType::BeyondStrip:
                setExtractedObject( 0 );
                pExtractedObject = NULL;
                break;

            default:
                ASSERT( 0 );                // shouldn't be
                setExtractedObject( 0 );
                pExtractedObject = NULL;
                break;
        }
    }

    return pExtractedObject;
}

void CsDriver::ExtractInfoFromGroupHeadAtom( CFlowAtom* pAtom ) {
    CFlowGroupHead* pGroupHead = (CFlowGroupHead*) pAtom;
    int             iSymbol    = pGroupHead->GetSymbol();
    CNDIndexes      theIndex = ( IsForwardWay() ) ? pGroupHead->GetIndexFwd() : pGroupHead->GetIndexBwd();

    setExtractedObject( iSymbol, theIndex );
}

void CsDriver::ExtractInfoFromGroupTailAtom( CFlowAtom* pAtom ) {
    CFlowGroupTail* pGroupTail = (CFlowGroupTail*) pAtom;
    int             iSymbol    = pGroupTail->GetSymbol();
    CNDIndexes      theIndex   = pGroupTail->GetIndex();

    setExtractedObject( iSymbol, theIndex );
}

void CsDriver::ExtractInfoFromHTOccAtom( CFlowAtom* pAtom ) {
    CFlowHTOcc*         pHTOcc   = (CFlowHTOcc*) pAtom;
    int                 iSymbol  = pHTOcc->GetSymbol();
    CNDIndexes          theIndex = ( IsForwardWay() ) ? pHTOcc->GetIndexFwd() : pHTOcc->GetIndexBwd();
    CFlowAtom::OccType  xOccType = pHTOcc->GetOccType();
    int                 iKindUse = ( xOccType == CFlowAtom::OccType::Head  ) ? 1 :
                                   ( xOccType == CFlowAtom::OccType::Tail  ) ? 2 : 0;


    setExtractedObject( iSymbol, theIndex );

    RefreshGroupOccs( iSymbol, theIndex, iKindUse );
}


void CsDriver::ExtractInfoFromBlockHeadAtom(CFlowAtom* pAtom)
{
    CFlowBlockTail* pFlowBlockTail = (CFlowBlockTail*)pAtom;
    int iSymbol = pFlowBlockTail->GetSymbol();
    CNDIndexes theIndex = pFlowBlockTail->GetIndex();

    setExtractedObject(iSymbol, theIndex);
}

void CsDriver::ExtractInfoFromBlockTailAtom(CFlowAtom* pAtom)
{
    CFlowBlockTail* pFlowBlockTail = (CFlowBlockTail*)pAtom;
    int iSymbol = pFlowBlockTail->GetSymbol();
    CNDIndexes theIndex = pFlowBlockTail->GetIndex();

    setExtractedObject(iSymbol, theIndex);
}


void CsDriver::ExtractInfoFromItemAtom( CFlowAtom* pAtom ) {
    CFlowItem* pFlowItem = (CFlowItem*) pAtom;
    int        iSymbol   = pFlowItem->GetSymbol();
    CNDIndexes theIndex  = pFlowItem->GetIndex();

    setExtractedObject( iSymbol, theIndex );

    // remark - item-atoms "inherit" the indexes already set by other atoms
    //          and do not change the ocurrences of its owner-group
}

/////////////////////////////////////////////////////////////////////////////
//
// --- operation control
//
/////////////////////////////////////////////////////////////////////////////
bool CsDriver::SetDoorCondition( DoorCondition xNewCondition ) {// victor Feb 20, 02
    // SetDoorCondition: set a given condition on the "door" controlling the execution of level-0' procs
    // remark - "door condition" must behave as a closed a ring (Locked --> Open --> Closed)
    bool    bDone = false;

    switch( DoorConditionValue() ) {
        case CsDriver::Locked:     // ... is Locked, can only set to Open (Pre-0 already executed)
            bDone = ( xNewCondition == CsDriver::Open );
            break;
        case CsDriver::Open:       // ... is Open, can only set to Closed (Post-0 already executed) or re-opened in Add mode
            bDone = ( xNewCondition == CsDriver::Closed ||
                      xNewCondition == CsDriver::Open );
            break;
        case CsDriver::Closed:     // ... is Closed, can only set to Locked (nothing executed)
            bDone = ( xNewCondition == CsDriver::Locked );
            break;
        default:
            ASSERT( 0 );                // can't be
            break;
    }
    ASSERT( bDone );

    if( bDone )
        m_eDoorCondition = xNewCondition;

    return bDone;
}

void CsDriver::SetSourceOfNodeAdvance( void ) {         // victor Feb 23, 02
    int         iSourceNode = -1;       // assuming the source will turn void
    CsDriver*   pCsDriver = this; // TODO should be the primary CsDriver!!!
    if( pCsDriver != NULL && GetTargetNode() != CEntryDriver::NodeAdvanceNone )
    {
        int iAtomLevel = pCsDriver->GetCurrAtomLevel();
        Pre74_CaseLevel* pCaseLevel = m_pEntryDriver->GetCaseLevelNode(iAtomLevel);
        iSourceNode = m_pEntryDriver->GetNodeNumber(pCaseLevel);
    }

    m_iSourceOfNodeAdvance = iSourceNode;
}

void CsDriver::SetTargetNode( int iNode ) {             // victor Dec 10, 01
    // remark - iNode must follow conventions of enum CEntryDriver::NodeAdvance
    if( iNode < CEntryDriver::NodeAdvanceNone ) {
        ASSERT( 0 ); // can't be
        m_iTargetNode = CEntryDriver::NodeAdvanceNone;
    }
    else if( iNode > CEntryDriver::NodeAdvanceToEndOfCase ) {
        ASSERT( 0 ); // can't be
        m_iTargetNode = CEntryDriver::NodeAdvanceToEndOfCase;
    }
    else
        m_iTargetNode = iNode;
    SetSourceOfNodeAdvance();
}

GROUPT* CsDriver::GetGroupTRoot( void ) {               // victor Mar 07, 02
    return GetFlow()->GetGroupTRoot();
}

int CsDriver::GetMaxLevel( void ) {                     // victor Mar 07, 02
    return GetFlowCore()->GetMaxLevel();
}

int CsDriver::GetSymbolOfLevel( int iLevel ) {          // victor Mar 07, 02
    CFlowCore*  pFlowCore  = GetFlowCore();
    int         iMaxLevel  = pFlowCore->GetMaxLevel();
    bool        bDone      = ( iLevel >= 0 && iLevel <= iMaxLevel );
    int         iSymbol    = 0;

    if( bDone ) {
        // set the GroupHead atom of the requested level as current and retrieve the symbol
        int         iCurrAtom = GetCurrAtomIndex(); // save the current-atom
        int         iAtom     = pFlowCore->GetLevelHeadIndex( iLevel );

        SetCurrAtomIndex( iAtom );

        CFlowAtom*  pAtom = GetCurrAtom();
        CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
        ASSERT( xAtomType == CFlowAtom::AtomType::GroupHead );

        if( xAtomType == CFlowAtom::AtomType::GroupHead )
            iSymbol = ((CFlowGroupHead*) pAtom)->GetSymbol();
        SetCurrAtomIndex( iCurrAtom );  // restore the current-atom
    }

    return iSymbol;
}

int CsDriver::GetLevelOfCurObject( void ) {             // victor Mar 07, 02
    CFlowCore*  pFlowCore  = GetFlowCore();
    CFlowAtom*  pAtom      = GetCurrAtom();
    int         iAtomLevel = pFlowCore->GetAtomLevel( pAtom );

    return iAtomLevel;
}

void CsDriver::RefreshGroupOccs( int iSymGroup, CNDIndexes& theIndex, int iKindUse ) {

    CString csMsg;
    csMsg.Format( _T("RefreshGroupOccs( group: %d, index: %s, iKind: %d )") NL,
        iSymGroup, theIndex.toStringBare().c_str(), iKindUse );
    RTRACE( csMsg );
    RTRACE( _T("{") NL );
    GROUPT*     pGroupT       = GPT(iSymGroup);
    bool        bIsLevelGroup = ( pGroupT->GetGroupType() == GROUPT::Level );
#ifdef _DEBUG
    CFlowAtom*  pAtom         = GetCurrAtom();
#endif

    // be aware - LevelGroups don't setup any occs!
    if( !bIsLevelGroup ) {              // LevelGroups have negative indexes
        bool    bScalar = ( pGroupT->GetNumDim() == 0 );
        int     iThisOcc;
        // the info must be combined as follows:
        //
        // (a) scalar groups:   forward        backward
        //      iKindUse = 1    set 1 occ      set 0 occs
        //      iKindUse = 2    set 1 occ      set 1 occ
        //      iKindUse = 0    (can't be)     (can't be)
        // (b) mult-groups: always set n occs, where n is the index of the
        //                  dimension of this group exactly as given in theIndex

        if( bScalar ) {
            if( iKindUse == 1 )
                iThisOcc = ( IsForwardWay() ) ? 1 : 0;
            else
                iThisOcc = 1;
        }
        else {
            CDimension::VDimType    xDimType = pGroupT->GetDimType();

            iThisOcc = theIndex.getIndexValue(xDimType);
            csMsg.Format( INDENT _T("iThisOcc = theIndex[xDimType{%d}] -> %d") NL,
                (int) xDimType, iThisOcc );
            RTRACE( csMsg );
        }

        int     iDataOccs   = pGroupT->GetDataOccurrences();
        bool    bCanRefresh = true;

        if( IsForwardWay() ) {
            // forward: depends on current refresh-limit
            switch( GetRefreshGroupOccsLimit() ) {
                case 2:                 // 2-don't refresh
                    // ... reserved for PathOn, EndLevel/EndGroup/SkipTo requests
                    bCanRefresh = false;
                    RTRACE( INDENT _T("<fwd> GetRefreshGroupOccsLimit()=2 -> bCanRefresh = false") NL );
                    break;
                case 1:                 // 1-up to DataOccs
                    // ... reserved for PathOff, Modify
                    //Savy added this check to prevent the refresh from incrementing the GroupOccurrence to a higher
                    //value than the dynamic max occs.
                    if (m_pEngineDriver->m_EngineSettings.IsPathOff() && m_pEntryDriver->IsModification() && hasMaxDEField(pGroupT)) {
                        int iDynamicMaxOccs = getGroupMaxOccsUsingMaxDEField(pGroupT);
                        iDataOccs = iDynamicMaxOccs;
                    }
                    bCanRefresh = ( iThisOcc <= iDataOccs || iDataOccs < 1 );
                    csMsg.Format( INDENT _T("<fwd> GetRefreshGroupOccsLimit()=1 -> bCanRefresh = %s") NL,
                        BOOL2STRING(bCanRefresh));
                    RTRACE( csMsg );
                    if( bCanRefresh == false )
                    {
                        csMsg.Format( INDENT _T("<fwd> bCanRefresh = ( iThisOcc [%d] <= iDataOccs [%d] || iDataOccs < 1 )") NL,
                            iThisOcc, iDataOccs );
                        RTRACE( csMsg );
                    }
                    break;
                case 0:                 // 0-no limit
                default:
                    // ... any other situation
                    bCanRefresh = true;
                    RTRACE( INDENT _T("<fwd> GetRefreshGroupOccsLimit(): default -> bCanRefresh = true") NL );
                    break;
            }
        }
        else {
            // backwards: refresh in existing zone only // victor Mar 25, 02
            bCanRefresh = ( iThisOcc <= iDataOccs );
            csMsg.Format( INDENT _T("<back> bCanRefresh = (iThisOcc [%d] <= iDataOccs [%d]) -> [%s]") NL,
                iThisOcc, iDataOccs,
                BOOL2STRING(bCanRefresh));
            RTRACE( csMsg );
        }

//SAVY Jul 30 ,2003 for sequential update in partial add mode
                //SAVY /GSF 03/26/2004 commented this code for the capi app . It is increasing
                //the occurrences to maxocc when you revisit a roster .
//        if( bCanRefresh /*|| m_pEntryDriver->GetPartialMode() == ADD_MODE*/) {
//		GSF 08/24/2005 uncommented this code to restore original behavior, and fix bug2611
		//Savy 10/29/2019 -fix for increasing occs in partial save add mode and when path is on. When reentering a roster in partial save add the occs
		//are getting incorrectly set to max occs due to occurrences getting update below.
        if( bCanRefresh) {
        	csMsg.Format( INDENT INDENT
                _T("%s.SetCurrentOccurrences( %d )") NL,
                pGroupT->GetName().c_str(), iThisOcc );
            RTRACE( csMsg );
            // remake the occurrences of the Group
            pGroupT->SetCurrentOccurrences( iThisOcc );
            pGroupT->CalculateAndSetCurrentOccurrences( iThisOcc );
            pGroupT->AdjustTotalOccs();
            RTRACE( INDENT _T("}") NL );
        }
    }
    RTRACE( _T("}") NL );
}

int CsDriver::GroupTrimOcc( bool bRestore ) {           // victor Dec 10, 01
    // GroupTrimOcc: remove trailing empty occurrences of a given group
    CSettings*  pSettings  = &m_pEngineDriver->m_EngineSettings;
    bool        bPathOff   = pSettings->IsPathOff();
    CFlowAtom*  pAtom      = GetCurrAtom();
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    int         iPivotAtom = GetCurrAtomIndex(); // save the current-atom
    int         iSymGroup  = -1;
    int         iNumOccs   = -1;
    int         iDataOccs=0; // RHF Feb 23, 2004
    // only to be invoked when the current atom is a GroupTail
    ASSERT( xAtomType == CFlowAtom::AtomType::GroupTail );
    if( !( xAtomType == CFlowAtom::AtomType::GroupTail ) )
        return iNumOccs;
    iSymGroup = ((CFlowGroupTail*) pAtom)->GetSymbol();

    // previous atom is HTOcc (either Tail or Last), the trail of an occurrence
    pAtom = GetPrevAtom();
    xAtomType = pAtom->GetAtomType();
    ASSERT( xAtomType == CFlowAtom::AtomType::HTOcc );
    if( !( xAtomType == CFlowAtom::AtomType::HTOcc ) )
        return iNumOccs;

    // scans backwards for a non-empty occurrence
    C3DObject*  p3DObject      = NULL;
    int         iSymVar;
    int         iFieldColor;
    bool        bHasColor;
    int         iVarLen;
    csprochar*       pszAsciiAddr;
    csprochar        pBuf[1024];
    csprochar*       pBlank;
    CFlowHTOcc* pScannedHTOcc  = (CFlowHTOcc*) pAtom;
    int         iScannedHTOcc  = GetCurrAtomIndex();
    bool        bNonEmptyFound = false;
    bool        bStopScanning  = false;

    while( !bStopScanning ) {
        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
                if( ((CFlowGroupHead*) pAtom)->GetSymbol() == iSymGroup )
                    bStopScanning = true;
                break;

            case CFlowAtom::AtomType::GroupTail:
                if( ((CFlowGroupTail*) pAtom)->GetSymbol() != iSymGroup )
                    bNonEmptyFound = ( GroupTrimOcc( false ) > 0 );
                break;

            case CFlowAtom::AtomType::Item:
                p3DObject   = ExtractInfoFromAtom( pAtom );
                iSymVar     = p3DObject->GetSymbol();
                iFieldColor = GetFieldColor( pAtom );
                bHasColor   = ( iFieldColor == FLAG_HIGHLIGHT || bPathOff && iFieldColor == FLAG_MIDLIGHT );

                // when the field has a color...
                // RHF COM Feb 23, 2004 if( bHasColor )
                if( true ) { // RHF Feb 23, 2004
                    // ... looks for the ascii' contents
                    pszAsciiAddr = FurnishFieldAsciiAddr( p3DObject );
                    ASSERT( pszAsciiAddr != NULL );

                    // ... prepare a blank area to compare the contents
                    VART* pVarT = VPT(iSymVar);
                    iVarLen = pVarT->GetLength();
                    pBlank  = ( iVarLen <= 1024 ) ? pBuf : (csprochar*) calloc( iVarLen, sizeof(csprochar) );
                    _tmemset( pBlank, _T(' '), iVarLen );

                    // RHF INIC Feb 23, 2004
                    bool bIsEmpty = ( _tmemcmp( pszAsciiAddr, pBlank, iVarLen ) == 0 );

                    if( bHasColor )
                    {
                        // in the rare case where a protected alpha field is blank (but has been visited),
                        // this blank field should be considered as an non-empty field
                        if( bIsEmpty && pVarT->IsAlpha() && pVarT->IsProtected() )
                            bIsEmpty = false;

                        bNonEmptyFound = !bIsEmpty;
                    }

                    if( iDataOccs == 0 && !bIsEmpty ) {
                        int     iSymDummy;
                        PassFrom3D( p3DObject, &iSymDummy, &iDataOccs );
                    }
                    // RHF END Feb 23, 2004

                    if( pBlank != pBuf )
                        free( pBlank );
                }
                break;

            case CFlowAtom::AtomType::HTOcc:
                pScannedHTOcc = (CFlowHTOcc*) pAtom;
                iScannedHTOcc = GetCurrAtomIndex();
                break;

            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
                // blocks can be ignored
                break;

            case CFlowAtom::AtomType::BeforeStrip:
            case CFlowAtom::AtomType::BeyondStrip:
            default:
                ASSERT( 0 );        // can't be
                break;
        }

        bStopScanning |= bNonEmptyFound;

        if( !bStopScanning ) {
            // get previous atom
            pAtom = GetPrevAtom();
            xAtomType = pAtom->GetAtomType();
        }
    }

    // set the number of occurrences found
    GROUPT*             pGroupT  = GPT(iSymGroup);
    CFlowAtom::OccType  xOccType = pScannedHTOcc->GetOccType();

    iNumOccs = 0;
    if( bNonEmptyFound ) {
        if( !pGroupT->GetNumDim() ) {
            ASSERT( xOccType == CFlowAtom::OccType::Head || xOccType == CFlowAtom::OccType::Tail );
            ASSERT( xOccType == CFlowAtom::OccType::Tail ); // attn - if not, bNonEmptyFound can't arise
            if( xOccType != CFlowAtom::OccType::Head )
                iNumOccs = 1;
        }
        else {
            ASSERT(
                    xOccType == CFlowAtom::OccType::First ||
                    xOccType == CFlowAtom::OccType::Inner ||
                    xOccType == CFlowAtom::OccType::Last
                  );
            if( xOccType != CFlowAtom::OccType::First ) {
                CDimension::VDimType    xDimType = pGroupT->GetDimType();

                iNumOccs = pScannedHTOcc->GetIndexBwd().getIndexValue(xDimType);
            }
        }
    }

    pGroupT->SetCurrentOccurrences( iNumOccs );
    pGroupT->CalculateAndSetCurrentOccurrences( iNumOccs );

    pGroupT->SetTotalOccurrences( iNumOccs );

    // RHF INIC May 07, 2004
    pGroupT->SetDataOccurrences( iNumOccs );
    iDataOccs = std::max( iNumOccs, iDataOccs );
    pGroupT->SetPartialOccurrences( iDataOccs );
    // RHF END May 07, 2004

    // set any field located beyond the non-empty occurrence to no-light
    int     iAtom = iScannedHTOcc;

    while( ++iAtom <= iPivotAtom ) {                    // RHF Feb 23, 02
        pAtom = GetAtomAt( iAtom );
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType ) {
            case CFlowAtom::AtomType::Item:
                SetFieldColorNone( pAtom );
                break;

            default:
                break;
        }
    }

    if( bRestore )
        SetCurrAtomIndex( iPivotAtom ); // restore the current-atom

    return iNumOccs;
}

void CsDriver::ResetRefreshGroupOccsLimit( void ) {
    // ResetRefreshGroupOccsLimit: set the refresh-group-occs-limit to "1-up to DataOccs" if PathOff & Modify, to "0-no limit" otherwise
	//Savy 10/29/2019 -fix for increasing occs in partial save add mode when reentering a roster in path off mode does not refresh occs
	//as the check did not take into account partial mode add causing an issue when using "skip to next" not updating the occs and failing
	//in partial save add mode. Now setting the occ limit upto data occs only in truly modify mode.
	bool				bIsModification = m_pEntryDriver->IsModification() && m_pEntryDriver->GetPartialMode() != ADD_MODE;
    CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff        = pSettings->IsPathOff();

    SetRefreshGroupOccsLimit( bPathOff && bIsModification );
}

// RHF INIC Feb 13, 2003
void CsDriver::CopyPendingAdvance(CsDriver* pCsDriver ) {
    int     iNumPendingAdvances = pCsDriver->GetNumOfPendingAdvances();

    if( iNumPendingAdvances > 0 ) {
          AddPendingAdvance( NULL );
    }
    /*
    for( int i = 0; i < iNumPendingAdvances; i++ ) {
        C3DObject*    p3DObject= pCsDriver->GetPendingAdvance( i );

        AddPendingAdvance( p3DObject );
    }
    */
}

void CsDriver::AddPendingAdvance(C3DObject* p3DObject ) {
    C3DObject*  p3DNewObject = new C3DObject;

    if( p3DObject != NULL )
        *p3DNewObject = *p3DObject;
    else
        p3DNewObject->SetEmpty();

    m_aPendingAdvances.emplace_back( p3DNewObject );
}
// RHF END Feb 13, 2003

void CsDriver::SavePendingAdvance( void ) {             // victor Jun 21, 01
    // SavePendingAdvance: store the current "target for advance-to" to the list of pending advance' requests
    // ... remark - empty targets are "to infinite"
    C3DObject*  p3DObject = new C3DObject;

    *p3DObject = m_TgtAdvanceTo;
    m_aPendingAdvances.emplace_back( p3DObject );

    // insures the best pending advance becomes active
    GetBestPendingAdvance();
}

bool CsDriver::GetBestPendingAdvance() // victor Jun 21, 01
{
    // GetBestPendingAdvance: from the list of pending advance' requests, choose the "best" (the farther one, indeed), copy it to the "target for advance-to" (or, if reached, kill both the target and the list)
    int iNumPending = GetNumOfPendingAdvances();
    bool bDone = ( iNumPending > 0 );

    if( bDone )
    {
        C3DObject* p3DBest = NULL;
        int iSymBest = 0;
        int iLevBest = 0;

        for( int iSlot = 0; iSlot < iNumPending; iSlot++ )
        {
            C3DObject* p3DObject = m_aPendingAdvances[iSlot];

            // "to infinite" is always the best one
            if( p3DObject->IsEmpty() )
            {
                p3DBest = p3DObject;
                break;                  // don't scan anymore
            }

            // if this is best, choose it
            bool bThisIsBest = false;
            int iSymbol = p3DObject->GetSymbol();

            ASSERT(NPT(iSymbol)->IsOneOf(SymbolType::Group, SymbolType::Block, SymbolType::Variable));

            int iLevel = NPT(iSymbol)->IsA(SymbolType::Group) ? GPT(iSymbol)->GetLevel() :
                         NPT(iSymbol)->IsA(SymbolType::Block) ? GetSymbolEngineBlock(iSymbol).GetGroupT()->GetLevel() :
                                                                VPT(iSymbol)->GetLevel();

            // when no one has been chosen yet, set the first one as "best"
            if( p3DBest == NULL )
                bThisIsBest = true;

            else if( *p3DObject != *p3DBest )
            {
                // "higher" levels have priority
                if( iLevel > iLevBest )
                    bThisIsBest = true; // "this" is at a "higher" level

                // at same level, the farther is best
                else if( iLevel == iLevBest && FurnishPrecedence(p3DBest, p3DObject) > 0 )
                    bThisIsBest = true; // "this" is farther than "best"
            }

            if( bThisIsBest )
            {
                p3DBest  = p3DObject;
                iSymBest = iSymbol;
                iLevBest = iLevel;
            }
        }

        ASSERT(p3DBest != NULL); // RCL, Jun 16, 2004

        // when "best" is before current-atom, discard "best" // RHF Jun 19, 2001
        if( !p3DBest->IsEmpty() )
        {
            if( SearchTargetLocation( p3DBest ) <= 0 )
            {
                bDone = false;          // "best" is before "current" (or it is the same as)

                // ... and kill the list of pending advances
                KillPendingAdvances();
            }
        }

        if( bDone )
        {
            // set the "target for advance-to" to the "best" found
            if( p3DBest != NULL )
                m_TgtAdvanceTo = *p3DBest;
            else
                m_TgtAdvanceTo.SetEmpty();

            SetRequestNature( AdvanceTo );
            SetRequestSource( true );   // from Logic (no matter the original source)
        }
    }

    return bDone;
}

void CsDriver::KillPendingAdvances( void ) {            // victor Jun 21, 01
    // KillPendingAdvances: erase all pending advances
    int     iNumPending = GetNumOfPendingAdvances();

    for( int iSlot = iNumPending - 1; iSlot >= 0; iSlot-- ) {
        C3DObject*  p3DObject = m_aPendingAdvances[iSlot];
        delete p3DObject;
    }
    m_aPendingAdvances.clear();
}



/////////////////////////////////////////////////////////////////////////////
//
// --- requests (and targets) from Interface and Logic
//
/////////////////////////////////////////////////////////////////////////////

    // (1) validating requests by source

void CsDriver::BuildRequestValid( void ) {
    RequestValid*   pRequest;

    //                                           Request       Interface Logic
    //                                           -------       --------- -----
    pRequest =  new RequestValid( None,           false,  false ); m_aRequestValid.emplace_back( pRequest );

    // (a) from Interface and Logic
    pRequest =  new RequestValid( AdvanceTo,      true,   true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( SkipTo,         true,   true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( Reenter,        true,   true  ); m_aRequestValid.emplace_back( pRequest );

    // (b) from Logic only
    pRequest =  new RequestValid( AdvanceToNext,  false,  true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( SkipToNext,     false,  true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( LogicEndLevel,  false,  true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( LogicEndGroup,  false,  true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( EnterFlowEnd,   false,  true  ); m_aRequestValid.emplace_back( pRequest );

    // (c) from Interface only
    pRequest =  new RequestValid( NextField,      true,   true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( PrevField,      true,   true  ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( InterEndOccur,  true,   false ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( InterEndGroup,  true,   false ); m_aRequestValid.emplace_back( pRequest );
    pRequest =  new RequestValid( InterEndLevel,  true,   false ); m_aRequestValid.emplace_back( pRequest );

    // (d) other requests from Interface
    //  --- none ---                                    // victor May 10, 01
}

bool CsDriver::IsRequestValid( void ) {
    bool            bIsValid = false;
    bool            bFound = false;
    RequestNature   xRequestNature = GetRequestNature();
    RequestValid*   pRequest;

    for( int iRequest = 0; !bFound && iRequest < (int)m_aRequestValid.size(); iRequest++ ) {
        pRequest = m_aRequestValid[iRequest];

        if( xRequestNature == pRequest->GetNature() ) {
            bFound = true;
            bIsValid = ( IsRequestFromLogic() ) ? pRequest->GetLogicValid() : pRequest->GetInterValid();
        }
    }

    return bIsValid;
}

    // (2) identification of the request

bool CsDriver::SetInterRequestNature( RequestNature xNature, bool bResetAdvance/*=true*/ ) {
    SetRequestEventType( (EventType) m_pIntDriver->m_iProgType ); // RHF Mar 23, 2003
    // save previous situation
    bool            bPrevRequestSource = m_bRequestFromLogic;
    RequestNature   xPrevRequestNature = m_xRequestNature;

    // set requested situation and evaluate the request
    SetRequestSource( false );          // from Interface
    SetRequestNature( xNature );

    bool    bValidNature = IsRequestValid();

    if( !bValidNature ) {               // restore previous situation
        m_bRequestFromLogic = bPrevRequestSource;
        m_xRequestNature    = xPrevRequestNature;
    }

    // RHF INIC Oct 17, 2002
    //Change behavior of persistent advance. Now persistent advance is reset when a BackRequest is issued
    if( bResetAdvance ) {
        KillPendingAdvances();
        ResetSourceOfNodeAdvance();
        SetTargetNode( CEntryDriver::NodeAdvanceNone );
    }
    // RHF END Oct 17, 2002


    return bValidNature;
}

bool CsDriver::SetLogicRequestNature( RequestNature xNature, bool bResetAdvance/*=true*/ ) {
    // save previous situation
    bool            bPrevRequestSource = m_bRequestFromLogic;
    RequestNature   xPrevRequestNature = m_xRequestNature;

    // set requested situation and evaluate the request
    SetRequestSource( true );           // from Logic
    SetRequestNature( xNature );

    bool    bValidNature = IsRequestValid();

    if( !bValidNature ) {               // restore previous situation
        m_bRequestFromLogic = bPrevRequestSource;
        m_xRequestNature    = xPrevRequestNature;
    }

    // RHF INIC Oct 17, 2002
    //Change behavior of persistent advance. Now persistent advance is reset when a BackRequest is issued
    if( bResetAdvance ) {
        KillPendingAdvances();
        ResetSourceOfNodeAdvance();
        SetTargetNode( CEntryDriver::NodeAdvanceNone );
    }
    // RHF END Oct 17, 2002

    return bValidNature;
}


void CsDriver::ResetRequest()
{
    RequestNature xNature = GetRequestNature();
    ResetRequests(xNature, xNature);
}

void CsDriver::ResetRequests(RequestNature xFirstRequestNature/* = RequestNature::AdvanceTo*/, RequestNature xLastRequestNature/* = RequestNature::None*/)
{
    // reset all the identification of the request
    m_xRequestNature    = None;
    m_bRequestBreaking  = false;
    m_pRequestOrigin    = NULL;
    m_pRequestEventType = EventNone;

    for( RequestNature xNature = xFirstRequestNature; xNature <= xLastRequestNature; xNature = (RequestNature)( (int)xNature + 1 ) )
    {
        // reset the specific parameters of the solved request
        switch( xNature )
        {
            case AdvanceTo:
                m_TgtAdvanceTo.SetEmpty();
                break;

            case SkipTo:
                m_TgtSkipTo.SetEmpty();
                break;

            case Reenter:
                m_TgtReenter.SetEmpty();
                break;

            case SkipToNext:
                m_iTgtSkipToNext = 0;
                break;

            case InterEndOccur:
                // TODO reset m_EndOccurParms
                break;

            case InterEndGroup:
                // TODO reset m_EndGroupParms
                break;

            case InterEndLevel:
                AtEndLevel_SetParameters( false, -1, false );
                break;
        }
    }

    SetNodeBoundTarget(); // victor Mar 07, 02

    // kill the list of pending advances (excepting when starting a new flow)
    if( !EnterFlowJustStarted() )
        KillPendingAdvances();
}


void CsDriver::SetRequestNature( RequestNature xNature ) {
    // set the new request as required
    m_xRequestNature = xNature;
    // check if this a "breaking" request
    switch( xNature ) {
        // TODO check this list of requests breaking the chain of event-procs is complete
        case SkipTo:
        case Reenter:
        case SkipToNext:
        case LogicEndGroup:
        case LogicEndLevel:
        case EnterFlowEnd:               // victor Aug 02, 01
        case PrevField:                  // victor Jun 12, 01
        case InterEndOccur:
        case InterEndGroup:
        case InterEndLevel:
            m_bRequestBreaking = true;
            break;

        case AdvanceTo:
        case AdvanceToNext:
        case NextField:
            m_bRequestBreaking = false;
            break;

        default:
            ASSERT(xNature == None);
            m_bRequestBreaking = false;
            break;
    }
}

void CsDriver::SetRequestOrigin( void ) {
    CFlowAtom*  pAtom = NULL;
    int         iAtom = 0;

    if( m_pFlowCore != NULL ) {
        pAtom = GetCurrAtom();
        iAtom = GetCurrAtomIndex();
    }

    m_pRequestOrigin = pAtom;
    m_iRequestOriginIndex = iAtom;
}

    // (3) requested from Interface and Logic

bool CsDriver::Set3DTarget( C3DObject* p3DObject ) {
    bool    bDone = true;
    int     iSymTarget = 0;
    switch( GetRequestNature() ) {
        case AdvanceTo:
            if( p3DObject != NULL ) {   // ... to a target
                m_TgtAdvanceTo = *p3DObject;
                iSymTarget     = m_TgtAdvanceTo.GetSymbol();
            }
            else                        // ... "unlimited" or "to the infinite"
                m_TgtAdvanceTo.SetEmpty();

            // add this "target for advance-to" to the list of pending advance' requests
            SavePendingAdvance();
            break;

        case SkipTo:
            if( p3DObject != 0 )
                m_TgtSkipTo = *p3DObject;
            else
                m_TgtAdvanceTo.SetEmpty();
            iSymTarget  = m_TgtSkipTo.GetSymbol();
            break;

        case Reenter:
            if( p3DObject != NULL ) {   // ... to a target
                m_TgtReenter = *p3DObject;
                iSymTarget   = m_TgtReenter.GetSymbol();
            }
            else                        // ... "to itself"
                m_TgtReenter.SetEmpty();
            break;

        default:
            bDone = false;
            break;
    }
    return bDone;
}

    // (4) requested from Logic only

bool CsDriver::SetSymTarget( int iSymbol ) {
    bool    bDone = true;
    switch( GetRequestNature() ) {
        case AdvanceToNext:
            ASSERT( NPT(iSymbol)->IsA(SymbolType::Variable) );
            // TODO soft-check that iSymbol is a Var
            m_iTgtAdvanceToNext = iSymbol;
            break;

        case SkipToNext:
            ASSERT( NPT(iSymbol)->IsA(SymbolType::Variable) );
            // TODO soft-check that iSymbol is a Var
            m_iTgtSkipToNext = iSymbol;
            break;

        case EnterFlowEnd:
            ASSERT( !iSymbol );
            m_iTgtSymFlow = 0;
            break;

        default:
            bDone = false;
            break;
    }
    return bDone;
}

    // (5) requested from Interface only

    // (6) other requests from Interface




/////////////////////////////////////////////////////////////////////////////
//
// --- information interchange
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::FurnishCurObject( C3DObject* p3DObject ) {
    // FurnishCurObject: places a copy of the current object into a given 3D-object
    bool    bDone = ( p3DObject != NULL );

    if( bDone )
        *p3DObject = *GetCurObject();

    return bDone;
}

bool CsDriver::FurnishGroupHead( C3DObject* p3DObject ) { // victor Mar 25, 02
    // FurnishGroupHead: return a 3D-object with the GroupHead of the current group
    bool        bLocated    = false;
    CFlowCore*  pFlowCore   = GetFlowCore();
    CFlowAtom*  pAtom       = GetCurrAtom();
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    if( pFlowCore == NULL || xAtomType != CFlowAtom::AtomType::Item )
        return bLocated;

    int         iSymSource  = ((CFlowItem*) pAtom)->GetSymbol();
    int         iSymGroup   = VPT(iSymSource)->GetOwnerGPT()->GetSymbolIndex();
    ASSERT( iSymGroup > 0 );

    if( iSymGroup <= 0 )
        return bLocated;

    int         iCurrAtom   = GetCurrAtomIndex(); // save the current-atom
    int         iSymbol     = 0;
    bool        bStopSearch = false;

    while( !bLocated && pAtom != NULL ) {
        switch( pAtom->GetAtomType() ) {
            case CFlowAtom::AtomType::GroupHead:
                iSymbol = ((CFlowGroupHead*) pAtom)->GetSymbol();

                if( iSymbol == iSymGroup ) {
                    CNDIndexes theIndex = ((CFlowGroupHead*) pAtom)->GetIndexFwd();

                    p3DObject->SetSymbol( iSymbol );
                    p3DObject->setIndexes( theIndex );

                    bLocated    = true;
                    bStopSearch = true;
                }
                break;

            case CFlowAtom::AtomType::GroupTail:
                iSymbol = ((CFlowGroupTail*) pAtom)->GetSymbol();

                if( iSymbol == iSymGroup ) {
                    ASSERT( 0 );        // shouldn't be
                    bStopSearch = true;
                }
                break;

            case CFlowAtom::AtomType::HTOcc:
            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
            case CFlowAtom::AtomType::Item:
                break;

            case CFlowAtom::AtomType::BeforeStrip:
            case CFlowAtom::AtomType::BeyondStrip:
            default:
                ASSERT( 0 );                // shouldn't be
                bStopSearch = true;
                break;
        }
        if( !bStopSearch )
            pAtom = m_pFlowCore->FlowStripPrev();
    }
    ASSERT( bLocated ); // if not, possible flow-strip corruption

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom
    return bLocated;
}

bool CsDriver::FurnishFirstField( C3DObject* p3DObject, bool bInTheOccur ) { // victor Mar 25, 02
    // FurnishFirstField: return a 3D-object with the first field either in the current occurrence or in the current group
    bool        bInTheGroup = ( !bInTheOccur );
    bool        bLocated    = false;
    CFlowCore*  pFlowCore   = GetFlowCore();
    CFlowAtom*  pAtom       = GetCurrAtom();
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    if( pFlowCore == NULL || xAtomType != CFlowAtom::AtomType::Item )
        return bLocated;

    int         iSymSource  = ((CFlowItem*) pAtom)->GetSymbol();
    int         iSymGroup   = VPT(iSymSource)->GetOwnerGPT()->GetSymbolIndex();
    ASSERT( iSymGroup > 0 );

    if( iSymGroup <= 0 )
        return bLocated;

    int         iCurrAtom   = GetCurrAtomIndex(); // save the current-atom
    int         iSymbol     = 0;
    bool        bStopSearch = false;
    CNDIndexes  theIndex( ONE_BASED, DEFAULT_INDEXES );

    while( !bStopSearch && pAtom != NULL ) {
        switch( pAtom->GetAtomType() ) {
            case CFlowAtom::AtomType::GroupHead:
                iSymbol = ((CFlowGroupHead*) pAtom)->GetSymbol();

                if( iSymbol == iSymGroup )
                    bStopSearch = true;
                break;

            case CFlowAtom::AtomType::GroupTail:
                iSymbol = ((CFlowGroupTail*) pAtom)->GetSymbol();

                if( iSymbol == iSymGroup ) {
                    ASSERT( 0 );        // shouldn't be
                    bStopSearch = true;
                }
                break;

            case CFlowAtom::AtomType::HTOcc:
                if( bInTheOccur ) {
                    bLocated    = true;
                    bStopSearch = true;
                }
                break;

            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
                break;

            case CFlowAtom::AtomType::Item:
                iSymbol = ((CFlowItem*) pAtom)->GetSymbol();
                theIndex = ((CFlowGroupHead*) pAtom)->GetIndexFwd();

                p3DObject->SetSymbol( iSymbol );
                p3DObject->setIndexes( theIndex );
                break;

            case CFlowAtom::AtomType::BeforeStrip:
            case CFlowAtom::AtomType::BeyondStrip:
            default:
                ASSERT( 0 );                // shouldn't be
                bStopSearch = true;
                break;
        }
        if( !bStopSearch )
            pAtom = m_pFlowCore->FlowStripPrev();
    }
    ASSERT( bLocated ); // if not, possible flow-strip corruption

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom
    return bLocated;
}

csprochar* CsDriver::FurnishFieldAsciiAddr( C3DObject* p3DObject ) {
    ASSERT( NPT(p3DObject->GetSymbol())->IsA(SymbolType::Variable) );

    // FurnishFieldValue: places a copy of the text-value of a given field (described by a 3D-object) into a given text-area
    CNDIndexes theIndex;
    VARX*       pVarX = GetFieldFrom3D( p3DObject, theIndex );
    csprochar*       pszAsciiAddr = NULL;// assuming there is no text available

    if( pVarX != NULL )
        pszAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarX, theIndex );

    return pszAsciiAddr;
}

bool CsDriver::FurnishFieldValue( C3DObject* p3DObject, csprochar* pszValueText ) {
    // FurnishFieldValue: places a copy of the text-value of a given field (described by a 3D-object) into a given text-area
    int     iSymVar = p3DObject->GetSymbol();
    ASSERT( NPT(iSymVar)->IsA(SymbolType::Variable) );
    bool    bDone = ( pszValueText != NULL && NPT(iSymVar)->IsA(SymbolType::Variable) );

    if( bDone ) {
        csprochar*       pszAsciiAddr = FurnishFieldAsciiAddr(p3DObject);
        VART*       pVarT   = VPT(iSymVar);
        int         iVarLen = pVarT->GetLength();

        if( pszAsciiAddr != NULL )
            _tmemcpy( pszValueText, pszAsciiAddr, iVarLen );
        else {
            bDone = false;
            _tmemset( pszValueText, _T(' '), iVarLen );
        }

        pszValueText[iVarLen] = 0;
    }

    return bDone;
}

bool CsDriver::FurnishFieldLight( C3DObject* p3DObject, int* pFieldLight ) {
    ASSERT( NPT(p3DObject->GetSymbol())->IsA(SymbolType::Variable) );

    // FurnishFieldLight: places the light of a given field (described by a 3D-object) into a given light-descriptor
    bool    bDone = ( pFieldLight != NULL );

    if( bDone ) {
        CNDIndexes theIndex;
        VARX*      pVarX = GetFieldFrom3D( p3DObject, theIndex );

        if( pVarX != NULL )
            *pFieldLight = m_pIntDriver->GetFieldColor( pVarX, theIndex );
        else {
            bDone = false;
            *pFieldLight = -1;
        }
    }

    return bDone;
}

int CsDriver::FurnishPrecedence( C3DObject* p3DTarget, C3DObject* p3DSource ) {
    // remark - adapted from CEngineArea::FieldPrecedence (see GroupT.cpp)
    // this evaluation assumes both fields belong to the same "major node"
    // (major node precedence should be calculated before enter here)
    // < 0: source before target
    //   0: source equals target
    // > 0: source after target

    // empty target: by axiom, "source before target"   // victor Jun 21, 01
    if( p3DTarget->IsEmpty() )                          // victor Jun 21, 01
        return -1;                                      // victor Jun 21, 01

    int     iPrecedence    = 0;         // assuming source equals target

    // if no 3D-source provided, use current-object     // RHF+vc Jun 15, 01
    if( p3DSource == NULL )                             // RHF+vc Jun 15, 01
        p3DSource = GetCurObject();                     // RHF+vc Jun 15, 01

    // analysis of intervening types                    // victor Dec 10, 01
    Symbol* pSourceSymbol = NPT(p3DSource->GetSymbol());
    Symbol* pTargetSymbol = NPT(p3DTarget->GetSymbol());
    ASSERT(pSourceSymbol->IsOneOf(SymbolType::Group, SymbolType::Block, SymbolType::Variable));
    ASSERT(pTargetSymbol->IsOneOf(SymbolType::Group, SymbolType::Block, SymbolType::Variable));

    // prior to blocks, this code was here, perhaps these scenarios should be handled at some point...
    /*
    if( eTargetSymType == SymbolType::Group && eSourceSymType == SymbolType::Group ) {
        // both are groups
        // TODO iPrecedence = GetGroupPrecedence???
    }
    else if( eTargetSymType == SymbolType::Variable && eSourceSymType == SymbolType::Group ) {
        // target is var, source is group
// TODO iPrecedence = FieldAgainstCurrentAtom( p3DTarget );
    }
    else if( eTargetSymType == SymbolType::Group && eSourceSymType == SymbolType::Variable ) {
// TODO iPrecedence = GroupAgainstCurrentAtom( p3DTarget );
    }
    */
    // below, both target and source are vars or blocks
    if( pSourceSymbol->IsOneOf(SymbolType::Block, SymbolType::Variable) && pTargetSymbol->IsOneOf(SymbolType::Block, SymbolType::Variable) )
    {
        if( *p3DTarget != *p3DSource )
        {
            // setup info of source var
            GROUPT* pSourceGroupT = pSourceSymbol->IsA(SymbolType::Block) ? assert_cast<const EngineBlock*>(pSourceSymbol)->GetGroupT() :
                                                                            ((VART*)pSourceSymbol)->GetOwnerGPT();
            int iSourceFlowOrder;
            int iSourceItem = pSourceGroupT->GetItemIndex(p3DSource->GetSymbol(), &iSourceFlowOrder);

            // setup info of target var
            GROUPT* pTargetGroupT = pTargetSymbol->IsA(SymbolType::Block) ? assert_cast<const EngineBlock*>(pTargetSymbol)->GetGroupT() :
                                                                            ((VART*)pTargetSymbol)->GetOwnerGPT();
            int iTargetFlowOrder = 0;
            int iTargetItem = pTargetGroupT->GetItemIndex(p3DTarget->GetSymbol(), &iTargetFlowOrder);

            // in different groups... depends on flow order
            if( pSourceGroupT != pTargetGroupT )
            {
                if( iSourceFlowOrder < iTargetFlowOrder )
                    iPrecedence = -1;           // source before target

                else if( iSourceFlowOrder > iTargetFlowOrder )
                    iPrecedence =  1;           // source after target
            }

            // in same group... depends on occurrences precedence
            else
            {
                // TEMPORARY - the "only" occur-index is used below
                int iSymDummy;
                int iSourceOcc;
                int iTargetOcc;

                PassFrom3D( p3DSource, &iSymDummy, &iSourceOcc );
                PassFrom3D( p3DTarget, &iSymDummy, &iTargetOcc );

                if( iSourceOcc != iTargetOcc )
                {
                    if( iSourceOcc < iTargetOcc )
                        iPrecedence = -1;           // source before target

                    else if( iSourceOcc > iTargetOcc )
                        iPrecedence =  1;           // source after target
                }

                // in same group & occurrence... depends on flow order
                else
                {
                    if( iSourceFlowOrder < iTargetFlowOrder )
                        iPrecedence = -1;           // source before target

                    else if( iSourceFlowOrder > iTargetFlowOrder )
                        iPrecedence =  1;           // source after target
                }
            }
        }
    }

    return iPrecedence;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- driver front-end
//
/////////////////////////////////////////////////////////////////////////////


std::shared_ptr<Paradata::FieldMovementTypeInfo> CsDriver::CreateFieldMovementType(RequestNature request_nature)
{
    Paradata::FieldMovementTypeInfo::RequestType request_type = Paradata::FieldMovementTypeInfo::RequestType::Advance;

    switch( request_nature )
    {
        case RequestNature::AdvanceTo:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::Advance;
            break;
        case RequestNature::SkipTo:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::Skip;
            break;
        case RequestNature::Reenter:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::Reenter;
            break;
        case RequestNature::AdvanceToNext:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::AdvanceToNext;
            break;
        case RequestNature::SkipToNext:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::SkipToNext;
            break;
        case RequestNature::InterEndOccur:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::EndOccurrence;
            break;
        case RequestNature::LogicEndGroup:
        case RequestNature::InterEndGroup:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::EndGroup;
            break;
        case RequestNature::LogicEndLevel:
        case RequestNature::InterEndLevel:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::EndLevel;
            break;
        case RequestNature::EnterFlowEnd:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::EndFlow;
            break;
        case RequestNature::NextField:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::NextField;
            break;
        case RequestNature::PrevField:
            request_type = Paradata::FieldMovementTypeInfo::RequestType::PreviousField;
            break;
        default:
            ASSERT(false);
    }

    return std::make_shared<Paradata::FieldMovementTypeInfo>(Paradata::FieldMovementTypeInfo { request_type, IsForwardWay() });
}

C3DObject* CsDriver::DriverBrain()
{
    // DriverBrain ... the only public method for solving requests
    // remark - in the loop below, every functions called return
    //          ... false - no other request was internally issued by the logic,
    //                      the control is returned to the caller
    //          ... true  - another request was issued and the cycle must continue on to solve it
    // remark - once the cycle is interrupted, a new "current 3D-object" must be
    //          calculated and returned to the interface

    m_pEntryDriver->SetCsDriver( this );                // victor Mar 07, 02
    m_pEngineDriver->SetFlowInProcess( GetFlow() );     // victor Jul 25, 01

    CString csMsg;
    int iLoopCount = 0;
    RTRACE( _T("DriverBrain()") NL );
    RTRACE( _T("{") NL );

    // for paradata
    RequestNature xFinalRequestNature = GetRequestNature();
    std::shared_ptr<Paradata::FieldMovementTypeInfo> initial_field_movement_type;

    if( Paradata::Logger::IsOpen() )
    {
        // store the previous movement instance for the validation event
        m_currentFieldMovementInstanceForValidationEvent = m_currentFieldMovementInstance;

        // log the entry event
        if( m_currentFieldEntryEvent != nullptr )
        {
            m_currentFieldEntryEvent->SetPostEntryValues();
            Paradata::Logger::LogEvent(m_currentFieldEntryEvent);
            m_currentFieldEntryEvent.reset();
        }

        // start setting up for paradata events for this method
        initial_field_movement_type = CreateFieldMovementType(xFinalRequestNature);
    }


    bool bNewRequestReceived = true;
    bool bValidRequest = true;
    bool bValidParms = true;

    bool bLoopBrokeByStop = false;

    while( bNewRequestReceived && bValidRequest && bValidParms )
    {
        iLoopCount++;
        csMsg.Format( INDENT _T("%d.- Looping, (bNewRequestReceived && bValidRequest && bValidParms)") NL, iLoopCount );
        RTRACE( csMsg );

        // set the origin only if not an "internal" request
        if( RemakeOrigin() )                            // victor Jun 21, 01
            SetRequestOrigin();

        // check the request is valid and has the expected parameters
        bValidRequest = IsRequestValid();
        bValidParms   = RequestHasValidParms();

        csMsg.Format( INDENT _T("bValidRequest = %s, bValidParams = %s") NL, BOOL2STRING(bValidRequest), BOOL2STRING(bValidParms) );
        RTRACE( csMsg );

        if( !bValidRequest || !bValidParms )
        {
            ASSERT( 0 );                // what happens?
            return GetCurObject();
            continue;                   // ... to return to the invoking point
        }

        // launch the operation to satisfy the request
        RequestNature xRequestNature = GetRequestNature();

        xFinalRequestNature = xRequestNature;

        switch( xRequestNature ) {
            // (a) requested from Interface and Logic
            case AdvanceTo:
            case SkipTo:
                bNewRequestReceived = SolveSkipOrAdvanceTo();
                break;

            case Reenter:
                bNewRequestReceived = SolveReenterTo();
                break;

            // (b) requested from Logic only
            case AdvanceToNext:
                bNewRequestReceived = SolveAdvanceToNext();   // victor May 10, 01
                break;

            case SkipToNext:
                bNewRequestReceived = SolveSkipToNext();      // victor May 10, 01
                break;

            case LogicEndGroup:
                bNewRequestReceived = SolveLogicEndGroup();   // victor May 10, 01
                break;

            case LogicEndLevel:
                bNewRequestReceived = SolveLogicEndLevel();   // victor May 10, 01
                break;

            case EnterFlowEnd:
                bNewRequestReceived = SolveEnterFlowEnd();    // victor Jul 25, 01
                break;

            // (c) requested from Interface only
            case NextField:
                bNewRequestReceived = SolveNextField( true );
                break;

            case PrevField:
                bNewRequestReceived = SolvePrevField();
                break;

            case InterEndOccur:
                bNewRequestReceived = SolveInterEndOccur();   // victor May 10, 01
                break;

            case InterEndGroup:
                bNewRequestReceived = SolveInterEndGroup();   // victor May 10, 01
                break;

            case InterEndLevel:
                bNewRequestReceived = SolveInterEndLevel();   // victor May 10, 01
                break;

            // (d) other requests from Interface
            //  --- none ---                                  // victor May 10, 01

            // (*) no implementation available for the request
            default:
                ASSERT( 0 );            // unable to handle this request
                bNewRequestReceived = false; // ... to return to the invoking point
                break;
        }

        if( m_pIntDriver->m_bStopProc )
        {
            SetDoorCondition(CsDriver::Closed);
            m_pEntryDriver->m_bMustEndEntrySession = true;
            SetCurObject( NULL );
            bLoopBrokeByStop = true;
            break;
        }

        // look for pending advances and prot-fields    // RHF+vc Jun 21, 01
        bNewRequestReceived = CheckImplicitActions( bNewRequestReceived );
        csMsg.Format( INDENT _T("bNewRequestReceived = CheckImplicitActions( bNewRequestReceived ) [%s]") NL,
                      BOOL2STRING(bNewRequestReceived) );
        RTRACE( csMsg );
    }

    RTRACE( _T("}") NL );

    if( !bLoopBrokeByStop )
    {
        ResetRequest();                     // forgets any request prior to return

        GetCurDeFld();                      // TRANSITION-TO-3D   // synchronizing m_CurObject & m_CurDeFld
    }


    if( initial_field_movement_type != nullptr )
    {
        std::shared_ptr<Paradata::FieldEntryInstance> from_field_entry_instance;

        if( m_currentFieldMovementInstance != nullptr )
            from_field_entry_instance = m_currentFieldMovementInstance->GetToFieldEntryInstance();

        std::shared_ptr<Paradata::FieldMovementTypeInfo> field_final_movement_type;
        std::shared_ptr<Paradata::FieldEntryInstance> to_field_entry_instance;
        DEFLD3* pDeFld = nullptr;

        if( !bLoopBrokeByStop )
        {
            field_final_movement_type = CreateFieldMovementType(xFinalRequestNature);

            pDeFld = GetCurDeFld();

            if( pDeFld != nullptr )
                to_field_entry_instance = std::make_shared<Paradata::FieldEntryInstance>(m_pIntDriver->m_pParadataDriver->CreateFieldInfo(pDeFld));
        }

        std::shared_ptr<Paradata::FieldMovementInstance> field_movement_instance = std::make_shared<Paradata::FieldMovementInstance>(
            from_field_entry_instance, initial_field_movement_type, field_final_movement_type, to_field_entry_instance);

        m_currentFieldMovementInstance = field_movement_instance;

        Paradata::Logger::LogEvent(std::make_shared<Paradata::FieldMovementEvent>(field_movement_instance));

        // set up the entry event (to be logged the next time this method is called)
        if( pDeFld != nullptr )
        {
            const VART* pVarT = VPT(pDeFld->GetSymbol());
            CaptureType requested_capture_type = pVarT->GetCaptureInfo().GetCaptureType();
            CaptureType actual_capture_type = pVarT->GetEvaluatedCaptureInfo().GetCaptureType();

            m_currentFieldEntryEvent = std::make_shared<Paradata::FieldEntryEvent>(
                field_movement_instance,
                m_pIntDriver->m_pParadataDriver->CreateFieldValidationInfo(pVarT),
                (int)requested_capture_type, (int)actual_capture_type
            );
        }
    }


    if( bLoopBrokeByStop )
        return nullptr;

    else
        return GetCurObject();              // TODO what if not a 3D-object???
}



// int getGroupMaxOccsUsingMaxDEField( GROUPT* pGroupT )
//
// Calculate max occs considering controller variable, if it has one
// called from isCurrentPositionEndGroup() method
//
// RCL, Sept 24, 2004
int CsDriver::getGroupMaxOccsUsingMaxDEField( GROUPT* pGroupT )
{
    ASSERT( pGroupT != 0 );

    int iMaxOccs = pGroupT->GetMaxOccs();

    if( hasMaxDEField( pGroupT ) )
    {
        CDEGroup* pCDEGroup = pGroupT->GetCDEGroup();
        ASSERT( pCDEGroup != 0 );
        ASSERT( pCDEGroup->GetMaxDEField() != 0 );
        const CDictItem* pItem = pCDEGroup->GetMaxDEField();
        CIMSAString sVal = FldGetVal(pItem);
        sVal.Trim();

        if(!sVal.IsEmpty() && sVal.IsNumeric())
        {
            int iVal = sVal.Val();
            if( iVal >= 0  && iVal <= iMaxOccs)
            {
                iMaxOccs = iVal;
            }
        }
    }

    return iMaxOccs;
}

// bool isCurrentPositionEndGroup( GROUPT* pGroupT )
// Only valid to be called when current atom is an item
// called from checkEndGroup() and checkEndGroupEndLevel() methods
//
// RCL, Sept 24, 2004
bool CsDriver::isCurrentPositionEndGroup( GROUPT* pGroupT )
{
    ASSERT( pGroupT != 0 );
    CFlowAtom* pAtom = GetCurrAtom();
    ASSERT( pAtom != 0 );
    ASSERT( pAtom->GetAtomType() == CFlowAtom::AtomType::Item );

    bool bRet = false;

    int iMaxOccs  = getGroupMaxOccsUsingMaxDEField( pGroupT );
    int iDataOccs = pGroupT->GetDataOccurrences();

    CNDIndexes
         theIndex = ((CFlowItem*) pAtom)->GetIndex();

    bool bProcess = hasMaxDEField( pGroupT );

    // Consider "lowest" available index
    // rcl, Jun 11, 2004
    int iOcc = theIndex.searchNotEmptyValue();


    if( iOcc <= 0 )
        iOcc = 1;
    if( iDataOccs > iMaxOccs ) {//could be possible in the new field controlled group
        iDataOccs = iMaxOccs;
    }
    if( pGroupT->GetMaxOccs() > 1 && iOcc > iDataOccs &&
        (C_IsAutoEndGroup() || bProcess ) ) {
        bRet = true;
    }

    return bRet;
}

// bool isCurrentPositionEndLevel()
// Only valid to be called when current atom is an item
// called from checkEndGroupEndLevel() method
//
// RCL, Sept 24, 2004
bool CsDriver::isCurrentPositionEndLevel()
{
    CFlowAtom* pAtom = GetCurrAtom();
    ASSERT( pAtom != 0 );
    ASSERT( pAtom->GetAtomType() == CFlowAtom::AtomType::Item );

    bool bRet = false;

    int iCurrAtomLevel = -1;
    if( m_pFlowCore != 0 )
        iCurrAtomLevel = m_pFlowCore->GetAtomLevel( pAtom );

    if( iCurrAtomLevel > 1)
    {
        int iCurLevel = m_pEntryDriver->GetActiveLevel();
        return m_pEntryDriver->IsLevelNodeNew(iCurLevel);
#ifdef OLD // REPO_TEMP
        int iNodeAbs  = -1;
        int iNodeRel  = m_pEntryDriver->GetEntryIFaz()->C_GetCurrentNodeNum( iCurLevel, &iNodeAbs );
        const TREE_NODE* pParentNode = NULL;
        if((iNodeAbs - iNodeRel-1) >= 0)
        {
            pParentNode = TNA(iNodeAbs - iNodeRel-1);
        }

        if(pParentNode)
        {
            TREE_NODE* pNode = const_cast<TREE_NODE*>(pParentNode);
            int iNumSons = pNode->GetNumberOfSons();
            if(iNodeRel > iNumSons || m_pEntryDriver->GetEntryIFaz()->C_IsNewNode())
            {
                bRet = true;
            }
        }
#endif
    }

    return bRet;
}

// void checkEndGroup( bool bIsModification, bool bPathOff,
//                     GROUPT* pGroupT,
//                     bool* pbEndGroup );
//
// checks if current position is an end group.
// It is important to check before calling this method
// that current atom is an Item type one
// Answer is written in *pbEndGroup
//
// RCL, Sept 24, 2004
void CsDriver::checkEndGroup(
                            bool bIsModification, bool bPathOff,
                            GROUPT* pGroupT,
                            bool* pbEndGroup )
{
    ASSERT( pGroupT != 0 );
    ASSERT( pbEndGroup != 0 );

    *pbEndGroup = false;

    bool bProcess = hasMaxDEField( pGroupT );

    if((bIsModification && bPathOff) || bProcess)
    {
        *pbEndGroup = isCurrentPositionEndGroup( pGroupT );
    }
}

// void checkEndGroupEndLevel( bool bIsModification, bool bPathOff,
//                             GROUPT* pGroupT,
//                             bool* pbEndGroup, bool* pbEndLevelOcc )
//
// checks if current position is an end group and End Level
// It is important to check before calling this method
// that current atom is an Item type one
// Answer is written in *pbEndGroup and *pbEndLevelOcc
//
// RCL, Sept 24, 2004
void CsDriver::checkEndGroupEndLevel(
                                     bool bIsModification, bool bPathOff,
                                     GROUPT* pGroupT,
                                     bool* pbEndGroup, bool* pbEndLevelOcc )
{
    ASSERT( pGroupT != 0 );
    ASSERT( pbEndGroup != 0 );
    ASSERT( pbEndLevelOcc != 0 );

    *pbEndGroup = false;
    if( pbEndLevelOcc != 0 )
        *pbEndLevelOcc = false;

    bool bProcess = hasMaxDEField( pGroupT );

    if((bIsModification && bPathOff) || bProcess)
    {
        *pbEndGroup = isCurrentPositionEndGroup( pGroupT );
        *pbEndLevelOcc = isCurrentPositionEndLevel();
        if(bProcess && !*pbEndGroup){
            //if we are on a group which has a look controlling field then we are not ready to end the field until this group is done - Savy 02/09/2012
            *pbEndLevelOcc  = false;
        }
    }

}

bool CsDriver::CheckImplicitActions( bool bNewRequestReceived ) { // RHF+vc Jun 21, 01
    // CheckImplicitActions: take care of killing or retrieving pending advances, and of protected fields
    bool            bRequestIssued = false;
    RequestNature   xRequestNature = GetRequestNature();
    CSettings* pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool       bPathOff        = pSettings->IsPathOff();

    CString csMsg;
    RTRACE( _T("CheckImplicitActions()") NL );
    RTRACE( _T("{") NL );
    if( bNewRequestReceived ) {
        RTRACE( INDENT _T("bNewRequestReceived:true") NL );
        RTRACE( INDENT _T("{") NL );
        // (a) does the request supersede pending advances?
        // TODO check the requests below are enough
        switch( xRequestNature ) {
            case CsDriver::Reenter:
            case CsDriver::PrevField:
            case CsDriver::LogicEndLevel:
            case CsDriver::EnterFlowEnd: // RHF Feb 13, 2003
                RTRACE( INDENT2 _T("KillPendingAdvances()") NL );
                KillPendingAdvances();  // ... kill the list of pending advances
                break;
        }
    }
    else {
        RTRACE( INDENT _T("bNewRequestReceived:false") NL );
        CFlowAtom*  pAtom = GetCurrAtom();
        CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
        int         iTargetNode = GetTargetNode();
        bool        bIsEnterStarted = EnterFlowJustStarted(); // victor Feb 19, 02
        bool        bSomeAdvanceToNode = ( GetTargetNode() != CEntryDriver::NodeAdvanceNone );

        csMsg.Format( INDENT _T("bIsEnterStarted = %s") NL, BOOL2STRING(bIsEnterStarted));
        RTRACE( csMsg );

        csMsg.Format( INDENT _T("bSomeAdvanceToNode = %s") NL, BOOL2STRING(bSomeAdvanceToNode));
        RTRACE( csMsg );

        // (b) if any pending advances, insures the best one becomes active
        // remark - pending advances are postponed when starting an EnterFlow // victor Aug 08, 01
        if( !bIsEnterStarted )
        {
            RTRACE( INDENT _T("!bIsEnterStarted") NL );
            RTRACE( INDENT _T("{") NL );
            bRequestIssued = GetBestPendingAdvance();
            csMsg.Format( INDENT2 _T("bRequestIssued = %s <- GetBestPendingAdvance()") NL, BOOL2STRING(bRequestIssued));
            RTRACE( csMsg );
            RTRACE( INDENT _T("}") NL );
        }

        // (c) take care of protected field at the current atom
        if( !bRequestIssued && !bIsEnterStarted && xAtomType == CFlowAtom::AtomType::Item ) {

            RTRACE( INDENT _T("!bRequestIssued && !bIsEnterStarted && xAtomType == CFlowAtom::AtomType::Item") NL );
            RTRACE( INDENT _T("{") NL );
            CNDIndexes theIndex;
            VARX*   pVarX = GetFieldFromAtom( pAtom, theIndex );
            VART*   pVarT = pVarX->GetVarT();

            /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/

            // Refactoring, RCL Sept 2004
            // {
            GROUPT* pGroupT = pVarT->GetOwnerGPT();

            bool bEndGroup = false;
            bool bEndLevelOcc = false;

            bool bIsModification = m_pEntryDriver->IsModification() &&
                                   m_pEntryDriver->GetPartialMode()!= ADD_MODE;

            ASSERT( pGroupT != 0 );

            checkEndGroupEndLevel( bIsModification, bPathOff,
                                   pGroupT, &bEndGroup, &bEndLevelOcc );

            // } Refactoring, RCL Sept 2004

             /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/
            if( pVarT->IsProtectedOrNoNeedVerif() || bEndGroup || bEndLevelOcc) {

                csMsg.Format( INDENT2 _T("pVarT->IsProtectedOrNoNeedVerif() || bEndGroup || bEndLevelOcc") NL,
                                BOOL2STRING(bEndGroup), BOOL2STRING(bEndLevelOcc) );
                RTRACE( csMsg );
                RTRACE( INDENT2 _T("{") NL );

                SetRequestSource( false );  // from Interface
                if( IsForwardWay() ) {
                    if(bEndGroup) {
                        SetRequestNature( CsDriver::InterEndGroup );
                    }
                    else if(bEndLevelOcc) {
                        m_pEntryDriver->GetEntryIFaz()->C_EndLevel( false, true, m_pEntryDriver->GetActiveLevel()-1 , false, true);
                    }
                    else {
                        SetRequestNature( CsDriver::NextField );
                    }
                }
                else
                    SetRequestNature( CsDriver::PrevField );

                bRequestIssued = true;

                RTRACE( INDENT2 _T("bRequestIssued = true") NL );
                RTRACE( INDENT2 _T("{") NL );
            }

            RTRACE( INDENT _T("}") NL );

        }

        if( IsForwardWay() && !bIsEnterStarted ) {      // victor Dec 10, 01
            // (d) some target-node for blind advance...
            CFlowCore*  pFlowCore = GetFlowCore();
            int         iAtomLevel = pFlowCore->GetAtomLevel( pAtom );
            bool        bIsLevelHead = pFlowCore->IsLevelHead( pAtom );
            bool        bIsLevelTail = pFlowCore->IsLevelTail( pAtom );
            bool        bIsModification = m_pEntryDriver->IsModification();
            if( !bRequestIssued && xRequestNature != Reenter ) {
                // (d1) ... check if any advance-to-next-node in process // victor Feb 23, 02
                if( iTargetNode == CEntryDriver::NodeAdvanceToNextNode ) {
                    Pre74_CaseLevel* pCaseLevel = m_pEntryDriver->GetCaseLevelNode(iAtomLevel);
                    int iThisNode = m_pEntryDriver->GetNodeNumber(pCaseLevel);

                    if( iThisNode == GetSourceOfNodeAdvance() ) {
                        // ... always in source node: asks for a NextField
                        if( !bRequestIssued && !bIsLevelTail ) {
                            SetRequestNature( CsDriver::NextField );
                            bRequestIssued = true;

                            csMsg.Format( INDENT3
                                _T("!bRequestIssued && xRequestNature != RequestNature::Reenter && ")
                                _T(" iThisNode == GetSourceOfNodeAdvance() && ")
                                _T(" !RequestIssued && !bIsLevelTail") NL );

                            RTRACE( csMsg );

                            RTRACE( INDENT3 _T("SetRequestNature( NextField )") NL );
                            RTRACE( INDENT3 _T("bRequestIssued = true") NL );

                        }
                    }
                    else {
                        // ... another node reached: reset related info
                        ResetSourceOfNodeAdvance();
                        SetTargetNode( CEntryDriver::NodeAdvanceNone );
                    }
                }
            }

            if( !bRequestIssued && xRequestNature != CsDriver::Reenter ) {
                // (d2) ... check if an advance-to-a-target-node being processed
                if( SearchingTargetNode() ) {
                    // ... not yet reached: asks for a NextField
                    if( !bRequestIssued ) {
                        SetRequestNature( CsDriver::NextField );
                        bRequestIssued = true;

                        csMsg.Format( INDENT3
                            _T("!bRequestIssued && xRequestNature != RequestNature::Reenter && ")
                            _T(" SearchingTargetNode() && !RequestIssued ") NL );

                        RTRACE( csMsg );

                        RTRACE( INDENT3 _T("SetRequestNature( NextField )") NL );
                        RTRACE( INDENT3 _T("bRequestIssued = true") NL );
                    }
                }
            }

            // (e) re-starting a level-head...
            if( !bRequestIssued && bIsLevelHead ) {
                if( iAtomLevel > 1 || bIsModification ) {
                    SetRequestNature( CsDriver::NextField );
                    bRequestIssued = true;

                    csMsg.Format( INDENT3 _T("!bRequestIssued && bIsLevelHead && ")
                        _T("((iAtomLevel [%d] > 1) || bIsModification [%s])") NL,
                        iAtomLevel,
                        BOOL2STRING(bIsModification));
                    RTRACE( csMsg );

                    RTRACE( INDENT3 _T("SetRequestNature( NextField )") NL );
                    RTRACE( INDENT3 _T("bRequestIssued = true") NL );
                    }
            }

            // (f) about to close a session...
            if( !bRequestIssued && bIsLevelTail && iAtomLevel < 1 ) {
            //BUCEN Changes Jan, 2004
                            CNPifFile*   pPifFile = m_pEngineDriver->GetPifFile();
                                bool bAutoAdd = true ;
                                if(pPifFile) {
                                        bAutoAdd =pPifFile->GetAutoAddFlag();
                                }
            //BUCEN Changes Jan, 2004
                if( bIsModification || !bAutoAdd || m_pEntryDriver->IsInsertMode() ) {
                    // ... closes the modification session (one case only)
                    RunEventProcs( 2 );
                }
//              else {
//                  // ... prepares to add another case
//                  SetRequestNature( CsDriver::RequestNature::NextField );
//                  bRequestIssued = true;
//              }
            }
        }
    }

    csMsg.Format( _T("} -> %s (bNewRequestReceived [%s] || bRequestIssued [%s])") NL,
                    BOOL2STRING(bNewRequestReceived || bRequestIssued),
                    BOOL2STRING(bNewRequestReceived),
                    BOOL2STRING(bRequestIssued));
    RTRACE( csMsg );
    return( bNewRequestReceived || bRequestIssued );
}

bool CsDriver::SearchingTargetNode( void ) {            // victor Dec 10, 01
    // remark - tailored to satisfy F10-EndOfCase       // victor Feb 23, 02
    int         iTopNode      = m_pEntryDriver->GetTotalNumberNodes() - 1;
    int         iTargetNode   = GetTargetNode();
    bool        bNodeSearched = ( iTargetNode >= 0 );
    bool        bToEndOfCase  = ( iTargetNode == CEntryDriver::NodeAdvanceToEndOfCase );
    CFlowAtom*  pAtom         = GetCurrAtom();
    int         iAtomLevel    = GetFlowCore()->GetAtomLevel( pAtom );

    if( bNodeSearched && iAtomLevel < 1 ) {
        if( iTargetNode < CEntryDriver::NodeAdvanceToEndOfCase )
            // ... the searched node is no longer available
            issaerror( MessageType::Error, 91190, iTargetNode );

        // ... voids the target-node and set the condition to "no node searched"
        SetTargetNode( CEntryDriver::NodeAdvanceNone );
        bNodeSearched = false;
    }

    if( bNodeSearched )
    {
        Pre74_CaseLevel* pCaseLevel = m_pEntryDriver->GetCaseLevelNode(iAtomLevel);
        int iThisNode = m_pEntryDriver->GetNodeNumber(pCaseLevel);
        bool bAtLastNode = ( iThisNode == iTopNode );

        // don't exceed the total number of nodes
        if( bToEndOfCase )
            iTargetNode = iTopNode;

        if( iThisNode < iTargetNode )
            bNodeSearched = true;
        else if( pAtom->GetAtomType() == CFlowAtom::AtomType::Item ) {
            // ... reached, it's a field in this node:
            if( !bToEndOfCase ) {       // ... voids the target-node
                SetTargetNode( CEntryDriver::NodeAdvanceNone );

                // ... then set the return code to "no longer searching"
                bNodeSearched = false;
            }
            else {                      // ... switch to advance-to-next-node
                SetTargetNode( CEntryDriver::NodeAdvanceToNextNode );

                // ... then confirm the return code to "yet searching"
                bNodeSearched = true;
            }
        }
    }
    return bNodeSearched;
}

void CsDriver::GotoLevelZeroTail( void ) {              // RHF+vc Jun 14, 01
    // GotoLevelZeroTail: solve the special request of immediatly ending the session
    int         iLevel    = 0;
    int         iAtom     = GetFlowCore()->GetLevelTailIndex( iLevel );

    SetRequestOrigin();

    SetCurrAtomIndex( iAtom );
    SetProgressBitmapOn();

    SetForwardWay( true );              // insures forward way

    // run the "closing" event-procs for this level-tail atom
    RunEventProcs( 2 );

    // clean everything
    LevelRestart( iLevel );
}

bool CsDriver::RequestHasValidParms( void ) {
    // RequestHasValidParms: check the request has the expected parameters
    bool            bParmsOK = false;
    RequestNature   xRequestNature = GetRequestNature();
    switch( xRequestNature ) {
        // (a) requested from Interface and Logic
        case AdvanceTo:
            // remark: no need to check this target!    // victor May 18, 01
            //         (empty target is "to infinite")  // victor May 18, 01
            bParmsOK = true;                            // victor May 18, 01
            break;

        case SkipTo:
            bParmsOK = ( !m_TgtSkipTo.IsEmpty() );
            break;

        case Reenter:
            // remark: no need to check this target!    // victor May 18, 01
            //         (empty target is "to itself")    // victor May 18, 01
            bParmsOK = true;                            // victor May 18, 01
            break;

        // (b) requested from Logic only
        case AdvanceToNext:
            bParmsOK = ( m_iTgtAdvanceToNext > 0 );
            break;

        case SkipToNext:
            bParmsOK = ( m_iTgtSkipToNext > 0 );
            break;

        case LogicEndGroup:
        case LogicEndLevel:
            bParmsOK = true;            // no parm required
            break;

        case EnterFlowEnd:
            bParmsOK = true;            // no parm required
            break;

        // (c) requested from Interface only
        case NextField:
        case PrevField:
            bParmsOK = true;            // no parm required
            break;

        case InterEndOccur:
        case InterEndGroup:
            bParmsOK = true;            // parm is always present (at least a default one)
            break;

        case InterEndLevel:
            bParmsOK = true;            // TODO - true checking of EndLevelParms are OK
            break;

        // (d) other requests from Interface
        //  --- none ---                                // victor May 10, 01

        // (*) undefined requests
        case None:
        default:
            ASSERT( 0 );                // no request defined
            break;
    }
    return bParmsOK;
}

// bool isUsingValid3DIndexesForVariable_local( VART* pVarT; const C3DObject& theObject )
// Check every index used until one is bad or everything is ok
// returns true if everything is ok, false otherwise
static bool isUsingValid3DIndexesForVariable_local( VART* pVarT, C3DObject& theObject )
{
    bool    bRet    = true;
    GROUPT* pGroupT = pVarT->GetOwnerGPT();
    int     iNumDim = pVarT->GetNumDim();   // number of user-allowed dimensions

    for( int i = iNumDim; i > 0; i-- )
    {
        ASSERT( pGroupT != 0 );
        int iMaxOccs = pGroupT->GetMaxOccs();

        int iOccur = 1;

        if( pVarT->IsArray() )
            iOccur = theObject.getIndexValue( pVarT->GetDimType( i - 1 ) );

        if( iOccur > iMaxOccs )
        {
            bRet = false; // exceeds max-occs of the owner group
            break;
        }

        pGroupT = pGroupT->GetOwnerGPT();
    }

    return bRet;
}

static bool isUsingValid3DIndexesForGroup_local( GROUPT* pGroupT, C3DObject& theObject )
{
    // TODO change mono-index evaluation below by a true multi-index checking
    bool bRet    = true;
    int  iOccur  = 1;


    if( pGroupT->GetNumDim() )
        iOccur = theObject.getIndexValue( pGroupT->GetDimType() );

    int iMaxOccs = pGroupT->GetMaxOccs();

    if( iOccur > iMaxOccs )
        bRet = false;

    return bRet;
}

const int CHECK_LOWER     = 0x01;
const int CHECK_EQUAL     = 0x02;
const int CHECK_GREATER   = 0x04;
const int ALLOW_NOLIGHT   = 0x08;
const int ALLOW_HIGHLIGHT = 0x10;
#define HAS_FLAG(theValue,flagToCheck)  ((theValue&flagToCheck)==flagToCheck)

void CsDriver::IsValidTargetGeneric( C3DObject*  p3DTarget, bool bSilent, int iFlags )
{
    // 20130910 changed all m_TgtSkipTo references to p3DTarget because advances following skips could cause erroneous error messages

    int iSymTarget = p3DTarget->GetSymbol();
    SymbolType eTargetType = NPT(iSymTarget)->GetType();

    // checking basic restrictions      // TODO make sure these checks are enough
    if( eTargetType != SymbolType::Group && eTargetType != SymbolType::Block && eTargetType != SymbolType::Variable )
        throw MessageIdException( 0 );

    if( eTargetType == SymbolType::Group || eTargetType == SymbolType::Block )
    {
        GROUPT* pGroupT = ( eTargetType == SymbolType::Group ) ? GPT(iSymTarget) :
                                                                 GetSymbolEngineBlock(iSymTarget).GetGroupT();

        if( pGroupT->GetSource() != GROUPT::Source::FrmFile )
            throw MessageIdException( 1 );       // Group not coming from FormFile

        // checking structural bounds
        if( isUsingValid3DIndexesForGroup_local( pGroupT, *p3DTarget /*m_TgtSkipTo*/ ) == false )
            throw MessageIdException( 3 );       // exceeds max-occs of the group

        // the above function doesn't check for lower limits, so do that for blocks
        if( eTargetType == SymbolType::Block )
        {
            if( pGroupT->GetMaxOccs() > 1 && p3DTarget->getIndexValue(pGroupT->GetDimType()) < 1 )
                throw MessageIdException(3);
        }

        // checking target location
        int iLocation = SearchTargetLocation( p3DTarget );

        if( HAS_FLAG(iFlags,CHECK_LOWER) && iLocation < 0 )
            throw MessageIdException( 5 );       // target-group is before the current atom

        // RHF INIC Dec 31, 2003
        if( HAS_FLAG(iFlags,CHECK_EQUAL) && iLocation == 0 )
            throw MessageIdException( -1 ); // target group is the same!
        // RHF END Dec 31, 2003

        if( HAS_FLAG(iFlags,CHECK_GREATER) && iLocation > 0 )
            throw MessageIdException( 5 );      // target-group is after the current atom

    }

    else
        // if( eTargetType == SymbolType::Variable )  // superfluous check
    {
        VART* pVarT = VPT(iSymTarget);

        if( !pVarT->IsInAForm() )
            throw MessageIdException( 2 );       // Item without field

        // checking structural bounds
        if( isUsingValid3DIndexesForVariable_local( pVarT, *p3DTarget /*m_TgtSkipTo*/ ) == false )
            throw MessageIdException( 4 );       // exceeds max-occs of the owner group

        // checking target location

        int iLocation = SearchTargetLocation( p3DTarget );

        if( HAS_FLAG(iFlags,CHECK_LOWER) && iLocation < 0 )
            throw MessageIdException( 6 );   // target-item is before the current atom

        // RHF INIC Dec 31, 2003
        if( HAS_FLAG(iFlags,CHECK_EQUAL) && iLocation == 0 )
            throw MessageIdException( -1 );   // target item is the same!

        if( HAS_FLAG(iFlags,CHECK_GREATER) && iLocation > 0 )
            throw MessageIdException( 6 );      // target-group is after the current atom
        // RHF END Dec 31, 2003

        // check color of target field  // TODO suppress below? test of location should be enough
        CSettings*  pSettings = &m_pEngineDriver->m_EngineSettings;
        bool        bPathOn   = pSettings->IsPathOn();
        int         iPivot = GetCurrAtomIndex();
        int         iAtom  = SearchTargetAtomIndex( p3DTarget, iPivot, iLocation );
        CFlowAtom*  pAtom  = GetAtomAt( iAtom );
        int         iFieldColor = (pAtom == NULL ) ? -1 : GetFieldColor( pAtom ); // RHF Nov 22, 2002 Add pAtom==NULL

        switch( iFieldColor ) {
        case FLAG_NOLIGHT:
            if( HAS_FLAG(iFlags,ALLOW_NOLIGHT) )// RHF Aug 18, 2000
                break;
            throw MessageIdException(7);
            break;

        case FLAG_MIDLIGHT:
            if( bPathOn )                           // RHF Feb 23, 02
                throw MessageIdException( 8 );   // can't skip to a skipped field
            break;

        case FLAG_HIGHLIGHT:
            if( HAS_FLAG(iFlags,ALLOW_HIGHLIGHT) )
                break;
            throw MessageIdException( 7 );       // can't skip to a highlighted field
            break;

        default:
            ASSERT( 0 );            // can't be - other color is for non-fields only!
            throw MessageIdException( 9 );       // report to CsPro staff
            break;
        }
    }
}

bool CsDriver::IsValidSkipTarget(bool bSilent/* = false*/, C3DObject* p3DTarget/* = nullptr*/) {              // victor Dec 10, 01

    // 91160 Unable to 'skip to %s' - invalid target {in %p}
    //     +
    // 91159 TODO: document this message too - "target item is the same!" reads below
    // 91161 Unable to 'skip to %s' - target-group is not specified in a FormFile {in %p}
    // 91162 Unable to 'skip to %s' - target-item has no field {in %p}
    // 91163 Unable to 'skip to %s' - target-group occurrence exceeds group defined maximum {in %p}
    // 91164 Unable to 'skip to %s' - target-item occurrence exceeds owner' group defined maximum {in %p}
    // 91165 Unable to 'skip to %s' - target-group is located backward in the flow {in %p}
    // 91166 Unable to 'skip to %s' - target-item is located backward in the flow {in %p}
    // 91167 Unable to 'skip to %s' - target-item field is a highlighted field {in %p}
    // 91168 Unable to 'skip to %s' - target-item field is a skipped field {in %p}
    // 91169 Unable to 'skip to %s' - please report the problem to CsPro staff {in %p}
    if( p3DTarget == nullptr )
        p3DTarget = &m_TgtSkipTo;

    bool bRet      = true;

    try
    {
        int iFlags = CHECK_LOWER | CHECK_EQUAL | ALLOW_NOLIGHT;
        IsValidTargetGeneric( p3DTarget, bSilent, iFlags );
    }
    catch( MessageIdException& e )
    {
        if( !bSilent )
            InvalidTargetMessage( 91160 + e.getErrorCode(), p3DTarget );

        bRet = false;
    }

    return bRet;
}

bool CsDriver::IsValidAdvanceTarget(bool bSilent/* = false*/, C3DObject* p3DTarget/* = nullptr*/) {           // victor Dec 10, 01
    // 91170 Unable to 'advance to %s' - invalid target {in %p}
    // 91171 Unable to 'advance to %s' - target-group is not specified in a FormFile {in %p}
    // 91172 Unable to 'advance to %s' - target-item has no field {in %p}
    // 91173 Unable to 'advance to %s' - target-group occurrence exceeds group defined maximum {in %p}
    // 91174 Unable to 'advance to %s' - target-item occurrence exceeds owner' group defined maximum {in %p}
    // 91175 Unable to 'advance to %s' - target-group is located backward in the flow {in %p}
    // 91176 Unable to 'advance to %s' - target-item is located backward in the flow {in %p}
    // 91177 Unable to 'advance to %s' - target-item field is a highlighted field {in %p}
    // 91178 Unable to 'advance to %s' - target-item field is a skipped field {in %p}
    // 91179 Unable to 'advance to %s' - please report the problem to CsPro staff {in %p}
    if( p3DTarget == nullptr )
        p3DTarget = &m_TgtAdvanceTo;

    if( p3DTarget->IsEmpty() )          // no checking for infinite advance
        return true;

    bool bRet = true;

    try
    {
        int iFlags = CHECK_LOWER | ALLOW_NOLIGHT;
        IsValidTargetGeneric( p3DTarget, bSilent, iFlags );
    }
    catch( MessageIdException& e )
    {
        if( !bSilent )
            InvalidTargetMessage( 91170 + e.getErrorCode(), p3DTarget );

        bRet = false;
    }

    return bRet;
}

bool CsDriver::IsValidReenterTarget(bool bAllowReenterToNoLight/* = false*/, bool bSilent/* = false*/, C3DObject* p3DTarget/* = nullptr*/) { // victor Dec 10, 01
    // 91180 Unable to 'reenter %s' - invalid target {in %p}
    // 91181 Unable to 'reenter %s' - target-group is not specified in a FormFile {in %p}
    // 91182 Unable to 'reenter %s' - target-item has no field {in %p}
    // 91183 Unable to 'reenter %s' - target-group occurrence exceeds group defined maximum {in %p}
    // 91184 Unable to 'reenter %s' - target-item occurrence exceeds owner' group defined maximum {in %p}
    // 91185 Unable to 'reenter %s' - target-group is located forward in the flow {in %p}
    // 91186 Unable to 'reenter %s' - target-item is located forward in the flow {in %p}
    // 91187 Unable to 'reenter %s' - target-item field is a never-keyed field {in %p}
    // 91188 Unable to 'reenter %s' - target-item field is a skipped field {in %p}
    // 91189 Unable to 'reenter %s' - please report the problem to CsPro staff {in %p}
    if( p3DTarget == nullptr )
        p3DTarget = &m_TgtReenter;

    bool bRet = true;

    try
    {
        int iFlags = CHECK_GREATER | ALLOW_HIGHLIGHT;
        if( bAllowReenterToNoLight )
            iFlags |= ALLOW_NOLIGHT;

        IsValidTargetGeneric( p3DTarget, bSilent, iFlags );
    }
    catch( MessageIdException& e )
    {
        if( !bSilent )
            InvalidTargetMessage( 91180 + e.getErrorCode(), p3DTarget );

        bRet = false;
    }

    return bRet;
}

void CsDriver::InvalidTargetMessage( int iMessage, C3DObject* p3DTarget ) {
    int iSymTarget = p3DTarget->GetSymbol();
    SymbolType eTargetType = NPT(iSymTarget)->GetType();
    GROUPT*     pGroupT = ( eTargetType == SymbolType::Group ) ? GPT(iSymTarget) : NULL;
    VART*       pVarT   = ( eTargetType == SymbolType::Variable ) ? VPT(iSymTarget) : NULL;

    CNDIndexes theIndex = p3DTarget->GetIndexes();

    // builds target description
    CString csTarget;

    if( iSymTarget <= 0 )
        csTarget = _T("<noname>");
    else {
        csTarget = WS2CS(NPT(iSymTarget)->GetName());

        int     iNumDim = ( pGroupT != NULL ) ? pGroupT->GetNumDim() :
                          ( pVarT   != NULL ) ? pVarT->GetNumDim() : 0;

        if( iNumDim )
            csTarget.Append(theIndex.toString(iNumDim).c_str());
    }

    // issue the requested message
    issaerror( MessageType::Error, iMessage, csTarget.GetString() );
}



/////////////////////////////////////////////////////////////////////////////
//
// --- methods solving requests
//
// ... remark:
//         run "up to interface" event-procs: PreProc/OnFocus/Interface
//         run "after interface" event-procs: Refresh/KillFocus/PostProc/OnOccChange
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::SolveSkipOrAdvanceTo( void ) {
    bool                bAdvanceTo = ( GetRequestNature() == AdvanceTo );
    bool                bSkipTo    = ( GetRequestNature() == SkipTo );
    C3DObject           o3DTarget  = ( bSkipTo ) ? m_TgtSkipTo : m_TgtAdvanceTo;
    C3DObject*          p3DTarget  = &o3DTarget;
    p3DTarget->SetAtomIndex(-1);
    bool                bInfinite  = p3DTarget->IsEmpty();// victor May 18, 01
    ASSERT( bAdvanceTo || ( bSkipTo && !bInfinite ) );
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    EventType           xEventType      = GetRequestEventType();
    bool                bEnterTheField  = bAdvanceTo;
    bool                bTargetReached  = false;
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    CFlowAtom*          pSkippedAtom    = NULL;
    bool                bSkippedItem    = false;
    CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff        = pSettings->IsPathOff();
    if( bSkipTo && xAtomType == CFlowAtom::AtomType::Item ) {
        pSkippedAtom  = pAtom;
        if( xEventType == EventPreProc || xEventType == EventOnFocus )
            bSkippedItem = true;


        // RHF INIC Dec 31, 2003
        if( AtSkip_GetFromInterface() && AtSkip_GetEnterField() )
            bEnterTheField = true;
        // RHF END Dec 31, 2003

    }

    if( xAtomType == CFlowAtom::AtomType::BeyondStrip )
        return bRequestIssued;
    SetForwardWay( true );              // insures forward way
    if( bSkipTo && bPathOff && AtSkip_GetFromInterface() && xAtomType == CFlowAtom::AtomType::Item ) m_bMidColor = true; // RHF Dec 31, 2003
    // run "after interface" event-procs for the preceeding atom
    bRequestIssued = SolveNextField( bEnterTheField );

    // RHF INIC Dec 31, 2003
    if( bSkipTo && AtSkip_GetFromInterface() ) {
         m_bMidColor = false;
        //CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
        //bool                bPathOff        = pSettings->IsPathOff();
        if( bPathOff && xAtomType == CFlowAtom::AtomType::Item ) bSkippedItem = true;
        if( AtSkip_GetEnterField() ) {
            bEnterTheField = false;
        }
    }
    // RHF END Dec 31, 2003


    if( pSkippedAtom != NULL ) {                        // victor Dec 10, 01
        if( bSkippedItem )
            SetFieldColorMid( pSkippedAtom );
        else
            SetFieldColorHigh( pSkippedAtom );
    }
    if( EnterFlowJustStarted() )        // ... enter-flow gets immediatly back!
        return false;

    if( bRequestIssued )
        bBreakingEvents = IsRequestBreakingEventsChain();
    else if( WasLevelTailReached() )                    // victor Jun 07, 01
        bBreakingEvents = true;                         // victor Jun 07, 01
    else if( m_pIntDriver->m_bStopProc ) // 20140430 savy's stop fix on 20140221 could lead to an infinite loop when a stop was executed after an advance
        bTargetReached = true;
    else if( bSkipTo || !bInfinite )    // check if target was reached
        // ... reached: when the target is before than -or equals- the current atom
        bTargetReached = ( SearchTargetLocation( p3DTarget ) <= 0 );

    // moving forward thru the flow-strip up to the target

    while( !bBreakingEvents && !bTargetReached ) {
        // reset the light                              // victor Dec 10, 01
        pAtom = GetCurrAtom();
        pSkippedAtom = ( bSkipTo ) ? pAtom : NULL;

        if( pSkippedAtom != NULL )                      // victor Dec 10, 01
            SetFieldColorMid( pSkippedAtom );

        // looks for the next field
        bRequestIssued = SolveNextField( bEnterTheField );
        if( EnterFlowJustStarted() )    // ... enter-flow gets immediatly back!
            return false;

#ifdef _DEBUG
        pAtom = GetCurrAtom();  // debug support only
#endif

        // RHF INIC Jun 21, 2001        // solves "noinput" appl' commands???
        //Savy - using optimize target compuatation during skip to.
        //for each iteration, target search is reevaluated causing slow down in long skips.
        //added new flag to reuse computed target atom index. Used only for skips
        bool     bTargetReachedAux = ( bInfinite ) ? false : ( SearchTargetLocation( p3DTarget,-1,0, bSkipTo) <= 0 );
        if( bTargetReachedAux )
            bEnterTheField = true;
        // RHF END Jun 21, 2001

        if( bRequestIssued )            // check if an events-break arised
            bBreakingEvents = IsRequestBreakingEventsChain();
        else if( WasLevelTailReached() )                // victor Jun 07, 01
            bBreakingEvents = true;                     // victor Jun 07, 01
        else if( m_pIntDriver->m_bStopProc ) // 20140430 so that a stop is properly handled as part of an advance
            bTargetReached = true;
        else if( bSkipTo || !bInfinite )// check if target was reached
            // ... reached: when the current atom is farther than -or equals- the target
            bTargetReached = bTargetReachedAux;
    }

    return bRequestIssued;
}

bool CsDriver::SolveReenterTo( void ) {
    bool                bReenter  = ( GetRequestNature() == Reenter );
    ASSERT( bReenter );
    C3DObject           o3DTarget       = m_TgtReenter;
    C3DObject*          p3DTarget       = &o3DTarget;
    p3DTarget->SetAtomIndex(-1);
    bool                bToItself       = p3DTarget->IsEmpty();// victor May 18, 01
    // RHF INIC Mar 23, 2003
#ifdef WIN32
    bool                bIgnoreProtected=(p3DTarget->GetSymbol() == -MAXLONG);
#else
    bool                bIgnoreProtected=(p3DTarget->GetSymbol() == -(0x7fffffff));
#endif

    if( bToItself || bIgnoreProtected )
        bToItself = true;

    if( bIgnoreProtected ) {
        m_TgtReenter.SetEmpty();
        o3DTarget       = m_TgtReenter;
    }
    // RHF END Mar 23, 2003


    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    bool                bTargetReached  = false;
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff        = pSettings->IsPathOff();
    bool                bToItselfAux = bToItself;
    EventType           eType= GetRequestEventType();

    // reenter called from PostProc and the group is target container. Example: reenter X from PostProc G and X belong to G
    if( !bToItself && xAtomType == CFlowAtom::AtomType::GroupTail && eType == EventPostProc ) {
        int iRelTarget = EvaluateRelationship( p3DTarget );
        if( iRelTarget == 4 ) // Target container
            bToItself = true;
    }

    if( xAtomType == CFlowAtom::AtomType::BlockTail )
    {
        // reentering a field on the block
        if( !bToItself && eType == EventPostProc )
            bToItself = ( EvaluateRelationship(p3DTarget) == 4 );

        // reentering the block itself
        else if( bToItself )
        {
            // translate block reenters to the actual block object
            C3DObject* p3DCurObject = GetCurObject();
            m_TgtReenter.SetSymbol(p3DCurObject->GetSymbol());
            m_TgtReenter.setIndexes(p3DCurObject->GetIndexes());
            return SolveReenterTo();
        }
    }

    if( bToItself ) {                                   // victor May 18, 01
        // ... back to "interface"                      // victor May 18, 01
        // RHF INIC Mar 23, 2003
        ASSERT( xAtomType == CFlowAtom::AtomType::Item ||
                xAtomType == CFlowAtom::AtomType::BlockTail ||
                xAtomType == CFlowAtom::AtomType::GroupHead ||
                xAtomType == CFlowAtom::AtomType::GroupTail );
        // Only executes OnFocus when reenter is called from Item PostProc

        if( xAtomType == CFlowAtom::AtomType::Item && eType != EventPostProc )
            SetProgressBitmapOff( EventOnFocus ); // RHF Mar 23, 2003

        if( ( xAtomType == CFlowAtom::AtomType::Item || xAtomType == CFlowAtom::AtomType::BlockTail ||
            xAtomType == CFlowAtom::AtomType::GroupTail ) && eType == EventPostProc )
        {
            SetForwardWay( false );         // insures backward way
            SetProgressBitmapOn( EventOnFocus );
            bRequestIssued = RunEventProcs();
            if( bRequestIssued )        // check if an events-break arised
                bBreakingEvents = IsRequestBreakingEventsChain();

        }

        if( xAtomType == CFlowAtom::AtomType::GroupTail && ( eType == EventKillFocus || eType == EventPostProc ) )
            bToItself = false;

        if( xAtomType == CFlowAtom::AtomType::GroupHead && ( eType == EventOnFocus || eType == EventPreProc ) )
            bToItself = false;

        if( xAtomType == CFlowAtom::AtomType::BlockTail && ( eType == EventKillFocus || eType == EventPostProc ) )
            bToItself = false;

        // RHF END Mar 23, 2003
        CSettings*      pSettings = &m_pEngineDriver->m_EngineSettings;

        // reset the light
        if( bPathOff )
            SetFieldColorLow( pAtom );
        else
            SetFieldColorNone( pAtom );

        // make sure "after interface" is called again  // victor Dec 10, 01
        SetProgressBitmapOnFrom( EventRefresh );
    }
    // RHF INIC Mar 23, 2003
    if( !bToItself ) {
     // RHF END Mar 23, 2003
        SetForwardWay( false );         // insures backward way

        // moving reverse thru the flow-strip up to the target
        while( !bBreakingEvents && !bTargetReached ) {
            // reset the light                          // victor Dec 10, 01
            pAtom = GetCurrAtom();
            xAtomType = pAtom->GetAtomType();
            if( xAtomType == CFlowAtom::AtomType::Item ) {
                if( bPathOff )
                    SetFieldColorLow( pAtom );
                else
                    SetFieldColorNone( pAtom );
            }

            // looks for the previous field...
            // RHF INIC Mar 23, 2003
            bRequestIssued = SolvePrevField( true, bIgnoreProtected ); // ... stopping at the reenter' target
            // RHF END Mar 23, 2003

            if( bRequestIssued ) {        // check if an events-break arised
                bBreakingEvents = IsRequestBreakingEventsChain();

                // to address a bug in this scenario:
                // [skipped block] [A, conditionally advance to C in the preproc] [B] [C, reenter A]
                // the advance in A wouldn't break this events chain and the block's procs would run, so
                // check if an advance was specified
                bBreakingEvents |= ( m_xRequestNature == CsDriver::AdvanceTo ||
                                     m_xRequestNature == CsDriver::AdvanceToNext ||
                                     m_xRequestNature == CsDriver::NextField ); // noinput
            }
            else {
                // check if target was reached
                // RHF Mar 23, 2003, Enter in group postproc and then an Interface Reenter
                if( bToItselfAux ) {
                    bTargetReached = true;
                }
                else {
                    // ... reached: when the target is after -or equals- the current atom
                    //Savy to optimize the search target location when reentering
                    bTargetReached = ( SearchTargetLocation( p3DTarget,-1,0,true ) >= 0 );
                }

                // take care of protected field at the target atom  // victor Feb 19, 02
                // (additional check in order to issue a NextField)
                if( bTargetReached ) {
                    pAtom = GetCurrAtom();
                    xAtomType = pAtom->GetAtomType();

                    if( xAtomType == CFlowAtom::AtomType::Item ) {
                        CNDIndexes theIndex;
                        VARX* pVarX = GetFieldFromAtom( pAtom, theIndex );
                        VART* pVarT = pVarX->GetVarT();

                        /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP END*/
                        // Refactoring, RCL Sept 2004
                        // {
                        GROUPT* pGroupT = pVarT->GetOwnerGPT();

                        bool bEndGroup = false;

                        bool bIsModification = m_pEntryDriver->IsModification();

                        ASSERT( pGroupT != 0 );

                        checkEndGroup( bIsModification, bPathOff, pGroupT, &bEndGroup );

                        // }  Refactoring, RCL Sept 2004

                        /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP END*/
                        if( pVarT->IsProtectedOrNoNeedVerif() || bEndGroup) {
                            DontRemakeOrigin();         // "internal" request must not change the origin
                            SetRequestSource( false );  // from Interface

                            if(bEndGroup) {
                                SetRequestNature( CsDriver::InterEndGroup );
                            }
                            else {
                                SetRequestNature( CsDriver::NextField );
                            }
                            bRequestIssued = true;
                        }
                    }
                }
            }
        }

        // When prevfield is trying to target a FORM or a group
        // it will stop in it, but this method should get
        // an item, so it will advance to the next field available..
        // JOC+rcl, May 2005
        if( bTargetReached && ( GetCurrAtom()->GetAtomType() == CFlowAtom::AtomType::GroupHead ||
            GetCurrAtom()->GetAtomType() == CFlowAtom::AtomType::BlockHead ) )
        {
            SetRequestNature( NextField );
            if( LocateNextAtom() ) // try to reach next atom to move away from GroupHead
                bRequestIssued = SolveNextField( bIgnoreProtected );
        }
    }

    return bRequestIssued;
}

bool CsDriver::SolveEnterFlowEnd( void ) {
    ASSERT( GetRequestNature() == EnterFlowEnd );
    bool                bRequestIssued = false;

    return bRequestIssued;
}

bool CsDriver::SolveAdvanceToNext( void ) {
    ASSERT( 0 );                        // TODO         // victor May 10, 01
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    CFlowCore*          pFlowCore       = GetFlowCore();
    bool                bEnterTheField  = true; // always true for AdvanceToNext
    bool                bTargetReached  = false;
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    ASSERT( xAtomType == CFlowAtom::AtomType::Item );

    if( !( xAtomType == CFlowAtom::AtomType::Item ) )
        return bRequestIssued;          // don't change the current-atom

    SetForwardWay( true );              // insures forward way

    // run "closing" event-procs for the origin-atom
    // run "after interface" event-procs for this item
    bRequestIssued = RunEventProcs( 2, bEnterTheField );

    if( bRequestIssued )
        return bRequestIssued;

    // (1) travels up to the group-tail of the source group
    int         iSymItem;
    int         iSymGroupSource = pFlowCore->GetAtomGroupSymbol( pAtom );
    bool        bGroupTailFound = false;

    while( !bGroupTailFound && !bTargetReached && !bBreakingEvents && LocateNextAtom() ) {
        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();

        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            // run "up to interface" event-procs for the new item
            bRequestIssued = RunEventProcs( 1 );

            // check if target was reached
            iSymItem   = ((CFlowItem*) pAtom)->GetSymbol();
            bTargetReached = ( iSymItem == m_iTgtAdvanceToNext );

            if( bTargetReached )
                continue;               // ... break the trip

            // looks for the next field
            bRequestIssued = SolveNextField( bEnterTheField );

            if( bRequestIssued )
                return bRequestIssued;
        }
        else {
            // (b) other atom - run all event-procs
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                    if( xAtomType == CFlowAtom::AtomType::GroupTail )
                        bGroupTailFound = ( pFlowCore->GetAtomGroupSymbol( pAtom ) == iSymGroupSource );

                    if( bGroupTailFound )
                        GroupTrimOcc(); // takes care of empty occurrences
                    else {
                        bRequestIssued = RunEventProcs();
                        if( bRequestIssued )
                            bBreakingEvents = IsRequestBreakingEventsChain();
                        else if( WasLevelTailReached() )
                            bBreakingEvents = true;
                    }                                   // victor Jun 21, 01
                    break;

                case CFlowAtom::AtomType::BeyondStrip:
                    break;

                default:
                    ASSERT( 0 );        // can't be
                    break;
            }
        }
    }
    // (2) the group-tail was reached without interference: asks for next field
    if( bGroupTailFound && !bTargetReached && !bBreakingEvents )
        bRequestIssued = AskForNextField(); // "internal" NextField request, "after interface"

    return bRequestIssued;
}

bool CsDriver::SolveSkipToNext( void ) {
    ASSERT( 0 );                        // TODO         // victor May 10, 01
    bool                bRequestIssued = true;

    return bRequestIssued;
}

bool CsDriver::SolveLogicEndGroup( void ) {
    // ... late implementation                          // victor Jun 14, 01
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    CFlowCore*          pFlowCore       = GetFlowCore();
    EventType           xEventType      = GetRequestEventType();
    bool                bPreamble       = ( xEventType == EventPreProc || xEventType == EventOnFocus );
    bool                bEnterTheField  = !bPreamble;

    if( IsProgressBeforeInterface() ) // 20140321 endgroups executed in onkey/onchar/userbar didn't process properly
        bEnterTheField = false;

    else if( AtSkip_GetFromInterface() )
        bEnterTheField = AtSkip_GetEnterField(); // RHF Dec 30, 2003


    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();

    // set to "1-up to DataOccs" if PathOff, to "2-no refresh" if PathOn
    CSettings*  pSettings = &m_pEngineDriver->m_EngineSettings;
    bool        bPathOff  = pSettings->IsPathOff();

    SetRefreshGroupOccsLimit( bPathOff ? 1 : 2 );
    SetForwardWay( true );              // insures forward way

    // saves the origin-group for later use             // RHF+vc Jun 15, 01
    int         iSymItem;
    int         iSymGroupSource = pFlowCore->GetAtomGroupInfo( pAtom, &iSymItem );

    // RHF COM Dec 31, 2003 if( bEnterTheField ) {
    if( bEnterTheField || AtSkip_GetFromInterface() ) { // RHF Dec 31, 2003
        if( bPathOff && AtSkip_GetFromInterface() && xAtomType == CFlowAtom::AtomType::Item )
            m_bMidColor = true; // RHF Dec 31, 2003

        // run "after interface" event-procs for the origin-atom
        if( xAtomType == CFlowAtom::AtomType::Item )
            // (a) run "after interface" event-procs for this item
            bRequestIssued = RunEventProcs( 2, bEnterTheField );
        else
            // (b) other atom - run all event-procs
            bRequestIssued = RunEventProcs();

        // RHF INIC Dec 31, 2003
        //if( AtSkip_GetFromInterface() && bPathOff ) {
            //SetFieldColorMid( pAtom );
            m_bMidColor = false;
        //}
        // RHF END Dec 31, 2003

        if( bRequestIssued )
            return bRequestIssued;
    }

    // (1) travels up to the group-tail of the source group
    bool    bGroupTailFound = false;

    while( !bGroupTailFound && !bBreakingEvents && LocateNextAtom() ) {
        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();

        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            // force low color
            SetFieldColorLow( pAtom );
            // remark - none of skipped fields will call AcceptFieldValue
        }
        else {
            // (b) other atom - run all event-procs
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                    if( xAtomType == CFlowAtom::AtomType::GroupTail )
                        bGroupTailFound = ( pFlowCore->GetAtomGroupSymbol( pAtom ) == iSymGroupSource );

                    if( bGroupTailFound ) {
                        GroupTrimOcc(); // takes care of empty occurrences
                    }

                    /* RHF COM Feb 10, 2003 else */{
                        bRequestIssued = RunEventProcs();
                        if( bRequestIssued )
                            bBreakingEvents = IsRequestBreakingEventsChain();
                        else if( WasLevelTailReached() )
                            bBreakingEvents = true;
                    }                                   // victor Jun 21, 01
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::BeyondStrip:
                    break;

                default:
                    ASSERT( 0 );        // can't be
                    break;
            }
        }
    }
    // reset occs-limit                                 // RHF+vc Jun 15, 01
    ResetRefreshGroupOccsLimit();

    // (2) the group-tail was reached without interference: asks for next field
    if( bGroupTailFound && !bBreakingEvents ) {
        bRequestIssued = AskForNextField(); // "internal" NextField request, "after interface"
        SetProgressBitmapOffUpTo( EventNone ); // RHF Feb 10, 2003
    }

    return bRequestIssued;
}

bool CsDriver::SolveLogicEndLevel( void ) {
    // moves to GroupTail of this Level (then one more atom if the request was issued from the Level GroupHead)
    bool        bRequestIssued  = false;
    CFlowCore*  pFlowCore       = GetFlowCore();
    bool        bIsModification = m_pEntryDriver->IsModification();
    CSettings*  pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool        bPathOff        = pSettings->IsPathOff();

    // remark - this reproduces lots of stuff formerly at DeFuncs/deendlevel
    CFlowAtom*  pAtom           = GetRequestOrigin();

    // RHF INIC Dec 31, 2003
    bool        bEnterTheField =  AtSkip_GetFromInterface() && AtSkip_GetEnterField();

    if( AtSkip_GetFromInterface() ) {
        m_bKillFocus = true;
        m_bMidColor = bPathOff && pAtom->GetAtomType() == CFlowAtom::AtomType::Item;
        bRequestIssued = SolveNextField( bEnterTheField );
        m_bMidColor = false;
        //if( bPathOff )
        //    SetFieldColorMid( pAtom );

        if( bRequestIssued ) return bRequestIssued;
    }
    // RHF END Dec 31, 2003

    int         iEndingLevel    = pFlowCore->GetAtomLevel( pAtom );
    bool        bFromLevelHead  = pFlowCore->IsLevelHead( pAtom );
    bool        bFromLevelTail  = pFlowCore->IsLevelTail( pAtom );
    bool        bIgnoreWrite    = bFromLevelHead;

    if( !bPathOff )
        TrimAllOcc( iEndingLevel ); // RHF END Oct 01, 2004

    if( true || bFromLevelHead || bFromLevelTail )
    {
        m_pEntryDriver->TakeCareOfEndingNode( iEndingLevel, bIgnoreWrite );

        if( iEndingLevel > 0 )
            m_pEntryDriver->ClearUnprocessedLevels(iEndingLevel);
    }

    if( iEndingLevel < 1 || bIgnoreWrite ) {
        // the level must be cleaned because WriteEndingNode will not be executed
        // ... clean data areas and ALL occurrences
        LevelCleanData( iEndingLevel );  // ... equivalent to old 'DeInitLevel'
        // ... set the lights off
        LevelCleanColors( iEndingLevel );// ... equivalent to old 'delow_level'
    }

    // set to "1-up to DataOccs" if PathOff, to "2-no refresh" if PathOn
    SetRefreshGroupOccsLimit( bPathOff ? 1 : 2 );
    SetForwardWay( true );              // insures forward way
    if( iEndingLevel <= 1 )             // ... makes sure the target-node is void
        SetTargetNode( CEntryDriver::NodeAdvanceNone );
    // setup a next atom depending on where the LogicEndLevel was issued:
    // ... issued at LevelHead or inside the level: to the tail of this level
    // ... issued at LevelTail: to the tail of parent level
    int     iTailAtom = GetFlowCore()->GetLevelTailIndex( iEndingLevel );
    int     iNextAtom = iTailAtom + ( bFromLevelHead || bFromLevelTail );

    SetCurrAtomIndex( iNextAtom );
    SetLevelTailReached();
    if( iEndingLevel < 1 && bFromLevelHead )
        SetDoorCondition( CsDriver::Closed );      // victor Mar 04, 02

    SetProgressBitmapOn();

    ResetRefreshGroupOccsLimit();

    // at the landing atom, set a NextField request
    SetInterRequestNature( CsDriver::NextField );
    SetRequestOrigin();
    SetRequestEventType( CsDriver::EventNone );

    bRequestIssued = true;

    return bRequestIssued;
}


bool CsDriver::SolveNextField( bool bEnterTheField ) {
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    CFlowCore*          pFlowCore       = GetFlowCore(); // victor Feb 22, 02

// RHF COM Dec 09, 2003SolveNextField:
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    if( xAtomType == CFlowAtom::AtomType::BeyondStrip )
        return bRequestIssued;
    // ... now checking for IdCollision         <begin> // victor Mar 12, 02
    int     iLevel = pFlowCore->GetAtomLevel( pAtom );
    if( xAtomType == CFlowAtom::AtomType::Item && iLevel == 1 ) {

        int     iCheckPoint = GetFlow()->GetIdCheckPoint( iLevel );
        int     iSymVar     = ((CFlowItem*) pAtom)->GetSymbol();
        int     iSymGroup   = VPT(iSymVar)->GetOwnerGroup();
        int     iFlowOrder;

        GPT(iSymGroup)->GetItemIndex( iSymVar, &iFlowOrder );

        if( iFlowOrder == iCheckPoint && SomeIdCollision() ) {
               SetRequestNature( Reenter );
               SetReenterTarget(VPT(iSymVar)); // RHF Jan 15, 2003
               bRequestIssued = true;
               return bRequestIssued;
        }
    }
    // ... now checking for IdCollision         <end>   // victor Mar 12, 02

    SetForwardWay( true );              // insures forward way

    // run "after interface" event-procs for the origin-atom
    if( xAtomType == CFlowAtom::AtomType::Item ) {
        // Refactoring, RCL Sept 2004
        // {
        CNDIndexes  theIndex;

        VARX* pVarX = GetFieldFromAtom( pAtom, theIndex );
        VART* pVarT = pVarX->GetVarT();
        GROUPT* pGroupT = pVarT->GetOwnerGPT();
        bool bPathOff = m_pEngineDriver->m_pEngineSettings->IsPathOff();

        bool bEndGroup = false;

        bool bIsModification = m_pEntryDriver->IsModification() &&
                               m_pEntryDriver->GetPartialMode() != ADD_MODE;

        ASSERT( pGroupT != 0 );

        checkEndGroup( bIsModification, bPathOff, pGroupT, &bEndGroup );

        // }  Refactoring, RCL Sept 2004

        if( !IsProgressBeforeOnFocus() ) {              // victor Aug 08, 01
            // normal situation: the item was already processed by the Interface and only "after interface" event-procs are to be run
            // run "after interface" event-procs for this item
            if( bEndGroup )
            {
                bRequestIssued = false;
            }
            else
                bRequestIssued = RunEventProcs( 2, bEnterTheField );
        }
        else {                                          // victor Aug 08, 01
            // after an EnterFlow ended, there are pending events before the interface
            // run "up to interface" event-procs for this (resumed) item
            bRequestIssued = RunEventProcs( 1 );
            return bRequestIssued;      // ... immediate return!
        }
    }
    else {
        if( xAtomType == CFlowAtom::AtomType::GroupTail && !pFlowCore->IsLevelTail( pAtom ) ) // victor Feb 23, 02
            GroupTrimOcc();             // takes care of empty occurrences
        if( xAtomType != CFlowAtom::AtomType::BeforeStrip )
            // run all event-procs for this non-item atom
            bRequestIssued = RunEventProcs();
    }
    if( EnterFlowJustStarted() )        // ... enter-flow gets immediatly back!
        return false;

    if( bRequestIssued )
        bBreakingEvents = IsRequestBreakingEventsChain();
    else if( WasLevelTailReached() )                    // victor Jun 07, 01
        bBreakingEvents = true;                         // victor Jun 07, 01

    if( !bBreakingEvents ) {
        // looks for the next item-atom in order to give it to the Interface
        if( !m_pIntDriver->m_bStopProc ) {
            bRequestIssued = ReachNextField();
        }
        else {
            bRequestIssued = false;
        }
        // restarts when noinput requested              // victor Dec 10, 01
        if( NoInputRequested() ) { //Noinput only in preproc/onfocus. Not in function
            bRequestIssued = AskForNextField(true); // "internal" NextField request, "after interface"// RHF Jul 03, 2003

            /* RHF COM INIC Jul 03, 2003
            ASSERT( !bRequestIssued );
            bEnterTheField = true;
            goto SolveNextField;
               RHF COM END Jul 03, 2003 */
        }
    }

    return bRequestIssued;
}


bool CsDriver::SolvePrevField( bool bStopAtReenterTarget, bool bIgnoreProtected )
{
    // SolvePrevField: any request issued before reaching an item becomes a return
    // ... parm "stop at reenter target" added          // victor Dec 10, 01
    bool                bRequestIssued = false;
    bool                bIsPrimaryFlow = ( GetFlow()->GetSubType() == SymbolSubType::Primary );
    CFlowAtom*          pAtomLimit = ( bIsPrimaryFlow ) ? GetFlowCore()->GetLowestFieldAtom() : NULL;
    CFlowAtom*          pAtom      = GetCurrAtom();

    // We need current atom [current item] when analyzing GroupHead later
    // rcl, May 2005
    CFlowItem*  pFlowCurrentItem = NULL;

    if( pAtom->GetAtomType() == CFlowAtom::AtomType::Item )
        pFlowCurrentItem = (CFlowItem*) pAtom;

    CFlowAtom::AtomType xAtomType  = pAtom->GetAtomType();
    CSettings*          pSettings  = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff   = pSettings->IsPathOff();
    bool                bBefore    = ( xAtomType == CFlowAtom::AtomType::BeforeStrip );

    bool                bCanGoBack = ( pAtom != pAtomLimit );
    CFlAdmin*           pFlAdmin;
    int                 iSymFlow;

    // ... new full check of "GroupHead targets"        // victor Mar 25, 02
    int                 iSymTarget = ( !m_TgtReenter.IsEmpty() ) ? m_TgtReenter.GetSymbol() : 0;
    bool                bGroupHeadTarget = ( iSymTarget > 0 ) ? m_pEngineArea->IsSymbolTypeGR( iSymTarget ) : false;
    bool                bBlockHeadTarget = ( iSymTarget > 0 ) ? NPT(iSymTarget)->IsA(SymbolType::Block) : false;
    bool                bRebound;
    C3DObject           o3DObserved;
    CFlowGroupHead*     pGroupHead;
    CFlowItem*          pFlowItem = 0;

    bool bSpecialCase = false;
    // Current behaviour prevents getting the previous atom
    // when the current atom is the first field, but we need to get
    // the previous one when the target is a group [form/roster]
    // so we will open the door
    // JOC, May 2005
    if( GetRequestNature() == CsDriver::Reenter && bCanGoBack == false &&
        NPT(m_TgtReenter.GetSymbol())->IsOneOf(SymbolType::Group, SymbolType::Block) )
    {
        bSpecialCase = true;
        bCanGoBack = true;
    }

    if( !bBefore && bCanGoBack ) {                      // victor Jun 14, 01
        int         iFieldColor;
        bool        bHasColor;
        CNDIndexes  theIndex;
        VARX*       pVarX;

        SetForwardWay( false );         // insures backward way

        if( !IsProgressBeforeOnFocus() ) {              // victor Aug 08, 01
            // run "after interface" event-procs for the origin-atom
            if( xAtomType == CFlowAtom::AtomType::Item ) {
                      // run "after interface" event-procs for this item (without EnterTheField)
                      bRequestIssued = RunEventProcs( 2 );

                      // reset the light
                      if( bPathOff )
                          SetFieldColorLow( pAtom );
                      else
                          SetFieldColorNone( pAtom );
                  }
                  else if( xAtomType != CFlowAtom::AtomType::BeyondStrip ) {
                      // other atom - run all event-procs
                      bRequestIssued = RunEventProcs();
                  }

        }
    // any request issued before reaching an item becomes a return
    bool        bFound = false;

    CFlowAtom* pLastAtom = GetCurrAtom();// RHF Mar 23, 2003

    while( !bRequestIssued && !bFound && LocatePrevAtom(bSpecialCase) ) {
        pAtom = GetCurrAtom();

        // RHF INIC Mar 23, 2003
        if( pAtom == pLastAtom ) {
            SetForwardWay( true ); // insures forward way
            break;
        }
        pLastAtom = pAtom;
        // RHF END Mar 23, 2003

        xAtomType = pAtom->GetAtomType();

        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item )
        {
            iFieldColor = GetFieldColor( pAtom );
            bHasColor   = ( iFieldColor == FLAG_HIGHLIGHT || bPathOff && iFieldColor == FLAG_MIDLIGHT );
            pVarX = GetFieldFromAtom( pAtom, theIndex );

            // 20101107 makes it so that going back (in operator controlled mode) always goes to the previous field,
            // which it sometimes wouldn't when pages (forms) were skipped
            if( bPathOff && !bHasColor )
            {
                // if we're here, then this field is on a group that was skipped and should have no data in its fields

                // this check ensures that when the previous field is on a roster, that we move to the first occurrence of
                // the roster, and we'll move to the first column as well
                if( theIndex.getIndexValue(0) == 0 )
                {
                    VART * pVarT = pVarX->GetVarT();
                    GROUPT * pGroupT = pVarT->GetOwnerGPT();
                    CDEGroup* pGroup = pGroupT->GetCDEGroup();

                    if( pGroup->GetItemType() != CDEFormBase::Roster )
                        bHasColor = true; // these are cases when the group is just a form

                    else if( pGroup->GetItemIndex(WS2CS(pVarT->GetName())) == 0 ) // this is a roster so only move to the first column
                        bHasColor = true;
                }

                // this workaround will not work if the previous field is protected; as the below code will move the cursor
                // back so as not to remain on the protected field, but i think it's good enough as is
            }

            if( bHasColor ) // RHF Feb 11, 2003
                // run "up to interface" event-procs for the new item
                bRequestIssued = RunEventProcs( 1 );

            // found... once the field has a color
            bFound = bHasColor;

            // reset the light
            if( bPathOff )
                SetFieldColorLow( pAtom );
            else
                SetFieldColorNone( pAtom );

            // take care of protected fields
            /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP END*/

            // Refactoring, RCL Sept 2004
            // {
            VART* pVarT = pVarX->GetVarT();
            GROUPT* pGroupT = pVarT->GetOwnerGPT();

            bool bEndGroup = false;

            bool bIsModification = m_pEntryDriver->IsModification() &&
                                   m_pEntryDriver->GetPartialMode() != ADD_MODE;

            ASSERT( pGroupT != 0 );

            checkEndGroup( bIsModification, bPathOff, pGroupT, &bEndGroup );

            // }  Refactoring, RCL Sept 2004

            // RHF INIC Mar 23, 2003. Solve problem enter from a postproc of group, back-tab from enter, and the last field is protected!
            if( bIgnoreProtected && bFound && pVarT->IsProtectedOrNoNeedVerif()) {
                bFound = false;
            }
            // RHF END Mar 23, 2003

            /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP END*/
            if( !bRequestIssued &&(pVarX->GetVarT()->IsProtectedOrNoNeedVerif() || bEndGroup) ) {
                bRebound = false;

                // ... is 1st field                 // RHF Jun 19, 2001
                if( pAtom == pAtomLimit )
                    bRebound = true;
                // ... checks for "reenter target" if requested
                else if( bStopAtReenterTarget ) {
                    pFlowItem = (CFlowItem*) pAtom;

                    o3DObserved.SetSymbol( pFlowItem->GetSymbol() );
                    o3DObserved.setIndexes( pFlowItem->GetIndex() );
                    if( o3DObserved == m_TgtReenter )
                        bRebound = true;
                }
                if( bRebound ) {
                    SetForwardWay( true ); // insures forward way
                    break;          // ... immediate return
                }
            }
        }


        // (b) block processing
        else if( xAtomType == CFlowAtom::AtomType::BlockHead || xAtomType == CFlowAtom::AtomType::BlockTail )
        {
            bRebound = false;

            // event procs have to be run if we've reached the block to be reentered
            if( xAtomType == CFlowAtom::AtomType::BlockHead )
            {
                if( bStopAtReenterTarget && bBlockHeadTarget )
                {
                    CFlowBlockHead* pFlowBlockHead = (CFlowBlockHead*)pAtom;

                    if( pFlowBlockHead->GetSymbol() == m_TgtReenter.GetSymbol() )
                    {
                        o3DObserved.SetSymbol(pFlowBlockHead->GetSymbol());
                        o3DObserved.setIndexes(pFlowBlockHead->GetIndex());
                        bRebound = SameBranch(&o3DObserved, &m_TgtReenter);
                    }
                }
            }

            bool run_event_procs = ( bRebound || GetRequestNature() != RequestNature::PrevField );

            if( !run_event_procs )
            {
                C3DObject* p3DObject = ExtractInfoFromAtom(pAtom);
                const EngineBlock& engine_block = GetSymbolEngineBlock(p3DObject->GetSymbol());
                CNDIndexes* zero_based_index = nullptr;

                // because there is no color for blocks, in a PrevField request, we will see if there are
                // any fields in the block, and if so, we will run the procs only if a field has color; this is
                // an attempt to resolve the problem whereby the onfocus/killfocus events could be executed for
                // blocks that have been skipped over
                for( const CDEField* field : engine_block.GetFields() )
                {
                    VARX* pVarX = VPX(field->GetSymbol());

                    if( zero_based_index == nullptr )
                        zero_based_index = new CNDIndexes(pVarX->PassIndexFrom3DToEngine(p3DObject->GetIndexes()));

                    int field_color = m_pIntDriver->GetFieldColor(pVarX, *zero_based_index);

                    if( field_color == FLAG_HIGHLIGHT || ( bPathOff && field_color == FLAG_MIDLIGHT ) )
                    {
                        run_event_procs = true;
                    }

                    // they also have to be run if the current field is on this block
                    else if( pFlowCurrentItem != nullptr && pFlowCurrentItem->GetSymbol() == field->GetSymbol() &&
                             p3DObject->GetIndexes() == pFlowCurrentItem->GetIndex() )
                    {
                        run_event_procs = true;
                    }

                    if( run_event_procs )
                        break;
                }

                delete zero_based_index;
            }

            if( run_event_procs )
                bRequestIssued = RunEventProcs();

            // if reaching the block to be reentered, set the forward way
            if( !bRequestIssued && bRebound )
            {
                SetForwardWay(true);
                bFound = true;
            }
        }

        // (c) other atom - run all event-procs
        else {
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    // ... new full check of "GroupHead targets" // victor Mar 25, 02
                    bRebound = false;

                    if( bStopAtReenterTarget && bGroupHeadTarget ) {
                        pGroupHead  = (CFlowGroupHead*) pAtom;

                        // The symbol will be enough to check if
                        // we have found the target. Will not use
                        // the specified indexes anymore
                        // rcl, May 2005
                        if( pGroupHead->GetSymbol() == m_TgtReenter.GetSymbol() )
                            bRebound = true;
//                            o3DObserved.SetSymbol( pGroupHead->GetSymbol() );
//                            o3DObserved.setIndexes( pGroupHead->GetIndexFwd() );
//                            if( o3DObserved == m_TgtReenter )
//                                bRebound = true;
                    }
                    if( bRebound ) {
                        SetForwardWay( true ); // insures forward way
                        bFound = true;
                        break;      // ... immediate return
                    }

                    // We need to find out if current grouphead is an ancestor of
                    // current item because EventProcs will be executed only for ancestors
                    // rcl+JOC, May 2005
                    if( pFlowCurrentItem != 0 &&
                        !VPT( pFlowCurrentItem->GetSymbol() )->IsAncestor( ((CFlowGroupHead*) pAtom)->GetSymbol(), false ) )
                    {
                        // if it is not ancestor -> do not execute RunEventProcs
                    }
                    else
                        bRequestIssued = RunEventProcs();
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    {
                        // RHF INIC Feb 11, 2003
                        bool bTargetGroup = false;
                        ASSERT( xAtomType == CFlowAtom::AtomType::GroupTail );
                        pGroupHead  = (CFlowGroupHead*) pAtom;

                        ASSERT( pGroupHead->GetSymbol() > 0 );
                        GROUPT* pGroupSource = GPT(pGroupHead->GetSymbol());
                        int     iMaxOcc      = pGroupSource->GetTotalOccurrences();

                        if( iMaxOcc >= 1 )
                            bTargetGroup = true;
                        // RHF END Feb 11, 2003
                        if( bTargetGroup ) // RHF Feb 11, 2003
                            bRequestIssued = RunEventProcs();
                    }
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    bRequestIssued = RunEventProcs();
                    break;

                case CFlowAtom::AtomType::BeforeStrip: // victor Aug 02, 01
                    // disable current flow (ending an EnterFlow)
                    SetLogicRequestNature( EnterFlowEnd );
                    SetSymTarget();
                    bRequestIssued = true;

                    pFlAdmin = GetFlAdmin();
                    iSymFlow = GetFlow()->GetSymbol();

                    pFlAdmin->DisableFlow( iSymFlow, false );
                    break;

                default:
                    ASSERT( 0 );    // can't be
                    break;
            }
        }
    }
}

    return bRequestIssued;
}

bool CsDriver::SolveInterEndOccur( void ) {
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    bool                bEnterTheField  = AtEndOccur_GetEnterField();
    CFlowCore*          pFlowCore       = GetFlowCore();
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    // remark - this interface' request must be issued starting at a field
    ASSERT( xAtomType == CFlowAtom::AtomType::Item );
    if( !( xAtomType == CFlowAtom::AtomType::Item ) )
        return bRequestIssued;          // don't change the current-atom

    SetForwardWay( true );              // insures forward way

    if( !bEnterTheField )
        SetFieldColorMid( pAtom );
    else {
        // run "after interface" event-procs for the origin-atom
        bRequestIssued = RunEventProcs( 2, bEnterTheField );

        if( bRequestIssued )
            bBreakingEvents = IsRequestBreakingEventsChain();
    }

    // (1) travels up to the nearest HTOcc
    int         iSymGroupSource = pFlowCore->GetAtomGroupSymbol( pAtom );
    bool        bHTOccFound     = false;

    while( !bHTOccFound && !bBreakingEvents && LocateNextAtom() ) {
        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();

        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            // force mid color for any field in the trip
            SetFieldColorMid( pAtom );
        }
        else {
            // (b) other atom - just hit the nearest HTOcc
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                    ASSERT( 0 );        // can't be
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    bHTOccFound = true;
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::BeyondStrip:
                    break;

                default:
                    ASSERT( 0 );        // can't be
                    break;
            }
        }
    }

    // (2) HTOcc reached: asks for next field
    if( !bBreakingEvents )
        bRequestIssued = AskForNextField(); // "internal" NextField request, "after interface"

    return bRequestIssued;
}

bool CsDriver::SolveInterEndGroup( void ) {
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    bool                bEnterTheField  = AtEndGroup_GetEnterField();
    CFlowCore*          pFlowCore       = GetFlowCore();
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    // remark - this interface' request must be issued starting at a field
    ASSERT( xAtomType == CFlowAtom::AtomType::Item );
    if( !( xAtomType == CFlowAtom::AtomType::Item ) )
        return bRequestIssued;          // don't change the current-atom

    int         iSymGroupSource = pFlowCore->GetAtomGroupSymbol( pAtom );
    GROUPT*     pGroupSource    = GPT(iSymGroupSource);
    int         iMaxOcc         = pGroupSource->GetDataOccurrences();

    SetForwardWay( true );              // insures forward way

    if( !bEnterTheField )
        SetFieldColorMid( pAtom );
    else {
        // run "after interface" event-procs for the origin-atom
        bRequestIssued = RunEventProcs( 2, bEnterTheField );

        if( bRequestIssued )
            bBreakingEvents = IsRequestBreakingEventsChain();
    }

    // (1) travels up to the group-tail of the source group
    bool        bGroupTailFound = false;

    while( !bGroupTailFound && !bBreakingEvents && LocateNextAtom() ) {
        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();

        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            // force mid color for any field in the trip
            SetFieldColorMid( pAtom );
        }
        else {
            // (b) other atom - run event-procs for the group being ended
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                    if( xAtomType == CFlowAtom::AtomType::GroupTail )
                        bGroupTailFound = ( pFlowCore->GetAtomGroupSymbol( pAtom ) == iSymGroupSource );

                    if( bGroupTailFound )
                        GroupTrimOcc(); // takes care of empty occurrences
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::BeyondStrip:
                    break;

                default:
                    ASSERT( 0 );        // can't be
                    break;
            }
        }
    }
    // (2) the group-tail was reached without interference: asks for next field
    if( bGroupTailFound && !bBreakingEvents )
        bRequestIssued = AskForNextField(); // "internal" NextField request, "after interface"

    return bRequestIssued;
}

bool CsDriver::SolveInterEndLevel( void ) {
    // remark - final implementation based on built-in Advance/Skip requests // victor Mar 07, 02
    bool                bRequestIssued  = false;
    bool                bBreakingEvents = false;
    bool                bEnterTheField  = AtEndLevel_GetEnterField();
    int                 iNextLevel      = AtEndLevel_GetNextLevel();
    bool                bWriteNode      = AtEndLevel_GetWriteNode();
    CFlowCore*          pFlowCore       = GetFlowCore();
    CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff        = pSettings->IsPathOff();
    CFlowAtom*          pAtom           = GetCurrAtom();
    CFlowAtom::AtomType xAtomType       = pAtom->GetAtomType();
    ASSERT( xAtomType == CFlowAtom::AtomType::Item );
    int                 iSourceLevel    = pFlowCore->GetAtomLevel( pAtom );
    bool                bChangeLevel    = ( iNextLevel != iSourceLevel );
    bool                bIgnoreWrite    = !bWriteNode;

    m_bSolveInterEndLevel = true; //SAVY For endlevel postproc Aug 01,2003
            if( !( xAtomType == CFlowAtom::AtomType::Item ) )
                return bRequestIssued;          // don't change the current-atom

            SetForwardWay( true );              // insures forward way

            if( !bWriteNode )
                m_pEntryDriver->TakeCareOfEndingNode( iSourceLevel, bIgnoreWrite );

            else
            {
                if( bEnterTheField ) {
                    SetInterRequestNature( CsDriver::AdvanceTo );
                    SetRequestOrigin();

                    // run "after interface" event-procs for the origin-atom
                    bRequestIssued = RunEventProcs( 2, bEnterTheField );

                    if( bRequestIssued )
                        return bRequestIssued;
                }

                if( !bRequestIssued ) {
                    C3DObject   o3DTarget;
                    CNDIndexes  theIndex( ZERO_BASED, DEFAULT_INDEXES );

                    int         iSymLevel = GetSymbolOfLevel( iSourceLevel );

                    o3DTarget.SetSymbol( iSymLevel );
                    o3DTarget.setIndexes( theIndex );

                    if( bPathOff )
                        SetInterRequestNature( CsDriver::SkipTo );
                    else
                        SetInterRequestNature( CsDriver::AdvanceTo );
            Set3DTarget( &o3DTarget );
            SetNodeBoundTarget( iSymLevel ); // asking for a node-bound of the source level

            bRequestIssued = SolveSkipOrAdvanceTo();

#ifdef _DEBUG
            CFlowAtom*  pCurrAtom = GetCurrAtom(); //debug support only
            bool        bLevelTailReached = WasLevelTailReached();
            ASSERT( bRequestIssued || bLevelTailReached );
#endif
            SetNodeBoundTarget();

            if( bRequestIssued )
                bBreakingEvents = IsRequestBreakingEventsChain();

            if( !bBreakingEvents && bChangeLevel )
                m_pEntryDriver->TakeCareOfEndingNode( iSourceLevel, bIgnoreWrite );
        }
    }
    SetLevelTailReached();

#ifdef _DEBUG
    CFlowAtom*  pCurrAtom  = GetCurrAtom(); // debug support only
    int         iCurrAtom  = GetCurrAtomIndex(); // save the current-atom
    int         iLevelHead = pFlowCore->GetLevelHeadIndex( iNextLevel );
#endif
    int         iLevelTail = pFlowCore->GetLevelTailIndex( iNextLevel );
    int         iNextAtom  = iLevelTail;

    if( !bBreakingEvents ) {
        SetCurrAtomIndex( iNextAtom );

        bRequestIssued = AskForNextField( false ); // "internal" NextField request, "before interface"
       // bRequestIssued = AskForNextField( bPathOff ); // RHF Jul 28, 2003 Fix F12 Bug in Path Off
    }

    return bRequestIssued;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- basic support to solve requests
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::AskForNextField( bool bEnableAfterInterface ) { // victor Jun 21, 01
    // AskForNextField: set an "internal" NextField request
    DontRemakeOrigin();                 // "internal" request must not change the origin
    // emulates a NextField' request
    SetRequestSource( false );          // from Interface
    SetRequestNature( CsDriver::NextField );

    // set the progress bitmap as required
    SetProgressBitmapOn();              // set all enabled
    if( bEnableAfterInterface )
        SetProgressBitmapOffUpTo( EventInterface );
    else {
            SetProgressBitmapOff( EventRefresh );
            SetProgressBitmapOff( EventKillFocus );
            SetProgressBitmapOff( EventPostProc );
            SetProgressBitmapOff( EventOnOccChange );
    }
    return true;
}

bool CsDriver::ReachNextField( void ) {
    // ReachNextField: looks for the next item-atom in order to give it to the Interface
    bool                bRequestIssued = false;
    bool                bBreakingEventsChain = false;
    CFlowCore*          pFlowCore      = GetFlowCore();
    CFlowAtom*          pAtom;
    CFlowAtom::AtomType xAtomType;
    CSettings* pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool       bPathOff        = pSettings->IsPathOff();
    // RHF INIC Sep 25, 2003
    bool                bSkipTo    = ( GetRequestNature() == CsDriver::SkipTo );
    C3DObject           o3DTarget  = ( bSkipTo ) ? m_TgtSkipTo : m_TgtAdvanceTo;
    C3DObject*          p3DTarget  = &o3DTarget;
    // RHF END Sep 25, 2003

    CString csMsg;
    int iLoopCount = 0;

    RTRACE( _T("ReachNextField::Begin") NL );
    RTRACE( _T("{") NL );

    while( !bBreakingEventsChain && LocateNextAtom() ) {
        iLoopCount++;
        csMsg.Format( INDENT _T("%d.- Looping, (bBreakingEventsChain == false && LocateNextAtom() != 0)") NL, iLoopCount );
        RTRACE( csMsg );

        pAtom = GetCurrAtom();
        xAtomType = pAtom->GetAtomType();


        // (a) is an item-atom
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            RTRACE( INDENT _T("xAtomType == CFlowAtom::AtomType::Item") NL );
            RTRACE( INDENT _T("{") NL );

            // force low color
            SetFieldColorLow( pAtom );

            // run "up to interface" event-procs for the new item

            bRequestIssued = RunEventProcs( 1 );

            // LevCt: controlling levels operation <begin>      // victor Mar 15, 02
            CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
            bool                bPathOff        = pSettings->IsPathOff();
            int     iSymVar = ((CFlowItem*)pAtom)->GetSymbol();
            VART*   pVarT = VPT(iSymVar);
            int     iLevel = GetFlowCore()->GetAtomLevel( pAtom );
            int     iFieldColor = GetFieldColor( pAtom );
            ASSERT( iLevel > 0 );
            bool    bEnteredField = ( bPathOff  && iFieldColor != FLAG_NOLIGHT ) ||
                            ( !bPathOff && iFieldColor == FLAG_HIGHLIGHT );


            /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/
            // Refactoring, RCL Sept 2004
            // {
            GROUPT* pGroupT = pVarT->GetOwnerGPT();

            bool bEndGroup = false;
            bool bEndLevelOcc = false;

            bool bIsModification = m_pEntryDriver->IsModification() &&
                                   m_pEntryDriver->GetPartialMode()!= ADD_MODE;

            ASSERT( pGroupT != 0 );

            checkEndGroupEndLevel( bIsModification, bPathOff,
                pGroupT, &bEndGroup, &bEndLevelOcc );

            // } Refactoring, RCL Sept 2004

            /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/
            if( pVarT->IsPersistentOrProtected() || pVarT->IsProtectedOrNoNeedVerif() || bEndGroup || bEndLevelOcc)
                m_pEntryDriver->LevCtSetInfo( iLevel, CEntryDriver::NoInfo );
            else if( bEnteredField )            // "entered"...
                m_pEntryDriver->LevCtSetInfo( iLevel, CEntryDriver::HasInfo );
            // LevCt: controlling levels operation <end>        // victor Mar 15, 02

            RTRACE( INDENT _T("}") NL );
            break;                      // ... immediate return (break the cycle)
        }
        // (b) is another atom
        else {
            RTRACE( INDENT _T("No es item -> else") NL );
            RTRACE( INDENT _T("{") NL );
            // trim occs of non-level' GroupTails       // victor Feb 23, 02
            if( xAtomType == CFlowAtom::AtomType::GroupTail && !pFlowCore->IsLevelTail( pAtom ) )
                GroupTrimOcc();         // takes care of empty occurrences

            // check if a node-bound was reached        // victor Mar 07, 02
            if( IsNodeBoundTarget() ) {
                int     iSymLevel = GetNodeBoundTarget();
                bool    bNodeBoundFound = false;

                if( pFlowCore->IsLevelHead( pAtom ) )
                    bNodeBoundFound = ( ((CFlowGroupHead*) pAtom)->GetSymbol() != iSymLevel );
                else if(pFlowCore->IsLevelTail( pAtom ) )
                     bNodeBoundFound = true;

                if( bNodeBoundFound ) {
                    // ... emulates "level-tail reached" for the level of the node-bound
                    SetLevelTailReached( GPT(iSymLevel)->GetLevel() );
                    break;              // ... immediate return (break the cycle)
                }
            }

            RTRACE( INDENT _T("}") NL );
        }
        // (c) other atom - run all event-procs
        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
            case CFlowAtom::AtomType::GroupTail:
            case CFlowAtom::AtomType::HTOcc:
            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
                bRequestIssued = RunEventProcs();
                csMsg.Format( INDENT _T("RunEventProcs() -> %s") NL, BOOL2STRING(bRequestIssued) );
                RTRACE( csMsg );
                if( bRequestIssued )
                {
                    bBreakingEventsChain = IsRequestBreakingEventsChain();
                    csMsg.Format( INDENT2 _T("IsRequestBreakingEventsChain() -> %s") NL, BOOL2STRING(bBreakingEventsChain) );
                    RTRACE( csMsg );
                }
                else if( WasLevelTailReached() )
                {
                    bBreakingEventsChain = true;
                    RTRACE( INDENT2 _T("WasLevelTailReached() -> true, bBreakingEventsChain = true") NL );
                }
                // RHF INIC Sep 25, 2003
                else if( bSkipTo ) {    // check if target was reached
                    RTRACE( INDENT2 _T("WasLevelTailReached() -> false, bSkipTo = true") NL );
                    RTRACE( INDENT2 _T("{") NL );
                    //Savy - using optimize target compuatation during skip to.
                    //for each iteration, target search is reevaluated causing slow down in long skips.
                    //added new flag to reuse computed target atom index. Used only for skips
                    bool    bTargetReached = ( SearchTargetLocation( p3DTarget,-1,0, bSkipTo ) <= 0 );

                    if( bTargetReached ) {
                        RTRACE( INDENT3 _T("( SearchTargetLocation( p3DTarget ) <= 0 ), bTargetReached = true") NL );
                        bBreakingEventsChain = true;
                        bRequestIssued = SetInterRequestNature( CsDriver::NextField );
                        RTRACE( INDENT3 _T("bBreakingEventsChain = true") NL );
                        csMsg.Format( INDENT3 _T("SetInterRequestNature( RequestNature::NextField ) == %s, bRequestIssued = %s") NL,
                                        BOOL2STRING(bRequestIssued), BOOL2STRING(bRequestIssued));
                        RTRACE( csMsg );
                    }
                    else
                    {
                        RTRACE( INDENT3 _T("( SearchTargetLocation( p3DTarget ) > 0 ), bTargetReached = false") NL );
                    }
                    RTRACE( INDENT2 _T("}") NL );
                }
                // RHF END Sep 25, 2003
                break;

            default:
                ASSERT( 0 );            // can't be
                break;
        }

        // RHF INIC Feb 10, 2003 Fix Enter Bug
        if( EnterFlowJustStarted() )
        {
            RTRACE( INDENT _T("EnterFlowJustStarted() == true, bBreakingEventsChain = true") NL );
            bBreakingEventsChain = true;
        }
        // RHF END Feb 10, 2003
    }
    csMsg.Format( _T("} -> returns %s") NL, BOOL2STRING(bRequestIssued) );
    RTRACE( csMsg );
    return bRequestIssued;
}

void CsDriver::ReachNextNodeAtLevel( int iTargetLevel ) {
    // ... check if this is needed (when/how/which source)
}

bool CsDriver::LocateNextAtom( void ) {
    CFlowCore*          pFlowCore   = GetFlowCore();
    CFlowAtom*          pAtom       = GetCurrAtom();
    CFlowAtom::AtomType xAtomType   = pAtom->GetAtomType();
    int                 iLevelTail  = -1;// means "no level-tail was reached"
    bool                bCanGoAhead = ( xAtomType != CFlowAtom::AtomType::BeyondStrip );

    if( bCanGoAhead ) {
        pAtom = GetNextAtom();
        xAtomType = pAtom->GetAtomType();

        // check if the upper-bound of the flow-strip was reached (but keeps the atom)
        bCanGoAhead = ( xAtomType != CFlowAtom::AtomType::BeyondStrip );

        if( bCanGoAhead ) {
            SetCurObject( ExtractInfoFromAtom( pAtom ) );
            SetProgressBitmapOn();                      // victor May 28, 01

            // setup "level tail reached" or not        // victor Jun 07, 01
            iLevelTail = ( pFlowCore->IsLevelTail( pAtom ) ) ? pFlowCore->GetAtomLevel( pAtom ) : -1;
        }
    }

    ReportToInterface( pAtom, 1 );                      // RHF Mar 05, 2002

    SetLevelTailReached( iLevelTail );  // marks "level tail reached" or not // victor Jun 07, 01

    return bCanGoAhead;
}

// Current behaviour prevents getting the previous atom
// when the current atom is the first field, but we need to get
// the previous one when the target is a group [form/roster]
// so we will open the door when bSpecialCase is true
// JOC, May 2005
bool CsDriver::LocatePrevAtom( bool bSpecialCase ) {
    bool                bIsPrimaryFlow = ( GetFlow()->GetSubType() == SymbolSubType::Primary );
    CFlowAtom*          pAtomLimit = ( bIsPrimaryFlow ) ? GetFlowCore()->GetLowestFieldAtom() : NULL;
    CFlowAtom*          pAtom      = GetCurrAtom();
    bool                bCanGoBack = ( pAtom != pAtomLimit );

    if( bSpecialCase && bCanGoBack == false )
        bCanGoBack = true;

    if( bCanGoBack ) {
        pAtom = GetPrevAtom();
        SetCurObject( ExtractInfoFromAtom( pAtom ) );
        SetProgressBitmapOn();                          // victor May 28, 01
    }

    ReportToInterface( pAtom, -1 );                     // RHF Mar 05, 2002

    SetLevelTailReached();              // marks "level tail NOT reached"    // victor Jun 07, 01

    return bCanGoBack;
}

// RHF INIC Mar 05, 2002
bool CsDriver::ReportToInterface( CFlowAtom* pAtom, int iDirection ) {
    CFlowAtom::AtomType xAtomType      = pAtom->GetAtomType();
    bool                bIsLevelHead   = false;
    bool                bIsLevelTail   = false;
    CFlowCore*          pFlowCore      = GetFlowCore();
    int                 iLevel         = pFlowCore->GetAtomLevel( pAtom );
    int                 iSymbol        = 0;
    CNDIndexes          iIndex( ONE_BASED, DEFAULT_INDEXES );
    // checking a proper atom was given
    switch( xAtomType ) {
        case CFlowAtom::AtomType::GroupHead:
            bIsLevelHead = pFlowCore->IsLevelHead( pAtom );
            iSymbol      = ((CFlowGroupHead*) pAtom)->GetSymbol();
            if( iDirection == 1 )
                iIndex  = ((CFlowGroupHead*) pAtom)->GetIndexFwd();
            else
                iIndex  = ((CFlowGroupHead*) pAtom)->GetIndexBwd();
            break;

        case CFlowAtom::AtomType::GroupTail:
            bIsLevelTail = pFlowCore->IsLevelTail( pAtom );
            iSymbol      = ((CFlowGroupTail*) pAtom)->GetSymbol();
            iIndex  = ((CFlowGroupTail*) pAtom)->GetIndex();
            break;

        case CFlowAtom::AtomType::HTOcc:
            iSymbol      = ((CFlowHTOcc*) pAtom)->GetSymbol();

            if( iDirection == 1 )
                iIndex  = ((CFlowHTOcc*) pAtom)->GetIndexFwd();
            else
                iIndex  = ((CFlowHTOcc*) pAtom)->GetIndexBwd();

            break;

        case CFlowAtom::AtomType::Item:
            iSymbol      = ((CFlowItem*) pAtom)->GetSymbol();
            iIndex = ((CFlowItem*) pAtom)->GetIndex();
            break;
    }

    int iOcc = -1; // For group/htocc

    if( xAtomType == CFlowAtom::AtomType::Item ) {
        // ONLY WORKS WITH 1 DIMENSION!!!
        /*
        for(int i=0; i < DIM_MAXDIM; i++ ) {
            iOcc = max( iOcc, iIndex[i] );
        }
        */

        // RTODO: Check this out again
        // RCL: Parche momentaneo
        iOcc = iIndex.searchNotEmptyValue();

        if( iOcc <= 0 )
            iOcc = 1;
    }
    //

    if( xAtomType == CFlowAtom::AtomType::HTOcc ) {
        CFlowAtom::OccType  xOccType= ((CFlowHTOcc*) pAtom)->GetOccType();

        if( xOccType != CFlowAtom::OccType::Inner )
            return true;
    }
    // RTODO: traspasar 3 dimensiones, no solo iOcc.
    //        y para el caso de group/htocc ... evaluar ...
    bool    bDone = m_pEntryDriver->ReportToInterface( iSymbol, iOcc, iDirection, xAtomType );

    return bDone;
}
// RHF END Mar 05, 2002



/////////////////////////////////////////////////////////////////////////////
//
// --- calling the interpreter
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::RunEventProcs( int iProcsSubset, bool bEnterTheField ) {
    // RunEventProcs: returns true if a request was set by called Execs
    // ... iProcsSubset can restrict the event-procs to be executed as follows
    //         0 no restriction - all event-procs
    //         1 restricted to "up to interface" (PreProc/OnFocus/Interface) only
    //         2 restricted to "after interface" (Refresh/KillFocus/PostProc/OnOccChange) only
    ASSERT( iProcsSubset >= 0 && iProcsSubset <= 2 );
    CFlowCore*          pFlowCore      = GetFlowCore();
    CSettings*          pSettings      = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff       = pSettings->IsPathOff();
    bool                bRequestIssued = false;
    bool                bIsPrimaryFlow  = ( GetFlow()->GetSubType() == SymbolSubType::Primary );// RHF Feb 17, 2003

CheckCurrentAtom:
    CFlowAtom*          pAtom          = GetCurrAtom();
    CFlowAtom::AtomType xAtomType      = pAtom->GetAtomType();
    bool                bIsLevelHead   = false;
    bool                bIsLevelTail   = false;
    int                 iLevel         = pFlowCore->GetAtomLevel( pAtom );
    int                 iSymbol        = 0;


    // checking a proper atom was given
    switch( xAtomType ) {
        case CFlowAtom::AtomType::GroupHead:
            bIsLevelHead = pFlowCore->IsLevelHead( pAtom );
            iSymbol      = ((CFlowGroupHead*) pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::GroupTail:
            bIsLevelTail = pFlowCore->IsLevelTail( pAtom );
            iSymbol      = ((CFlowGroupTail*) pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::HTOcc:
            iSymbol      = ((CFlowHTOcc*) pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::BlockHead:
            iSymbol = ((CFlowBlockHead*)pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::BlockTail:
            iSymbol = ((CFlowBlockTail*)pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::Item:
            iSymbol      = ((CFlowItem*) pAtom)->GetSymbol();
            break;

        case CFlowAtom::AtomType::BeforeStrip:
        case CFlowAtom::AtomType::BeyondStrip:
        default:
            ASSERT( 0 );            // can't be
            iSymbol = 0;
            break;
    }


    if( iSymbol <= 0 )
        return bRequestIssued;

    // in Add mode, don't use again level-0' head if the door is already open
    if( bIsLevelHead && iLevel < 1 && DoorConditionIs( Open ) ) {
        LocateNextAtom();
        goto CheckCurrentAtom;
    }

    // "calling table" definition
    bool    bCallProc[EventOnOccChange + 1];
    int     iEvent;

    // calculate the bitmap to be applied
    byte    cStatic   = GetStaticBitmap();
    byte    cDynamic  = GetDynamicBitmap( bIsLevelHead || bIsLevelTail );
    byte    cProgress = GetProgressBitmap();            // victor May 28, 01
    byte    cBitmap   = ( cStatic & cDynamic & cProgress );

//  AdjustBitmap( &cBitmap, pAtom );    // void events without proc // uncomment once the Procs be properly attached

    // prepare the "calling table" for each event' type
    for( iEvent = 0; iEvent <= EventOnOccChange; iEvent++ )
        bCallProc[iEvent] = false;      // ... reset the table

    // ... gets "up to interface" events (unless "after interface" only)
    if( iProcsSubset != 2 ) {
        iEvent = EventPreProc;
        bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );

        iEvent = EventOnFocus;
        bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );

        // item-atoms are the only having interface-event
        if( xAtomType == CFlowAtom::AtomType::Item ) {
            iEvent = EventInterface;
            bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );
        }
    }

    // ... gets "after interface" events (unless "up to interface" only)
    if( iProcsSubset != 1 )
    {
        iEvent = EventRefresh;

        // item-atoms always have refresh-event
        if( xAtomType == CFlowAtom::AtomType::Item )
            bCallProc[iEvent] = true;

        // groupHead/groupTail-atoms can't have refresh-event
        else if( xAtomType == CFlowAtom::AtomType::HTOcc )
            bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );

        iEvent = EventKillFocus;
        bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );

        if( m_bKillFocus ) {
            bCallProc[iEvent] = true;
            m_bKillFocus = false;
        }

        iEvent = EventPostProc;
        bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );

         //SAVY For endlevel postproc Aug 01,2003
        if(bIsLevelTail && m_bSolveInterEndLevel && bPathOff){
            bCallProc[iEvent] = true;
            m_bSolveInterEndLevel = false;
        }

        // htOcc-atoms are the only having onOccChange-event
        if( xAtomType == CFlowAtom::AtomType::HTOcc ) {
            iEvent = EventOnOccChange;
            bCallProc[iEvent] = GetBitOfEventBitmap( &cBitmap, iEvent );
        }
    }

    // calling the actual execution of each one of applicable event-procs
    bool    bRequestArised = false;
    bool    bBreakingEventsChain = false;
    int     iProcType;
    bool    bMarkProgress;                              // victor Dec 10, 01

    for( iEvent = 0; !bBreakingEventsChain && iEvent <= EventOnOccChange; iEvent++ ) {
        // update progress bitmap                       // victor Dec 10, 01
        switch( iProcsSubset ) {
            case 1:                     // up to interface
                bMarkProgress = ( iEvent <= EventInterface );
                break;
            case 2:                     // after interface
            case 0:                     // all procs
            default:
                bMarkProgress = true;
                break;
        }

        if( bMarkProgress )
            SetProgressBitmapOffUpTo( iEvent );

        if( !bCallProc[iEvent] )
            continue;

        bool    bEventKillOrPost=false;
        switch( iEvent ) {
            case EventPreProc:
                iProcType = PROCTYPE_PRE;

                // opening-level actions
                if( bIsLevelHead ) {
                    LevelPrologue( iLevel );

                    if( iLevel < 1 )                             // victor Feb 20, 02
                        SetDoorCondition( Open ); // victor Feb 20, 02
                }
                break;

            case EventOnFocus:
                iProcType = PROCTYPE_ONFOCUS;
                break;

            case EventInterface:
                // TODO when landing here: should return to the driver???
                iProcType = -1;         // no proc attached
                break;

            case EventRefresh:
                iProcType = -1;         // no proc attached
                if( xAtomType == CFlowAtom::AtomType::Item ) {
                    if( bEnterTheField )
                        bRequestArised = AcceptFieldValue( pAtom );

                    // updating field color
                    // remark - AcceptFieldValue can issue a reenter "to itself" request
                    if( bRequestArised || !IsForwardWay() || !bEnterTheField )
                        SetFieldColorMid( pAtom );
                    else {
                        // RHF INIC Dec 31, 2003
                        if( m_bMidColor ) {
                            SetFieldColorMid( pAtom );
                            m_bMidColor = false;
                        }
                        else
                        // RHF END Dec 31, 2003
                            SetFieldColorHigh( pAtom );

                    }
                }
                break;

            case EventKillFocus:
                iProcType = PROCTYPE_KILLFOCUS;
                bEventKillOrPost = true;
                break;

            case EventPostProc:
                iProcType = PROCTYPE_POST;
                bEventKillOrPost = true;
                break;

            case EventOnOccChange:
                iProcType = PROCTYPE_ONOCCCHANGE;
                break;

            default:
                ASSERT( 0 );            // can't be
                iProcType = -1;         // no proc attached
                break;
        }

        /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/
        bool bEndLevelOcc = false;
        bool bIsModification = m_pEntryDriver->IsModification() && m_pEntryDriver->GetPartialMode() != ADD_MODE;
        CSettings*          pSettings      = &m_pEngineDriver->m_EngineSettings;
        bool                bPathOff       = pSettings->IsPathOff();
        CFlowAtom*          pAtom          = GetCurrAtom();

        /* SAVY ADDED THIS FOR IMPLEMENTING AUTO GROUP/LEVEL END in Pathoff October 23 2002*/
        if( bIsModification && bPathOff && m_pEntryDriver->GetEntryIFaz()->C_IsNewNode() )
        {
            bRequestArised = (m_pIntDriver->exendlevl(0) != 0);
            bRequestArised = true;
        }

        else if( iProcType >= 0 )
        {
            if( xAtomType == CFlowAtom::AtomType::Item )
            {
                bRequestArised = m_pIntDriver->ExecuteProcVar(iSymbol, (ProcType)iProcType);

                if( bEventKillOrPost && bIsPrimaryFlow )
                {
                    VART* pVarT=VPT(iSymbol);
                    bool bIsProtected = ( pVarT->IsProtectedOrNoNeedVerif() );

                    if( !bIsProtected )
                        m_pEntryDriver->SetNewCase(false);
                }
            }

            else if( xAtomType == CFlowAtom::AtomType::BlockHead || xAtomType == CFlowAtom::AtomType::BlockTail )
            {
                bRequestArised = m_pIntDriver->ExecuteProcBlock(iSymbol, (ProcType)iProcType);
            }

            else
            {
                bRequestArised = m_pIntDriver->ExecuteProcGroup(iSymbol, (ProcType)iProcType);

                // RHF INIC Feb 17, 2003
                if( bEventKillOrPost && bIsPrimaryFlow )
                    m_pEntryDriver->SetNewCase(false);
                // RHF END Feb 17, 2003
            }

            // SAVY INTERACTIVE EDIT INIT
            //stuff for sequential field
            if( xAtomType == CFlowAtom::AtomType::Item && iProcType == PROCTYPE_PRE ) { //process sequential
                CNDIndexes theIndex;
                VARX*       pVarX = GetFieldFromAtom( pAtom, theIndex );
                VART*       pVarT = pVarX->GetVarT();

                ProcessSequentialFld(pVarT);
            }
            //Stuff for Interactive edit by Savy
            if(m_pEntryDriver->GetStopAdvance() && xAtomType == CFlowAtom::AtomType::Item ) {
                m_pEntryDriver->SetStopAdvance(false);
                bRequestArised = true;
                SetRequestNature(Reenter);
                SetTargetNode( CEntryDriver::NodeAdvanceNone );
                return true;
            }
            // SAVY INTERACTIVE EDIT END
        }

        if( EnterFlowJustStarted() )    // ... enter-flow gets immediatly back!
            return false;

        if( bRequestArised ) {
            bRequestIssued = true;
            SetRequestEventType( (EventType) iEvent );
            bBreakingEventsChain = IsRequestBreakingEventsChain();

            // always update the origin of the request  // victor Mar 04, 02
            SetRequestOrigin();                         // victor Mar 04, 02
        }
    }

    if( NoInputRequested() ) // 20131219 a noinput statement executed after an enter was previously ignored
        bRequestIssued = AskForNextField(true);

    // closing-level actions
    bool    bFinishLevel = ( bIsLevelTail && IsForwardWay() );

    if( bFinishLevel ) {
        bRequestIssued = FinishLevel( iLevel, bRequestIssued );

        if( !bRequestIssued && iLevel <= 1 )
            SetCurObject( NULL );       // no more fields when ending Levels 0 or 1
    }

    return bRequestIssued;
}

bool CsDriver::FinishLevel( int iEndingLevel, bool bRequestIssued ) { // victor Feb 23, 02
    bool    bRequestArised = false;

    if( iEndingLevel <= 0 ) {           // Level-0 epilogue doesn't generate anymore requests
        LevelEpilogue( iEndingLevel, bRequestIssued );
        // formally closing the session         <begin> // victor Feb 20, 02
        if( !DoorConditionIs(CsDriver::Closed ) ) {
            SetDoorCondition(CsDriver::Closed );
            m_pEntryDriver->m_bMustEndEntrySession = true;
        }
        // formally closing the session         <end>   // victor Feb 20, 02
    }
    else {
        bRequestArised = LevelEpilogue( iEndingLevel, bRequestIssued );

        bRequestIssued |= bRequestArised;
    }

    return bRequestIssued;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- support for level' processing                    // victor May 30, 01
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::LevelPrologue( int iLevel ) {
    // LevelPrologue: performs preliminary tasks for a Level-node before running the procs for its groupHead-atom
    bool    bRequestIssued  = false;
    bool    bIsPrimaryFlow  = ( GetFlow()->GetSubType() == SymbolSubType::Primary );
    bool    bIsModification = m_pEntryDriver->IsModification();

    if( iLevel == 0 ) {                 // Level-0: this is the session' opening
        ResetRefreshGroupOccsLimit();   // "1-up to DataOccs" (for PathOff, Modify), "0-no limit" otherwise

        if( !bIsModification )
        {
            m_pEntryDriver->DoQid();

            for( int iLevelToInit = 1; iLevelToInit <= Appl.MaxLevel; iLevelToInit++ )
                m_pEntryDriver->DeInitLevel(iLevelToInit);
        }

        m_pEntryDriver->dedriver_start();
    }
    else {
        // SAVY OCT 23, INIT 2002
        if(iLevel > 1) {
            m_pEntryDriver->SetNewCase(false);
        }
        // SAVY OCT 23, END 2002

        // ... taken from wexEntry/execute_driver/case PRELEV
        int     iSymDic = GetFlow()->GetSymDicAt( 0 );
        DICT*   pDicT   = DPT(iSymDic);
        DICX*   pDicX   = pDicT->GetDicX();
        int     iStopNode = m_pEntryDriver->iModification_Node;
        CSettings*      pSettings   = &m_pEngineDriver->m_EngineSettings;
        void*   pCaseNode = NULL;

        if( m_pEntryDriver->MustStopAtNextNode() )
            m_pEntryDriver->SetAddModeFlags();

        if( iLevel <= 1 ) {
            if( bIsPrimaryFlow ) {       // entered-flows: never "new case"
                m_pEntryDriver->SetNewCase( true );
            }
        }

        bool bReset = bIsModification && (iLevel > pDicX->LastOpenLevel);

        if( !bIsModification || bReset || !pSettings->IsPathOff() )
            m_pEntryDriver->LevCtStartLevel( iLevel );

        //TODO: in ModificationMode, the information of existing children must be set too

        if( bIsPrimaryFlow )
        {
            ASSERT(!m_pEntryDriver->InEnterMode()); //REPO_TEMP

            if( iLevel == 1 )
                pDicT->ResetPrimaryKey();

            if( !bIsModification )
            {
                m_pEntryDriver->AddLevelNode(iLevel);
                m_pEntryDriver->InitializePersistentFields(iLevel);

                if( iLevel == 1 )
                {
                    m_pEntryDriver->InitializeAutoIncrementFields();
                    m_pEntryDriver->PrefillKeyFromPff();
                    m_pEntryDriver->PrefillNonPersistentFields();
                }
            }

            else
            {
                if( iLevel == 1 && pDicX->LastOpenLevel > 0 ) {
                    ASSERT( 0 );
                    pDicX->LastOpenLevel = 0;
                }

                if( iLevel > pDicX->LastOpenLevel ) {
                    // this applies only to the first time the modified node comes to memory
                    GROUPT* pGroupTRoot = GetFlow()->GetGroupTRoot();
                    GROUPT* pGroupTLevel = pGroupTRoot->GetLevelGPT( iLevel );

                    // saving cur-occurrence of this Level-node
                    int     iSavCurOcc = pGroupTLevel->GetCurrentOccurrences();

                    // get the corresponding level node
                    m_pEntryDriver->GetNextLevelNode(iLevel);

                    // reinstalling cur-occurrence of this Level-node
                    pGroupTLevel->SetCurrentOccurrences( iSavCurOcc );

                    pDicX->LastOpenLevel = iLevel;          // RHF May 03, 2000

                    // setup primary-key of loaded case
                    if( iLevel == 1 && bIsPrimaryFlow )
                        pDicT->SetPrimaryKey();

                    // modified nodes: never "new case"
                    m_pEntryDriver->SetNewCase( false );
                }
            }
        }

        bool bResetItAlso = m_pEntryDriver->IsNewCase(); // TODO: modify once bNewCase be calculated otherwise

        LevelOpenNode( iLevel, bResetItAlso );

        m_pEntryDriver->SetActiveLevel( iLevel );
        SetLevelTailReached();
    }

    return bRequestIssued;
}

bool CsDriver::LevelOpenNode( int iOpeningLevel, bool bResetItAlso ) {
    bool    bRequestIssued = false;     // remark - no request can be issued here
    bool    bIsPrimaryFlow = ( GetFlow()->GetSubType() == SymbolSubType::Primary );

    if( iOpeningLevel >= 0 ) {
        int     iLevel      = iOpeningLevel;
        int     iMaxLevel   = GetFlowCore()->GetMaxLevel();
        GROUPT* pGroupTRoot = GetFlow()->GetGroupTRoot();
        int     iSymLevel   = pGroupTRoot->GetLevelGPT( iLevel )->GetSymbol();

        if( !bIsPrimaryFlow ) {             // entered dics have Level 1 only
            // wipe-out color flags in the target level (up to max-occs for everything)
            m_pEntryDriver->WipeGroupFieldColors( iSymLevel );
        }

        // reset occurrences of all groups...
        // ... in the opening level
        if( iLevel >= 1 )
            m_pEntryDriver->ResetGroupCurOccs( iSymLevel, bResetItAlso );

        // ... in each possible child level
        for( iLevel = iOpeningLevel + 1; iLevel <= iMaxLevel; iLevel++ ) {
            iSymLevel = pGroupTRoot->GetLevelGPT( iLevel )->GetSymbol();
            m_pEntryDriver->ResetLevelCurOccs( iLevel, true );
        }

        // reset the Level-control' forced-level
        m_pEntryDriver->LevCtResetForcedNextLevel();
    }

    return bRequestIssued;
}

bool CsDriver::LevelEpilogue( int iLevel, bool bRequestPosted ) {
    // LevelEpilogue: performs ending tasks for a Level-node after running the procs  for its groupTail-atom
    ASSERT( IsForwardWay() );           // can't arrive here when going backward
    bool    bRequestIssued  = false;
    bool    bIsPrimaryFlow  = ( GetFlow()->GetSubType() == SymbolSubType::Primary );
    bool    bIsModification = m_pEntryDriver->IsModification();

    if( !bIsPrimaryFlow ) {             // occs & data areas are not changed in externals
        SetLogicRequestNature(CsDriver::EnterFlowEnd );
        SetSymTarget();
        bRequestIssued = true;

        int     iSymFlow = GetFlow()->GetSymbol();

        GetFlAdmin()->DisableFlow( iSymFlow, true ); // bExitThroughTail is true!
    }
    else {
        RequestNature   xRequestNature = ( bRequestPosted ) ? GetRequestNature() : CsDriver::None;
        bool            bLogicEndLevelPosted = ( xRequestNature == CsDriver::LogicEndLevel );

        // ... taken from wexEntry/execute_driver/case POSLEV
        if( iLevel < 1 ) {
            // RHF INIC Sep 26, 2002
            // Fix problem with F12 in Path-On, 1 level app.  (an infinite loop).
            if( iLevel == 0 && bRequestPosted == 0 )
                KillPendingAdvances();
            // RHF END Sep 26, 2002

            return bRequestIssued;
        }

        int     iSymDic = GetFlow()->GetSymDicAt( 0 );
        DICT*   pDicT   = DPT(iSymDic);
        DICX*   pDicX   = pDicT->GetDicX();

        // set goal (or next parent-level)
        int     iGoalLevel       = iLevel - 1; // ... the parent of current, closing level
        int     iForcedNextLevel = 0;       // ... there is no forced-next=level
        bool    bHasForcedNextLevel = m_pEntryDriver->LevCtHasForcedNextLevel();

        if( bHasForcedNextLevel ) {
            // ... the parent of forced, requested level
            iGoalLevel = m_pEntryDriver->LevCtGetForcedNextLevel() - 1;

            iForcedNextLevel = iGoalLevel + 1;

            // anyway, reset the Level-control' forced-level
            m_pEntryDriver->LevCtResetForcedNextLevel();
        }

        // closes every level-nodes stopping at the goal-level
        for( int iEndingLevel = iLevel; iEndingLevel > iGoalLevel; iEndingLevel-- ) {
            // get & reset forced-no-write request for this level
            bool    bIgnoreWrite = m_pEntryDriver->LevCtHasForcedNoWrite( iEndingLevel );
            if( bIgnoreWrite )          // set forced-no-write condition to false
                m_pEntryDriver->LevCtResetForcedNoWrite( iEndingLevel );

            // execute end of level for the closing' level
            bRequestIssued = LevelCloseNode( iEndingLevel, bIgnoreWrite );

            if( bRequestIssued )        // ... the interface must capture the field again
                return bRequestIssued;

            // adjusting ForcedNextLevels
            if( iForcedNextLevel > 0 ) { // && iEndingLevel == iLevel
                // advance nodes, declaring each one "written"
                // REPO_TEMP: what was the next scenario for?
                //m_pEntryDriver->DataMapAdvanceToLevelNode( iForcedNextLevel, NodeStatusWRITTEN );
                iForcedNextLevel = 0;
            }

            pDicX->LastOpenLevel = iEndingLevel - 1;    // RHF May 03, 2000

            // finally, set the ending-level as active
            m_pEntryDriver->SetActiveLevel( iEndingLevel );

            // BUCEN Changes Jan, 2004
            CNPifFile* pPifFile = m_pEngineDriver->GetPifFile();
            bool bAutoAdd = ( pPifFile == nullptr ) ? true : pPifFile->GetAutoAddFlag();
            // BUCEN Changes Jan, 2004

            // Modify: Post-Level-1 done
            if( bIsModification || !bAutoAdd || m_pEntryDriver->IsInsertMode() )
            {
                // once Post-Level-1 was done...
                if( iEndingLevel == 1 ) {
                   m_pEntryDriver->Decorrlevl = iEndingLevel;
// TODO            break;               // ... Level-zero is not reached here???
                }
            }

            else // auto add
            {
                m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStop);
                m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStart);
            }
        }

        if( bLogicEndLevelPosted ) {
            DontRemakeOrigin();         // attn - must not change the origin of the LogicEndLevel
            bRequestIssued = true;
        }
        else {
            LevelRestart( iLevel );// cleaning everything
            SetLevelTailReached();
            SetProgressBitmapOn();

            // warns of level-tail reached when no LogicEndLevel requested
            if( xRequestNature != CsDriver::LogicEndLevel ) {
                    SetLevelTailReached( iLevel );

                // when modifying a case, force level-0' tail once this level-1 completed
                int     iNextAtom = -1;

                if( iLevel == 1 ) {
                // BUCEN Changes Jan, 2004
                    CNPifFile*   pPifFile = m_pEngineDriver->GetPifFile();
                                        bool bAutoAdd = true ;
                                        if(pPifFile) {
                                                bAutoAdd =pPifFile->GetAutoAddFlag();
                                        }
                // BUCEN Changes Jan, 2004
                    if( bIsModification || !bAutoAdd || m_pEntryDriver->IsInsertMode() )
                        iNextAtom = GetFlowCore()->GetLevelTailIndex( iLevel - 1 );
                    else {
                        iNextAtom = GetFlowCore()->GetLevelHeadIndex( iLevel - 1 );
                        SetLevelTailReached();          // victor Mar 14, 02
                    }
                    SetCurrAtomIndex( iNextAtom );
                }
            }
        }
    }

    return bRequestIssued;
}

bool CsDriver::LevelCloseNode( int iEndingLevel, bool bIgnoreWrite ) {
    // LevelCloseNode: taken from wexEntry/execute_EndOfLevel
    bool    bRequestIssued = false;
    bool    bIsPrimaryFlow = ( GetFlow()->GetSubType() == SymbolSubType::Primary );
    ASSERT( bIsPrimaryFlow );           // this function should not be used by externals!
    bool    bIsModification = m_pEntryDriver->IsModification();

    if( m_pEngineDriver->m_pEngineSettings->GetInteractiveEdit() && m_pEntryDriver->GetIgnoreWrite() )
        bIgnoreWrite = true;

    if( bIsPrimaryFlow && iEndingLevel >= 0 ) {
        int     iSymDic   = GetFlow()->GetSymDicAt( 0 );
        int     iMaxLevel = GetFlowCore()->GetMaxLevel();
        bool    bIsReady  = m_pEntryDriver->CheckBeforeWriteEndingNode( iSymDic, iMaxLevel, iEndingLevel, bIgnoreWrite );

        if( !bIsReady ) {               // id-collision or not accepted by the operator
            // set a request to reenter the last entered field
            SetInterRequestNature( PrevField, true );// RHF Oct 17, 2002 Add true
            bRequestIssued = true;
        }
        else {
            // just write the node using a simplified version of the old stuff
            m_pEntryDriver->WriteEndingNode( iEndingLevel, bIgnoreWrite );

            if( iEndingLevel > 0 ) {    // for actual nodes...
                // ... clean data areas and ALL occurrences
                LevelCleanData( iEndingLevel );  // ... equivalent to old 'DeInitLevel'
                // ... set the lights off
                LevelCleanColors( iEndingLevel );// ... equivalent to old 'delow_level'
                if(iEndingLevel == 1){
                    m_pEntryDriver->SetPartialMode(NO_MODE);
                }
            }

            // LevCt: controlling levels operation
            m_pEntryDriver->LevCtInitLevel( iEndingLevel );
        }
    }

    return bRequestIssued;
}

bool CsDriver::LevelRestart( int iLevel ) {
    // LevelRestart: for a given Level, goes straight to the groupHead-atom of the level, wipes colors, and cleans the data
    CFlowCore*  pFlowCore = GetFlowCore();
    int         iMaxLevel = pFlowCore->GetMaxLevel();
    bool        bDone     = ( iLevel >= 0 && iLevel <= iMaxLevel );

    if( bDone ) {
        // ... clean data areas and ALL occurrences
        LevelCleanData( iLevel );       // ... equivalent to old 'DeInitLevel'

        // ... set the lights off
        LevelCleanColors( iLevel );     // ... equivalent to old 'delow_level'

        m_pEngineDriver->ResetDynamicAttributes();

        // set the level' groupHead-atom as current-atom
        int     iAtom = pFlowCore->GetLevelHeadIndex( iLevel );

        SetCurrAtomIndex( iAtom );
        SetProgressBitmapOn();                          // victor May 28, 01

        // TODO should stay at this level' GroupHead, or go back one more atom???

        SetForwardWay( true );              // insures forward way
    }

    return bDone;
}

bool CsDriver::LevelCleanData( int iLevel ) {
    // LevelCleanData: for a given Level, clean data areas and ALL occurrences in that Level and descendant-levels also
    // ... equivalent to old 'DeInitLevel'
    int     iMaxLevel = GetFlowCore()->GetMaxLevel();
    bool    bDone     = ( iLevel >= 0 && iLevel <= iMaxLevel );

    if( bDone ) {
        // initializing data areas for each Sect at the level
        int     iSymSec = DIP(0)->SYMTfsec;

        while( iSymSec >= 0 ) {
            SECT*   pSecT = SPT( iSymSec );

            if( pSecT->GetLevel() == iLevel )
                m_pEngineDriver->initsect( pSecT );

            iSymSec = pSecT->SYMTfwd;
        }

        // initializing group occurrences
        GROUPT* pGroupTRoot = GetFlow()->GetGroupTRoot();
        bool    bIsModification = m_pEntryDriver->IsModification();

        for( int iAtLevel = iLevel; iAtLevel <= iMaxLevel; iAtLevel++ ) {
            // ... this is equivalent to old 'InitLevelGroups'
            GROUPT* pGroupTLevel = pGroupTRoot->GetLevelGPT( iAtLevel );

            // for each group in this level-group
            for( int iItem = 0; iItem < pGroupTLevel->GetNumItems(); iItem++ ) {
                int     iSymItem = pGroupTLevel->GetItemSymbol( iItem );

                if( iSymItem > 0 && NPT(iSymItem)->IsA( SymbolType::Group ) )
                    GPT(iSymItem)->InitOneGroup();
            }
        }
    }

    return bDone;
}

bool CsDriver::LevelCleanColors( int iLevel ) {
    // LevelCleanColors: for a given Level, wipes out the color of all item-atoms in that Level and descendant-levels also
    // ... equivalent to old 'delow_level'
    CFlowCore*  pFlowCore = GetFlowCore();
    int         iMaxLevel = pFlowCore->GetMaxLevel();
    bool        bDone     = ( iLevel >= 0 && iLevel <= iMaxLevel );

    if( bDone ) {
        int     iHeadAtom = pFlowCore->GetLevelHeadIndex( iLevel );
        int     iTailAtom = pFlowCore->GetLevelTailIndex( iLevel );

        // save the current atom
        int     iCurrAtom = pFlowCore->GetLevelHeadIndex( iLevel );

        for( int iAtom = iHeadAtom; iAtom <= iTailAtom; iAtom++ ) {
            SetCurrAtomIndex( iAtom );

            CFlowAtom*          pAtom = GetCurrAtom();
            CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();

            if( xAtomType == CFlowAtom::AtomType::Item )
                SetFieldColorNone( pAtom );
        }

        SetCurrAtomIndex( iCurrAtom );  // restore the current-atom
    }

    return bDone;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- generic evaluation and searching methods
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::SameBranch( C3DObject* p3DObsObject, C3DObject* p3DRefObject ) { // RHF Jun 18, 2001
    // SameBranch: return true if a given observed-object is in the same branch of a given reference-object
    bool    bSameBranch = true;
    ASSERT( p3DRefObject != NULL );
    ASSERT( p3DObsObject != NULL );
    int     iRefSymbol  = p3DRefObject->GetSymbol();
    SymbolType eRefSymType = NPT(iRefSymbol)->GetType();
    int     iObsSymbol  = p3DObsObject->GetSymbol();
    SymbolType eObsSymType = NPT(iObsSymbol)->GetType();
    ASSERT( eRefSymType == SymbolType::Group || eRefSymType == SymbolType::Variable || eRefSymType == SymbolType::Block );
    ASSERT( eObsSymType == SymbolType::Group || eObsSymType == SymbolType::Variable || eObsSymType == SymbolType::Block );

    // get the symbol of the Reference' group
    int iRefSymGroup;

    if( eRefSymType == SymbolType::Group )
        iRefSymGroup = iRefSymbol;
    else if( eRefSymType == SymbolType::Variable )
        iRefSymGroup = VPT(iRefSymbol)->GetOwnerGroup();
    else if( eRefSymType == SymbolType::Block )
        iRefSymGroup = GetSymbolEngineBlock(iRefSymbol).GetGroupT()->GetSymbolIndex();
    else
        ASSERT(0);

    // get the symbol of the Observed' group
    int     iObsSymGroup;

    if( eObsSymType == SymbolType::Group )
        iObsSymGroup = iObsSymbol;
    else if( eObsSymType == SymbolType::Variable )
        iObsSymGroup = VPT(iObsSymbol)->GetOwnerGroup();
    else if( eObsSymType == SymbolType::Block )
        iObsSymGroup = GetSymbolEngineBlock(iObsSymbol).GetGroupT()->GetSymbolIndex();
    else
        ASSERT(0);

    // analyze the basic relationship among Observed and Reference groups
    GROUPT*     pRefGroupT   = GPT(iRefSymGroup);
    GROUPT*     pObsGroupT   = GPT(iObsSymGroup);
    bool        bRefIsLevel  = ( pRefGroupT->GetGroupType() == GROUPT::Level );
    bool        bObsIsLevel  = ( pObsGroupT->GetGroupType() == GROUPT::Level );
    int         iRefLevelNum = pRefGroupT->GetLevel();
    int         iObsLevelNum = pObsGroupT->GetLevel();


    // basic conditions: one or both are Level-groups
    if( bRefIsLevel && bObsIsLevel ) {
        if( iRefLevelNum != iObsLevelNum )
            bSameBranch = false;
    }
    else if( bRefIsLevel ) {
        if( iRefLevelNum != iObsLevelNum )
            bSameBranch = false;
    }
    else if( bObsIsLevel ) {
        if( iObsLevelNum != iRefLevelNum )
            bSameBranch = false;
    }
    // when one or both were Level-groups, the analyze ends here
    if( bRefIsLevel || bObsIsLevel )
        return bSameBranch;

    // other conditions
    if( iRefLevelNum != iObsLevelNum )
        bSameBranch = false;
    else {
        // ancestor conditions
        bool    bCheckAncestor = true;  // Maybe in the future will be a parameter

        if( bCheckAncestor && pRefGroupT != pObsGroupT ) {
            if( !pRefGroupT->IsAncestor( iObsSymGroup, false ) )
                bSameBranch = false;
            else if( !pObsGroupT->IsAncestor( iRefSymGroup, false ) )
                bSameBranch = false;
        }
    }
    if( !bSameBranch )
        return bSameBranch;

    // starting here, there are two possible relationships:
    // ... both are the same group
    // ... one of the groups is ancestor of the other
    // now check the indexes
    CNDIndexes aRefIndex = p3DRefObject->getIndexes();
    CNDIndexes aObsIndex = p3DObsObject->getIndexes();

    // Set to 0 all NOT common dimension
    // remark the index-sets are "each index at its dimension-axis" (zero when no index at the axis)
    int iDimMax = aObsIndex.getIndexNumber();
    for( int iDim = 0; iDim < iDimMax; iDim++ ) {
        if( aObsIndex.getIndexValue(iDim) && !aRefIndex.getIndexValue(iDim) )
            aObsIndex.setIndexValue(iDim,0);
        else if( !aObsIndex.getIndexValue(iDim) && aRefIndex.getIndexValue(iDim) )
            aRefIndex.setIndexValue(iDim,0);
    }

    // will be at the same branch if (the surviving) indexes are identical
    for( int i = 0; bSameBranch && i < iDimMax; i++ )
        bSameBranch = ( aObsIndex.getIndexValue(i) == aRefIndex.getIndexValue(i) );

    return bSameBranch;
}

bool CsDriver::CanDecrementOcc( C3DObject* p3DSource, bool bPathOn ) {
    // CanDecrementOcc: return true if a given group-occurrence is empty
    // ... remarks:
    //   - PathOn : search backward first highlighted item
    //   - PathOff: search backward first highlighted or midlight item
    bool        bDecrement = false;

    // Take the ocurrences from the founded item
    C3DObject*   p3DPrevColoredObject;

    if( bPathOn )
        p3DPrevColoredObject = SearchFieldHighColor( false );
    else
        p3DPrevColoredObject = SearchFieldWithColor( false );

    if( p3DPrevColoredObject == NULL || !SameBranch( p3DSource, p3DPrevColoredObject ) )
        bDecrement = true;

    return bDecrement;
}

int CsDriver::FindActualOccs( const C3DObject& the3DReference ) { // victor Dec 10, 01
    // FindActualOccs: scan backward the occurrences of a group starting at a given HTOcc (Last or Tail) atom and returns the effective number of occurrences entered
    int             iSymbol     = the3DReference.GetSymbol();
    ASSERT( NPT(iSymbol)->IsA(SymbolType::Group) );
    GROUPT*         pGroupT     = GPT(iSymbol);
    CSettings*      pSettings   = &m_pEngineDriver->m_EngineSettings;
    bool            bPathOff    = pSettings->IsPathOff();
    int             iCurrAtom   = GetCurrAtomIndex(); // save the current-atom

    RestartFlow();

    // look forward for the HTOcc(bwd) equivalent to the reference object
    CFlowHTOcc*     pHTOcc;
    CFlowAtom*      pAtom;
    CFlowAtom::AtomType xAtomType;
    C3DObject       o3DObserved;
    bool            bCanGoAhead = true;
    bool            bPivotFound = false;

    while( bCanGoAhead && !bPivotFound ) {
        pAtom = GetNextAtom();
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType ) {
            case CFlowAtom::AtomType::HTOcc:
                pHTOcc  = (CFlowHTOcc*) pAtom;
                o3DObserved.SetSymbol( pHTOcc->GetSymbol() );
                o3DObserved.setIndexes( pHTOcc->GetIndexBwd() );
                bPivotFound = ( o3DObserved == the3DReference );
                break;
            default:
                bCanGoAhead = ( xAtomType != CFlowAtom::AtomType::BeyondStrip );
                break;
        }
    }

    // look backward for the first non-empty field (breaks when GroupHead found)
    int             iFieldColor;
    bool            bHasColor = false; // Set default value, rcl, Jul 15, 2004
    bool            bCanGoBack  = bPivotFound;
    bool            bColorFound = false;

    while( bCanGoBack && !bColorFound ) {
        pAtom = GetPrevAtom();
        xAtomType = pAtom->GetAtomType();

        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
                bCanGoBack = false;
                break;
            case CFlowAtom::AtomType::HTOcc:
                pHTOcc  = (CFlowHTOcc*) pAtom;
                // ASSERT( pHTOcc->GetSymbol() == o3DObserved.GetSymbol() );
                o3DObserved.SetSymbol( pHTOcc->GetSymbol() );
                o3DObserved.setIndexes( pHTOcc->GetIndexBwd() );
                break;
            case CFlowAtom::AtomType::Item:
                iFieldColor = GetFieldColor( pAtom );
                bHasColor   = ( iFieldColor == FLAG_HIGHLIGHT || bPathOff && iFieldColor == FLAG_MIDLIGHT );
                break;
            case CFlowAtom::AtomType::BlockHead:
            case CFlowAtom::AtomType::BlockTail:
            case CFlowAtom::AtomType::GroupTail:
                // please, continue looking back
                break;
            default:
                ASSERT( 0 );            // can't be
                bCanGoBack = false;
                break;
        }
        bColorFound = bHasColor; // default value needed here, rcl Jul 15, 2004
    }

    // set the high-occ to be returned
    int             iHighOcc = -1;

    if( !bPivotFound )
        iHighOcc = -1;                  // could not locate a pivot
    else if( !bColorFound )
        iHighOcc = 0;                   // no colored field found
    else if( !pGroupT->GetNumDim() )
        iHighOcc = 1;                   // colored found, single
    else {
        // extract the high-occ from o3DObserved (???)
        CDimension::VDimType    xDimType = pGroupT->GetDimType();

        iHighOcc = o3DObserved.getIndexValue( xDimType );
    }

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return iHighOcc;
}

//Savy - using optimize target compuatation during skip to.
//for each iteration, target search is reevaluated causing slow down in long skips.
//added new flag to reuse computed target atom index. Used only for skips
int CsDriver::SearchTargetLocation( C3DObject* p3DTarget, int iRefAtom, int iSearchWay, bool bFillTargetAtomIndex/*= false*/) { // victor Dec 10, 01
    // SearchTargetLocation: compares the target with a reference atom (the current atom by default) and return
    //      0 ... are the same (or invalid target, or not found in this flow)
    //      1 ... target is located after the reference atom
    //     -1 ... target is located before the reference atom
    // the parameter iSearchWay asks for
    //      0 ... search forward, then backward (default option)
    //      1 ... search forward only
    //     -1 ... search backward only
    int         iLocation = 0;          // assuming "same as reference atom"
    bool        bSearchInPlace  = ( iSearchWay == 0 );
    bool        bSearchForward  = ( iSearchWay >= 0 );
    bool        bSearchBackward = ( iSearchWay <= 0 );
    int         iCurrAtom = GetCurrAtomIndex(); // save the current-atom
    int         iPivotAtom = ( iRefAtom >= 0 ) ? iRefAtom : iCurrAtom;

    if (!bFillTargetAtomIndex)
        p3DTarget->SetAtomIndex(-1);

    // setup internal version of the target
    int iSymTarget = p3DTarget->GetSymbol();
    SymbolType eTargetSymType = NPT(iSymTarget)->GetType();
    ASSERT( eTargetSymType == SymbolType::Group || eTargetSymType == SymbolType::Block || eTargetSymType == SymbolType::Variable );

    if( eTargetSymType != SymbolType::Group && eTargetSymType != SymbolType::Block && eTargetSymType != SymbolType::Variable )
        return iLocation;               // invalid target - return "same as reference atom"

    if( IsNodeBoundTarget() )
        return 1;

    int iTargetAtomIndex = -1;

    //Savy to fix the redundant calls to
    if( bSearchInPlace &&               // check the target against the reference atom
        SearchTargetAtomIndex( p3DTarget, iPivotAtom, 0 ) >= 0 )
        bSearchForward = bSearchBackward = false;


    if( bSearchForward &&               // search forward if needed
        (iTargetAtomIndex = SearchTargetAtomIndex( p3DTarget, iPivotAtom, 1 )) >= 0) {
        iLocation = 1;                  // the target is located "after the reference atom"
        bSearchBackward = false;
    }

    if( bSearchBackward &&              // search backward if needed
        (iTargetAtomIndex = SearchTargetAtomIndex( p3DTarget, iPivotAtom, -1 )) >= 0)
        iLocation = -1;                 // the target is located "before the reference atom"


    if (bFillTargetAtomIndex) {
        if (p3DTarget->GetAtomIndex() > -1 && bSearchForward && iTargetAtomIndex > p3DTarget->GetAtomIndex())//past or on target. stop using the stored Atomindex
            p3DTarget->SetAtomIndex(-1);
        else if (p3DTarget->GetAtomIndex() > -1 && bSearchBackward && iTargetAtomIndex < p3DTarget->GetAtomIndex())//past or on target. stop using the stored Atomindex
            p3DTarget->SetAtomIndex(-1);
        else
            p3DTarget->SetAtomIndex(iTargetAtomIndex);
    }
    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return iLocation;
}

//Savy - using optimize target computation during skip to.
//for each iteration, target search is reevaluated causing slow down in long skips.
//added new flag to reuse computed target atom index. Used only for skips
int CsDriver::SearchTargetAtomIndex( C3DObject* p3DTarget, int iPivotAtom, int iWay ) { // victor Dec 10, 01
    int         iAtomIndex       = -1;
    int         iSymTarget       = p3DTarget->GetSymbol();
    SymbolType  eTargetType = NPT(iSymTarget)->GetType();
    bool        bStopSearch = ( eTargetType != SymbolType::Group && eTargetType != SymbolType::Block && eTargetType != SymbolType::Variable );
    ASSERT( !bStopSearch );

    SetCurrAtomIndex( iPivotAtom );     // set the pivot-atom

    while( iAtomIndex < 0 && !bStopSearch ) {
        CFlowAtom*  pAtom;

        if( iWay )
            pAtom = ( iWay > 0 ) ? GetNextAtom() : GetPrevAtom();
        else {
            pAtom = GetCurrAtom();
            bStopSearch = true;
        }

        CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();

        if (p3DTarget->GetAtomIndex() != -1 &&  iWay != 0) {
            // forward search
            if (iWay  > 0 &&  GetCurrAtomIndex() < p3DTarget->GetAtomIndex()) {
                //We have not passed the target index
                iAtomIndex = p3DTarget->GetAtomIndex();
                bStopSearch = true;
                break;
            }
            else if (iWay < 0 && GetCurrAtomIndex() > p3DTarget->GetAtomIndex()) {//backward search
                //We have not passed the target index
                iAtomIndex = p3DTarget->GetAtomIndex();
                bStopSearch = true;
                break;
            }
        }

        switch( xAtomType )
        {
            case CFlowAtom::AtomType::GroupTail:
            case CFlowAtom::AtomType::HTOcc:
            case CFlowAtom::AtomType::BlockTail:
                break;          // not evaluated as target

            case CFlowAtom::AtomType::GroupHead:
            {
                CFlowGroupHead* pGroupHead = (CFlowGroupHead*) pAtom;
//                o3DObserved.SetSymbol( pGroupHead->GetSymbol() );
//                o3DObserved.setIndexes( pGroupHead->GetIndexFwd() );
//                if( o3DObserved == *p3DTarget )
//                    iAtomIndex = GetCurrAtomIndex();
                // GroupHead does not consider group occurrences indexes
                // to find a match
                // rcl, May 2005
                if( pGroupHead->GetSymbol() == p3DTarget->GetSymbol() )
                    iAtomIndex = GetCurrAtomIndex();
                break;
            }

            case CFlowAtom::AtomType::BlockHead:
            {
                CFlowBlockHead* pFlowBlockHead = (CFlowBlockHead*)pAtom;

                if( pFlowBlockHead->GetSymbol() == p3DTarget->GetSymbol() )
                {
                    if( pFlowBlockHead->GetIndex() == p3DTarget->GetIndexes() )
                        iAtomIndex = GetCurrAtomIndex();
                }

                break;
            }

            case CFlowAtom::AtomType::Item:
            {
                if( eTargetType == SymbolType::Variable ) {
                    CFlowItem* pFlowItem = (CFlowItem*) pAtom;
                    if (pFlowItem->GetSymbol() == p3DTarget->GetSymbol()) {
                        C3DObject           o3DObserved;
                        o3DObserved.SetSymbol(pFlowItem->GetSymbol());
                        o3DObserved.setIndexes(pFlowItem->GetIndex());
                        if (o3DObserved == *p3DTarget)
                            iAtomIndex = GetCurrAtomIndex();
                    }
                }
                break;
            }

            case CFlowAtom::AtomType::BeyondStrip:
                ASSERT( iWay >= 0 );    // corrupted flow-strip otherwise
                bStopSearch = true;
                break;

            case CFlowAtom::AtomType::BeforeStrip:
                ASSERT( iWay <= 0 );    // corrupted flow-strip otherwise
                bStopSearch = true;
                break;

            default:
                ASSERT( 0 );            // can't be (invalid atom) - corrupted flow-strip
                bStopSearch = true;
                break;
        }
    }
    SetCurrAtomIndex( iPivotAtom );     // set the pivot-atom

    return iAtomIndex;
}

C3DObject* CsDriver::SearchFieldWithValue( bool bFromPrevAtom ) { // victor Jun 21, 01
    // SearchFieldWithValue: starting at a given atom, searchs backward for the nearest field with color and non-blank ascii contents and return a 3D-object with its description
    C3DObject*  p3DObject   = NULL;
    CSettings*  pSettings   = &m_pEngineDriver->m_EngineSettings;
    bool        bPathOff    = pSettings->IsPathOff();
    int         iCurrAtom   = GetCurrAtomIndex(); // save the current-atom
    CFlowAtom*  pAtomLimit  = GetFlowCore()->GetLowestFieldAtom();
    CFlowAtom*  pAtom       = GetCurrAtom();
    bool        bCanGoBack  = ( pAtom != pAtomLimit );
    int         iSymVar;
    int         iFieldColor;
    bool        bHasColor;
    int         iVarLen;
    csprochar*       pszAsciiAddr;
    csprochar        pBuf[1024];
    csprochar*       pBlank;

    if( bFromPrevAtom && bCanGoBack ) {
        pAtom = GetPrevAtom();
        bCanGoBack = ( pAtom != pAtomLimit );
    }

    while( bCanGoBack && p3DObject == NULL ) {
        if( pAtom->GetAtomType() == CFlowAtom::AtomType::Item ) {
            iSymVar     = p3DObject->GetSymbol();
            iFieldColor = GetFieldColor( pAtom );
            bHasColor   = ( iFieldColor == FLAG_HIGHLIGHT || bPathOff && iFieldColor == FLAG_MIDLIGHT );

            // when the field has a color...
            if( bHasColor ) {
                // ... looks for the ascii' contents
                pszAsciiAddr = FurnishFieldAsciiAddr( p3DObject );
                ASSERT( pszAsciiAddr != NULL );

                // ... prepare a blank area to compare the contents
                iVarLen = VPT(iSymVar)->GetLength();
                pBlank  = ( iVarLen <= 1024 ) ? pBuf : (csprochar*) calloc( iVarLen, sizeof(csprochar) );
                _tmemset( pBlank, _T(' '), iVarLen );

                // ... when non-empty, get the 3D-object from this atom
                if( _tmemcmp( pszAsciiAddr, pBlank, iVarLen ) != 0 )
                    p3DObject = ExtractInfoFromAtom( pAtom );

                if( pBlank != pBuf )
                    free( pBlank );
            }
        }

        pAtom = GetPrevAtom();
        bCanGoBack = ( pAtom != pAtomLimit );
    }

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return p3DObject;
}

C3DObject* CsDriver::SearchFieldByColor( bool bFromPrevAtom, bool bHigh, bool bMid ) { // victor Jun 21, 01
    // SearchFieldByColor: starting at the current atom, searchs backward for the nearest field colored as requested and return a 3D-object with its description
    C3DObject*  p3DObject   = NULL;
    int         iCurrAtom   = GetCurrAtomIndex(); // save the current-atom
    CFlowAtom*  pAtomLimit  = GetFlowCore()->GetLowestFieldAtom();
    CFlowAtom*  pAtom       = GetCurrAtom();
    bool        bCanGoBack  = ( pAtom != pAtomLimit );
    bool        bNoColor    = ( !bHigh && !bMid );
    int         iFieldColor;

    if( bFromPrevAtom && bCanGoBack ) {
        pAtom = GetPrevAtom();
        bCanGoBack = ( pAtom != pAtomLimit );
    }

    while( bCanGoBack && p3DObject == NULL ) {
        if( pAtom->GetAtomType() == CFlowAtom::AtomType::Item ) {
            iFieldColor = GetFieldColor( pAtom );

            // when matching the required color(s), copy the item to the 3D-object
            if( bHigh    && iFieldColor == FLAG_HIGHLIGHT ||
                bMid     && iFieldColor == FLAG_MIDLIGHT  ||
                bNoColor && iFieldColor == FLAG_NOLIGHT    )
                p3DObject = ExtractInfoFromAtom( pAtom );
        }

        pAtom = GetPrevAtom();
        bCanGoBack = ( pAtom != pAtomLimit );
    }

    SetCurrAtomIndex( iCurrAtom );          // restore the current-atom

    return p3DObject;
}

C3DObject* CsDriver::SearchFieldPrevToGroup( int iAtLevel ) { // victor Jun 21, 01
    // SearchFieldPrevToGroup: starting at the current atom, searchs backward for the first field preceeding the group and return a 3D-object with its description
    C3DObject*  p3DObject       = NULL;
    int         iCurrAtom       = GetCurrAtomIndex(); // save the current-atom
    CFlowCore*  pFlowCore       = GetFlowCore();
    CFlowAtom*  pAtomLimit      = pFlowCore->GetLowestFieldAtom();
    CFlowAtom*  pAtom           = GetCurrAtom();
    bool        bCanGoBack      = ( pAtom != pAtomLimit );
    int         iSymGroupSource = pFlowCore->GetAtomGroupSymbol( pAtom );
    ASSERT( iSymGroupSource > 0 );
    int         iSymGroup;
    bool        bFound          = false;

    while( bCanGoBack && !bFound ) {
        pAtom = GetPrevAtom();
        if( pAtom->GetAtomType() != CFlowAtom::AtomType::Item )
            bCanGoBack = ( pAtom != pAtomLimit );
        else {
            iSymGroup  = pFlowCore->GetAtomGroupSymbol( pAtom );

            // found... when the item-atom belongs to another Group
            if( iSymGroup != iSymGroupSource ) {
                // if requested, check that the atom is at the required level
                if( !iAtLevel )
                    bFound = true;
                else
                    bFound = ( pFlowCore->GetAtomLevel( pAtom ) == iAtLevel );
            }
        }
    }
    if( bFound )
        p3DObject = ExtractInfoFromAtom( pAtom );

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return p3DObject;
}

C3DObject* CsDriver::SearchFieldNextToGroup( int iAtLevel ) { // victor Jun 21, 01
    // SearchFieldNextToGroup: starting at the current atom, searchs forward for the first field following the group and return a 3D-object with its description
    C3DObject*  p3DObject       = NULL;
    int         iCurrAtom       = GetCurrAtomIndex(); // save the current-atom
    CFlowCore*  pFlowCore       = GetFlowCore();
    CFlowAtom*  pAtom           = GetCurrAtom();
    int         iSymGroupSource = pFlowCore->GetAtomGroupSymbol( pAtom );
    ASSERT( iSymGroupSource > 0 );
    int         iSymGroup;
    bool        bFound          = false;

    while( !bFound ) {
        if( pAtom->GetAtomType() == CFlowAtom::AtomType::BeyondStrip )
            break;

        pAtom = GetNextAtom();
        if( pAtom->GetAtomType() == CFlowAtom::AtomType::Item ) {
            iSymGroup  = pFlowCore->GetAtomGroupSymbol( pAtom );

            // found... when the item-atom belongs to another Group
            if( iSymGroup != iSymGroupSource ) {
                // if requested, check that the atom is at the required level
                if( !iAtLevel )
                    bFound = true;
                else
                    bFound = ( pFlowCore->GetAtomLevel( pAtom ) == iAtLevel );
            }
        }
    }
    if( bFound )
        p3DObject = ExtractInfoFromAtom( pAtom );

    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return p3DObject;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- managing field values and colors (of fields given by an item-atom)
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::AcceptFieldValue( CFlowAtom* pAtom ) {    // victor May 21, 01
    // EnterFieldValue: for numeric fields, check if "keyed value" is in ranges
    // ... the "keyed value" is supposed to be already passed to engine by means of CEntryIFaz::C_FldPutVal
    bool bRequestIssued = false;
    bool bAccepted = true;  // benign assumption - accepted
    bool request_issued_in_on_refused = false;
    CNDIndexes theIndex;
    VARX* pVarX = GetFieldFromAtom( pAtom, theIndex );
    VART* pVarT = NULL;                           // victor Mar 15, 02
    std::shared_ptr<Paradata::FieldValidationEvent> field_validation_event;

    // process true items only
    if( pVarX != NULL )
    {
        pVarT = pVarX->GetVarT();

        if( Paradata::Logger::IsOpen() )
        {
            std::shared_ptr<Paradata::FieldValueInfo> field_value_info;

            if( m_pEngineDriver->GetPifFile()->GetApplication()->GetApplicationProperties().GetParadataProperties().GetRecordValues() )
                field_value_info = m_pIntDriver->m_pParadataDriver->CreateFieldValueInfo(pVarT, theIndex);

            std::shared_ptr<Paradata::FieldEntryInstance> field_entry_instance;

            if( m_currentFieldMovementInstanceForValidationEvent != nullptr )
            {
                field_entry_instance = m_currentFieldMovementInstanceForValidationEvent->GetToFieldEntryInstance();

                // any future validation will not be for the current field
                m_currentFieldMovementInstanceForValidationEvent.reset();
            }

            // theIndex is zero-based but CreateFieldInfo expects a one-based index
            CFlowItem* pFlowItem = (CFlowItem*)pAtom;
            const CNDIndexes& theOneBasedIndex = pFlowItem->GetIndex();

            field_validation_event = std::make_shared<Paradata::FieldValidationEvent>(
                m_pIntDriver->m_pParadataDriver->CreateFieldInfo(pVarT, theOneBasedIndex),
                m_pIntDriver->m_pParadataDriver->CreateFieldValidationInfo(pVarT),
                field_value_info,
                field_entry_instance
            );
        }


        // fields are validated if they are:
        // 1) numeric
        // 2) alphas of length 1
        // 3) alphas that aren't text/combo boxes (as of CSPro 7.3)
        CaptureType evaluated_capture_type = pVarT->GetEvaluatedCaptureInfo().GetCaptureType();

        bool validate_field = pVarT->IsNumeric() ||
                              ( pVarT->GetLength() == 1 ) ||
                              ( evaluated_capture_type != CaptureType::TextBox &&
                                evaluated_capture_type != CaptureType::ComboBox &&
                                evaluated_capture_type != CaptureType::NumberPad );

        if( validate_field )
            bAccepted = pVarX->InRange(theIndex);

        if( field_validation_event != nullptr )
            field_validation_event->SetInValueSet(bAccepted);


        if( bAccepted )
        {
            // verify if a refused value should be accepted
            if( pVarT->IsNumeric() && m_pIntDriver->HasSpecialFunction(SpecialFunction::OnRefused) &&
                m_pIntDriver->GetVarFloatValue(pVarX, theIndex) == REFUSED )
            {
                ASSERT(!m_pIntDriver->GetRequestIssued());

                double on_refused_result = m_pIntDriver->ExecSpecialFunction(pVarT->GetSymbolIndex(), SpecialFunction::OnRefused, { });

                if( m_pIntDriver->GetRequestIssued() )
                {
                    request_issued_in_on_refused = true;
                    on_refused_result = 0;
                }

                if( field_validation_event != nullptr )
                    field_validation_event->SetOnRefusedResult(on_refused_result);

                bAccepted = ( on_refused_result != 0 );
            }
        }


        else
        {
            bool value_is_notappl = false;
            bool can_enter_notappl = ( ( pVarT->m_iBehavior & CANENTER_NOTAPPL ) != 0 );
            bool can_enter_out_of_range = ( ( pVarT->m_iBehavior & CANENTER_OUTOFRANGE) != 0 );
            std::optional<double> field_value;

            // potentially accept the value based on the field properties
            if( m_pEntryDriver != nullptr )
            {
                if( pVarT->IsNumeric() )
                {
                    field_value = m_pIntDriver->GetVarFloatValue(pVarX, theIndex);

                    value_is_notappl = ( *field_value == NOTAPPL );

                    bAccepted = value_is_notappl &&                                                 // value is notappl
                                can_enter_notappl &&                                                // can enter notappl
                                ( ( pVarT->m_iBehavior & CANENTER_NOTAPPL_NOCONFIRM ) != 0 );       // don't ask
                }

                if( !bAccepted && !value_is_notappl )
                {
                    bAccepted = can_enter_out_of_range &&                                           // can enter out of range
                                ( ( pVarT->m_iBehavior & CANENTER_OUTOFRANGE_NOCONFIRM ) != 0 );    // don't ask

                    bAccepted |= !m_pEntryDriver->GetStopOnOutOfRange(); // also potentially accept the value during an interactive edit
                }

                // warn about the invalid response (if the field is not protected)
                if( !bAccepted && !pVarT->IsProtectedOrNoNeedVerif() )
                {
                    bAccepted = m_pEntryDriver->ConfirmValue(value_is_notappl, pVarT, theIndex);

                    if( field_validation_event != nullptr )
                        field_validation_event->SetOperatorConfirmed(bAccepted);
                }
            }


            if( !bAccepted )
            {
                // keyed fields
                if( !pVarT->IsProtectedOrNoNeedVerif() )
                    m_pEngineDriver->GetSystemMessageManager().IncrementMessageCount(MGF::OperatorEnteredInvalidValue);

                // protected fields
                else
                {
                    // allow invalid values:
                    // - if the value was blank or out of range and that was allowed with confirmation
                    // - if in verification mode
                    // - if in a persistent field in operator controlled mode
                    bAccepted =
                        ( value_is_notappl && can_enter_notappl ) ||
                        ( !value_is_notappl && can_enter_out_of_range ) ||
                        ( m_pEntryDriver != nullptr && m_pEntryDriver->IsVerify() ) ||
                        ( m_pEngineDriver->m_EngineSettings.IsPathOff() && pVarT->IsPersistent() );

                    if( !bAccepted )
                    {
                        if( field_value.has_value() )
                            issaerror(MessageType::Abort, 1010, pVarT->GetName().c_str(), *field_value);

                        else
                            issaerror(MessageType::Abort, 1011, pVarT->GetName().c_str());
                    }
                }
            }
        }
    }

    // complete the field depending of it was accepted or not
    if( !bAccepted ) // not accepted...
    {
        if( !request_issued_in_on_refused )
        {
            // ... setup a request for "reenter to itself"
            SetLogicRequestNature(CsDriver::Reenter, true ); // RHF Apr 25, 2003 Add true for avoid persistent advance
            SetReenterTarget(pVarT); // RHF Jan 15, 2003
        }

        bRequestIssued = true;
    }

    else // accepted...
    {
        if( pVarT != nullptr )
        {
            if( pVarT->IsPersistent() )
                m_pEntryDriver->UpdatePersistentField(pVarT);

            else if( pVarT->IsAutoIncrement() )
                m_pEntryDriver->UpdateAutoIncrementField(pVarT);
        }
    }


    if( field_validation_event != nullptr )
        Paradata::Logger::LogEvent(field_validation_event);


    return bRequestIssued;
}

bool CsDriver::SomeIdCollision( void ) {
    // ... taken from wExentry/CheckIdCollision         // victor Jun 14, 01
    bool    bCollisionDetected = false;                 // victor Feb 21, 00
    bool    bIsPrimaryFlow     = ( GetFlow()->GetSubType() == SymbolSubType::Primary );

    if( !bIsPrimaryFlow )
        return bCollisionDetected;

    // scanning keys (initial and current)
    int     iSymDic = GetFlow()->GetSymDicAt( 0 );
    DICT*   pDicT   = DPT(iSymDic);
    DICX*   pDicX   = pDicT->GetDicX();
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
        // RHF INIC Jul 28, 2003
        if( m_pEntryDriver->IsPartial() ) {
            if( !bSameKey )
                bCollisionDetected = pDicX->GetDataRepository().ContainsCase(pszCurrentKey);
        }
        else
            bCollisionDetected = pDicX->GetDataRepository().ContainsCase(pszCurrentKey);
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
    }

    return bCollisionDetected;
}

int CsDriver::GetFieldColor( CFlowAtom* pAtom ) {       // victor Jun 14, 01
    int         iFieldColor = -1;       // assuming is not an item-atom
    CNDIndexes  theIndex;
    VARX*       pVarX = GetFieldFromAtom( pAtom, theIndex );

    if( pVarX != NULL )                 // process true items only
        iFieldColor = m_pIntDriver->GetFieldColor( pVarX, theIndex );

    return iFieldColor;
}

void CsDriver::SetFieldColorLow( CFlowAtom* pAtom ) {   // victor May 21, 01
    // SetFieldColorLow: set the color of a field to "low"
    CNDIndexes theIndex;
    VARX*      pVarX = GetFieldFromAtom( pAtom, theIndex );

    if( pVarX != NULL ) {               // process true items only
        CSettings*  pSettings = &m_pEngineDriver->m_EngineSettings;
        bool        bPathOn   = pSettings->IsPathOn();

        if( bPathOn )                   // PathOn: always becomes no-light
            m_pIntDriver->SetFieldColor( FLAG_NOLIGHT, pVarX, theIndex );
        else {                          // PathOff: becomes mid only if had color
            int     iFieldColor = m_pIntDriver->GetFieldColor( pVarX, theIndex );

            if( iFieldColor != FLAG_NOLIGHT )
                m_pIntDriver->SetFieldColor( FLAG_MIDLIGHT, pVarX, theIndex );
        }
    }
}

void CsDriver::SetFieldColorMid( CFlowAtom* pAtom ) {   // victor May 21, 01
    // SetFieldColorMid: set the color of a field to "mid"
    CNDIndexes theIndex;
    VARX*      pVarX = GetFieldFromAtom( pAtom, theIndex );

    if( pVarX != NULL )                 // process true items only
        m_pIntDriver->SetFieldColor( FLAG_MIDLIGHT, pVarX, theIndex );
}

void CsDriver::SetFieldColorHigh( CFlowAtom* pAtom ) {  // victor May 21, 01
    // SetFieldColorHigh: set the color of a field to "high"
    CNDIndexes  theIndex;
    VARX*       pVarX = GetFieldFromAtom( pAtom, theIndex );

    if( pVarX != NULL )                 // process true items only
        m_pIntDriver->SetFieldColor( FLAG_HIGHLIGHT, pVarX, theIndex );
}

void CsDriver::SetFieldColorNone( CFlowAtom* pAtom ) { // victor May 21, 01
    // SetFieldNoColor: wipes out the color of a field
    CNDIndexes  theIndex;
    VARX*       pVarX = GetFieldFromAtom( pAtom, theIndex );

    if( pVarX != NULL )                 // process true items only
        m_pIntDriver->SetFieldColor( FLAG_NOLIGHT, pVarX, theIndex );
}

VARX* CsDriver::GetFieldFromAtom( CFlowAtom* pAtom, CNDIndexes& theIndex ) { // victor May 21, 01
    // GetFieldFromAtom: get a field from a given atom and translates its indexes to engine' collapsed dimensions
    VARX*   pVarX = NULL;               // assuming not an item-atom
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    if( xAtomType == CFlowAtom::AtomType::Item ) {
        // get the field from the atom and translates atom' indexes to engine
        CFlowItem*  pFlowItem = (CFlowItem*) pAtom;
        int         iSymVar   = pFlowItem->GetSymbol();

        pVarX = VPX(iSymVar);

        theIndex = pVarX->PassIndexFrom3DToEngine( pFlowItem->GetIndex() );

        ASSERT( theIndex.isZeroBased() );
    }

    return pVarX;
}

VARX* CsDriver::GetFieldFrom3D( C3DObject* p3DObject, CNDIndexes& theIndex ) { // victor Jun 14, 01
    // GetFieldFrom3D: get a field from a given 3D-object and translates its indexes to engine
    VARX*   pVarX = NULL;               // assuming not an item-object

    // if no 3D-object provided, use current-object
    if( p3DObject == NULL )
        p3DObject = GetCurObject();

    if( p3DObject != NULL ) {
        int     iSymVar = p3DObject->GetSymbol();

        if( iSymVar > 0 && NPT(iSymVar)->IsA(SymbolType::Variable) ) {
            pVarX = VPX(iSymVar);

            theIndex = pVarX->PassIndexFrom3DToEngine( p3DObject->getIndexes() );
        }
    }

    return pVarX;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- adjusting bitmaps according to procedures attached to the atom
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::AdjustBitmap( byte* pByte, CFlowAtom* pAtom ) {
    // AdjustBitmap: adjust a given bitmap (for GroupHead/GroupTail/Item only)
    bool    bDone = ( pAtom != NULL );

    if( bDone ) {
        CFlowAtom::AtomType xAtomType  = pAtom->GetAtomType();
        int                 iSymbol = 0;

        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
                iSymbol    = ((CFlowGroupHead*) pAtom)->GetSymbol();
                AdjustGroupBitmap( pByte, iSymbol );
                break;

            case CFlowAtom::AtomType::GroupTail:
                iSymbol    = ((CFlowGroupTail*) pAtom)->GetSymbol();
                AdjustGroupBitmap( pByte, iSymbol );
                break;

            case CFlowAtom::AtomType::HTOcc:
                // remark - no adjust is needed
                break;

            case CFlowAtom::AtomType::Item:
                iSymbol    = ((CFlowItem*) pAtom)->GetSymbol();
                AdjustItemBitmap( pByte, iSymbol );
                break;

            default:
                ASSERT( 0 );            // can't be
                bDone = false;
                break;
        }
    }

    return bDone;
}

void CsDriver::AdjustGroupBitmap( byte* pByte, int iSymGroup )
{
    // AdjustGroupBitmap: reset in the given bitmap all bits without Proc in the given Group
    const GROUPT* pGroupT = GPT(iSymGroup);

    if( !pGroupT->HasProcIndex(ProcType::PreProc) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventPreProc );

    if( !pGroupT->HasProcIndex(ProcType::OnFocus) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventOnFocus );

    if( !pGroupT->HasProcIndex(ProcType::KillFocus) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventKillFocus );

    if( !pGroupT->HasProcIndex(ProcType::PostProc) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventPostProc );

    if( !pGroupT->HasProcIndex(ProcType::OnOccChange) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventOnOccChange );
}

void CsDriver::AdjustItemBitmap( byte* pByte, int iSymVar )
{
    // AdjustItemBitmap: reset in the given bitmap all bits without Proc in the given Var
    const VART* pVarT = VPT(iSymVar);

    if( !pVarT->HasProcIndex(ProcType::PreProc) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventPreProc );

    if( !pVarT->HasProcIndex(ProcType::OnFocus) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventOnFocus );

    if( !pVarT->HasProcIndex(ProcType::KillFocus) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventKillFocus );

    if( !pVarT->HasProcIndex(ProcType::PostProc) )
        ResetBitOfEventBitmap( pByte, CsDriver::EventPostProc );

    // ... remark OnOccChange is NOT possible for an Item-atom
    ResetBitOfEventBitmap( pByte, CsDriver::EventOnOccChange );
}



/////////////////////////////////////////////////////////////////////////////
//
// --- static bitmaps for flow-strip atoms
//
/////////////////////////////////////////////////////////////////////////////

CsDriver::byte CsDriver::GetStaticBitmap( void ) {
    // GetStaticBitmap: build and return the static bitmap for the current way and the current-atom in the flow-strip
    byte        cByte;
    CFlowAtom*  pAtom    = GetCurrAtom();
    bool        bForward = IsForwardWay();
    byte*       pBitmap  = ( bForward ) ? &m_aStaticForward[0] : &m_aStaticBackward[0];

    ResetBitmap( &cByte );

    if( pAtom != NULL ) {
        // extract preset bitmap
        CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();

        cByte = pBitmap[(int)xAtomType];
    }

    return cByte;
}

void CsDriver::BuildPresetStaticBitmaps( bool bForward ) {
    byte*   pBitmap  = ( bForward ) ? m_aStaticForward : m_aStaticBackward;
    int     iIniType = (int)CFlowAtom::AtomType::GroupHead;
    int     iTopType = (int)CFlowAtom::AtomType::Item;
    for( int iAtomType = iIniType; iAtomType <= iTopType; iAtomType++ ) {
        byte*   pByte = ( pBitmap + iAtomType );

        SeedStaticBitmap( pByte, bForward, (CFlowAtom::AtomType) iAtomType );
    }
}

bool CsDriver::SeedStaticBitmap( byte* pByte, bool bForward, CFlowAtom::AtomType xAtomType ) {
    bool    bDone = true;

    if( bForward ) {                    // forward bitmap
        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
                SetEventBitmap( pByte, 1, 1, 0, 0, 0, 0, 0 );
                break;

            case CFlowAtom::AtomType::GroupTail:
                SetEventBitmap( pByte, 0, 0, 0, 0, 1, 1, 0 );
                break;

            case CFlowAtom::AtomType::HTOcc:
                SetEventBitmap( pByte, 0, 0, 0, 1, 0, 0, 1 );
                break;

            case CFlowAtom::AtomType::BlockHead:
                SetEventBitmap( pByte, 1, 1, 0, 0, 0, 0, 0 );
                break;

            case CFlowAtom::AtomType::BlockTail:
                SetEventBitmap( pByte, 0, 0, 0, 0, 1, 1, 0 );
                break;

            case CFlowAtom::AtomType::Item:
                SetEventBitmap( pByte, 1, 1, 1, 1, 1, 1, 0 );
                break;

            default:
                bDone = false;
                break;
        }
    }
    else {                              // backward bitmap
        switch( xAtomType ) {
            case CFlowAtom::AtomType::GroupHead:
                SetEventBitmap( pByte, 0, 0, 0, 0, 1, 0, 0 );
                break;

            case CFlowAtom::AtomType::GroupTail:
                SetEventBitmap( pByte, 0, 1, 0, 0, 0, 0, 0 );
                break;

            case CFlowAtom::AtomType::HTOcc:
                SetEventBitmap( pByte, 0, 0, 0, 1, 0, 0, 1 );
                break;

            case CFlowAtom::AtomType::BlockHead:
                SetEventBitmap( pByte, 0, 1, 0, 0, 1, 0, 0 );
                break;

            case CFlowAtom::AtomType::BlockTail:
                SetEventBitmap( pByte, 0, 1, 0, 0, 0, 0, 0 );
                break;

            case CFlowAtom::AtomType::Item:
                SetEventBitmap( pByte, 0, 1, 1, 1, 1, 0, 0 );
                break;

            default:
                bDone = false;
                break;
        }
    }
    ASSERT( bDone );

    return bDone;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- dynamic bitmaps for flow-strip atoms
//
/////////////////////////////////////////////////////////////////////////////

CsDriver::byte CsDriver::GetDynamicBitmap( bool bIsLevelHeadOrTail ) {
    // GetDynamicBitmap: build and return the dynamic bitmap for the current way and the current-atom (and for the calculated condition) in the flow-strip
    // ... now takes fixed bitmaps for Level' head/tail // victor Jun 14, 01
    byte            cByte;
    CFlowAtom*      pAtom = GetCurrAtom();

    ResetBitmap( &cByte );

    if( pAtom != NULL ) {
        if( !bIsLevelHeadOrTail ) {     // normal atoms // victor Jun 14, 01
            // extract preset bitmap
            CFlowAtom::AtomType xAtomType      = pAtom->GetAtomType();
            RequestNature       xRequestNature = GetRequestNature();
            CSettings*          pSettings      = &m_pEngineDriver->m_EngineSettings;
            bool                bPathOn        = pSettings->IsPathOn();
            byte*               pBitmapPool    = ( bPathOn ) ? m_aDynamicPathOn : m_aDynamicPathOff;
            int                 iBitmap        = GetDynamicBitmapIndex( xRequestNature, (int)xAtomType );
            byte*               pBitmap        = pBitmapPool + iBitmap;
            int                 iCondition     = EvaluateCondition( bPathOn, xRequestNature );

            if( iCondition >= 0 )
                cByte = pBitmap[iCondition];
            else
                cByte = m_cFullBitmap;  // DEFAULT - everything active
        }                                               // victor Jun 14, 01
        else                                            // victor Jun 14, 01
            // Level' head/tails: the same as the static// victor Jun 14, 01
            cByte = GetStaticBitmap();                  // victor Jun 14, 01
    }

    return cByte;
}

int CsDriver::EvaluateCondition( bool bPathOn, RequestNature xRequestNature ) {
    int         iCondition        = -1; // assumes undefined condition
    CFlowAtom*  pAtom             = GetCurrAtom();
    CFlowAtom::AtomType xAtomType = pAtom->GetAtomType();
    CFlowCore*  pFlowCore         = GetFlowCore();
    int         iEndingLevel      = -1;
    int         iAtomLevel        = pFlowCore->GetAtomLevel( pAtom );
    bool        bDone             = true;

    // filtering RequestNature
    switch( xRequestNature ) {
        case AdvanceTo:
        case SkipTo:
        case Reenter:
        case AdvanceToNext:
        case SkipToNext:
        case LogicEndGroup:
        case LogicEndLevel:
        case NextField:
        case PrevField:
        case InterEndOccur:
            break;
        case InterEndGroup:
            xRequestNature = LogicEndGroup; // TEMPORARY
            break;
        case InterEndLevel:
            break;
        case EnterFlowEnd:                              // victor Jul 25, 01
            ASSERT( 0 );                // cannot arrive here!
            break;
        default:                        // unrecognized RequestNature
            bDone = false;
            break;
    }
    // ignoring None (advance to itself bug correction)
    ASSERT( xRequestNature == None || bDone );

    // filtering AtomType
    switch( xAtomType ) {
        case CFlowAtom::AtomType::GroupHead:
        case CFlowAtom::AtomType::GroupTail:
        case CFlowAtom::AtomType::HTOcc:
        case CFlowAtom::AtomType::BlockHead:
        case CFlowAtom::AtomType::BlockTail:
        case CFlowAtom::AtomType::Item:
            break;
        case CFlowAtom::AtomType::BeforeStrip:
        case CFlowAtom::AtomType::BeyondStrip:
        default:
            bDone = false;
            break;
    }

    // ignoring None (advance to itself bug correction)
    ASSERT( xRequestNature == None || bDone );

    if( !bDone )
        return iCondition;

    // for the analysis below, use iRelOrigin & iRelTarget as follows:
    //   1 - identical: current-atom identical to request-origin
    //   2 - external : current-atom external to request-origin
    //   3 - container: the Group of the request-origin is an ancestor of the current-atom
    //   4 - son      : the Group of the request-origin is a descendant of the current-atom
    // ... remark that "brother" comes converted to "external"
    C3DObject*  p3DObject  = GetCurObject();
    C3DObject*  p3DTarget  = NULL;
    C3DObject*  p3DOrigin  = ExtractInfoFromAtom( GetRequestOrigin() );
    int         iRelOrigin = EvaluateRelationship( p3DOrigin );
    int         iRelTarget = -1;
//~~int         iRelTarget = EvaluateRelationship( p3DTarget ); // EXAMPLE ONLY

    // calculating condition to be returned
    switch( xRequestNature ) {
        case AdvanceToNext:             // ... equivalent to AdvanceTo
        case AdvanceTo:                 // .................................
            p3DTarget = &m_TgtAdvanceTo;
            iRelTarget = EvaluateRelationship( p3DTarget );
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else
                        iCondition = 1; // any other HTOcc
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    break;

                case CFlowAtom::AtomType::Item:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( *p3DObject != *p3DTarget )
                        iCondition = 1; // any non Target-Item
                    else
                        iCondition = 2; // Target
                    break;
            }
            break;

        case SkipToNext:                // ... equivalent to SkipTo
        case SkipTo:                    // .................................
            p3DTarget = &m_TgtSkipTo;
            iRelTarget = EvaluateRelationship( p3DTarget );
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    if( iRelOrigin == 2 && iRelTarget == 2 )
                        iCondition = 0; // External
                    else if( iRelTarget == 1 )
                        iCondition = 1; // Target
                    else if( iRelTarget == 4 )
                        iCondition = 2; // Target Container
                    else
//                      ASSERT( 0 );    // SkipTo/GroupHead -> unknown condition
                        iCondition = 3; // any other GroupHead // confirmed by RHF // victor Mar 17, 02
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    if( iRelOrigin == 4 )
                        iCondition = 0; // Origin Container
                    else if( iRelOrigin == 2 && iRelTarget == 2 )
                        iCondition = 1; // External
                    else
//                      ASSERT( 0 );    // SkipTo/GroupTail -> unknown condition
                        iCondition = 2; // any other GroupTail // confirmed by RHF // victor Mar 17, 02
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( iRelTarget == 4 )
                        iCondition = 1; // Target Container
                    else if( iRelOrigin == 4 )
                        iCondition = 2; // Origin Container
                    else if( iRelOrigin == 2 && iRelTarget == 2 )
                        iCondition = 3; // External
                    else
                        iCondition = 4; // Any other HTOcc // RHF Mar 04, 2002
                    break;

                case CFlowAtom::AtomType::BlockHead:
                {
                    // skipping to the block
                    if( iRelTarget == 1 && SameBranch(p3DObject, p3DTarget) )
                        iCondition = 1;

                    // skipping to something on the block
                    else if( iRelTarget == 4 && SameBranch(p3DObject, p3DTarget) )
                        iCondition = 2;

                    // skipping the block completely
                    else
                        iCondition = 0;

                    break;
                }

                case CFlowAtom::AtomType::BlockTail:
                {
                    // skipping from something on the block
                    if( iRelOrigin == 4 && SameBranch(p3DOrigin, p3DObject) )
                        iCondition = 1;

                    // skipping the block completely
                    else
                        iCondition = 0;

                    break;
                }

                case CFlowAtom::AtomType::Item:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( *p3DObject != *p3DTarget )
                        iCondition = 1; // non-Target
                    else
                        iCondition = 2; // Target
                    break;
            }
            break;

        case Reenter:                   // .................................
            p3DTarget = &m_TgtReenter;
            iRelTarget = EvaluateRelationship( p3DTarget );
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( *p3DObject == *p3DTarget && iRelOrigin == 4 )
                        /*was 1*/           iCondition = 2; // Origin Container
                    else if( iRelOrigin == 4 )
    /**new**/           iCondition = 2; // Target & Origin Container // TODO check this, added // victor Mar 25, 02
                    else if( iRelOrigin == 2 && iRelTarget == 2 )
    /*was 2*/           iCondition = 3; // External
                    else if( *p3DObject == *p3DTarget )
    /*was 3*/           iCondition = 4; // Target
                    else if( iRelOrigin == 3 )
    /*was 4*/           iCondition = 5; // Son of origin // TODO should be provided by JOC // victor Dec 10, 01
                    else
// void Feb 16, 02      ASSERT( 0 );    // Reenter/GroupHead -> unknown condition
    /*was 5*/           iCondition = 6; // any other GroupHead // TODO should be provided by JOC // victor Feb 16, 02
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    // rewritten, rcl+JOC May 2005
                    if( iRelOrigin == 1 ||  // Origin
                        iRelOrigin == 3 )   // Origin Container
                    {
                        iCondition = 0; // Origin, will *not* happen
                        // 20131218 this can happen (if a reenter is executed in a group postproc, so i'm removing the following assert but leaving it on the desktop version)
                        //ASSERT(0);
                    }
                    // external to both origin and target
                    else if( /*iRelOrigin == 2 &&*/ iRelTarget == 2 ) // comparing iRelOrigin and 2 is superfluous
                        iCondition = 2;
                    else if( iRelTarget == 4 ||  // Target container
                             iRelTarget == 1  )  //
                        iCondition = 3;
                    else if( iRelTarget == 3 ) // Group Target is ancestor of current group
                        iCondition = 2; // bitmap chosen identical to External
                    else
                        ASSERT(0);
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0; // All cases will use the same Condition, JOC+rcl, May 2005
                                    // See SeedDynamicBitmap() method below, Reenter/HtOcc case
                    break;

                    ASSERT(0); // could never be here, everything handled by iCondition = 0 // rcl, May 2005
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( iRelTarget == 4 )
                        iCondition = 1; // Target Container
                    else if( iRelOrigin == 4 )
                        iCondition = 2; // Origin Container
                    else if( iRelOrigin == 2 && iRelTarget == 2 )
                        iCondition = 3; // External
                    else if( iRelOrigin == 3 )
                        iCondition = 4; // Son of origin // TODO should be provided by JOC // victor Dec 10, 01
                    else
                        ASSERT( 0 );    // Reenter/HTOcc -> unknown condition
                    break;

                case CFlowAtom::AtomType::BlockHead:
                {
                    // reentering the block
                    if( iRelTarget == 1 && SameBranch(p3DObject, p3DTarget) )
                        iCondition = 1;

                    // reentering something before the block while on the block
                    else if( ( iRelOrigin == 1 || iRelOrigin == 4 ) && SameBranch(p3DObject, p3DTarget) )
                        iCondition = 2;

                    else
                        iCondition = 0;

                    break;
                }

                case CFlowAtom::AtomType::BlockTail:
                {
                    // reentering the block or something on the block
                    if( ( iRelOrigin == 1 || iRelTarget == 4 ) && SameBranch(p3DOrigin, p3DObject) )
                        iCondition = 1;

                    else
                        iCondition = 0;

                    break;
                }

                case CFlowAtom::AtomType::Item:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( !p3DTarget->IsEmpty() && *p3DObject != *p3DTarget )
                        iCondition = 1; // except Target
                    else
                        iCondition = 2; // Target
                    break;
            }
            break;

        case LogicEndGroup:             // .................................
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( iRelOrigin == 3 )
                        iCondition = 1; // Son of Origin
                    else
                        iCondition = 2; // any other GroupHead
                    break;

                case CFlowAtom::AtomType::GroupTail:

                    // RHF INIC Feb 11, 2003
                    // Tail Found
                    if( p3DObject->GetSymbol() == p3DOrigin->GetSymbol() )
                        iRelOrigin = 1;
                    // RHF END Feb 11, 2003

                    if( iRelOrigin == 1 ) {
                        EventType  eType= GetRequestEventType();

                        if( eType == EventOnFocus )
                            iCondition = 0; // Origin, in OnFocus
                        else if( eType == EventPreProc ||
                                 eType == EventKillFocus  )
                            iCondition = 1; // Origin PreProc,KillFocus
                        else
                            iCondition = 4; // Use 0 bitmap
                    }
                    else if( iRelOrigin == 3 )
                        iCondition = 2; // Son of Origin
                    else if( iRelOrigin == 4 ) {
                        iCondition = 3; // Origin Container (only item)
                    }
                    else if( iRelOrigin == 2 )
                        iCondition = 4; // External
                    else
                        ASSERT( 0 );    // LogicEndGroup/GroupTail -> unknown condition
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( iRelOrigin == 4 )
                        iCondition = 1; // Origin Container
                    else
                        iCondition = 2; // any other HTOcc
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    break;

                case CFlowAtom::AtomType::Item:
                    if( iRelOrigin == 1 )
                        iCondition = 0; // Origin
                    else if( iRelOrigin == 2 )
                        // Notes: "Target" is the first field of an External Group
                        iCondition = 1; // Target
                    else
                        iCondition = 2; // any non Target-Item
                    break;
            }
            break;

        case LogicEndLevel:             // .................................
            iEndingLevel = pFlowCore->GetAtomLevel( GetRequestOrigin() );

            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any Item
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    if( iAtomLevel == iEndingLevel - 1 )
                        iCondition = 0; // Target-level' GroupTail
                    else
                        iCondition = 1; // any other GroupTail
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // any Item
                    break;
            }
            break;

        case NextField:                 // .................................
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    iCondition = 0;
                    break;
            }
            break;

        case PrevField:                 // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    iCondition = 0;
                    break;
            }
            break;

        case InterEndOccur:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    // Notes: solved by algorithm       // RHF May 14, 01
                    ASSERT( 0 );        // InterEndOccur -> impossible condition
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail (an Origin Container)
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // any HTOcc
                    break;

                case CFlowAtom::AtomType::Item:
                    if( iRelOrigin != 1 )
                        iCondition = 0; // any non Origin-Item
                    else {
                        // TODO - extract the value of the item and choose below
                        iCondition = 1; // Origin with value NOTAPPL
                        iCondition = 2; // Origin with value
                    }
                    break;
            }
            break;

        case InterEndGroup:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            bDone = false; // TODO undefined!
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    break;
            }
            break;

        case InterEndLevel:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            bDone = false; // TODO undefined!
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    break;
            }
            break;
    }
    ASSERT( bDone );
//  ASSERT( iCondition >= 0 );          // TEMPORARY uncomment once the evaluation complete

    return iCondition;
}

int CsDriver::EvaluateRelationship( C3DObject* p3DRefObject ) { // victor Jun 14, 01
    // EvaluateRelationship: get a simplified relationship between the current-object and a given reference-object
    C3DObject*  p3DCurObject = GetCurObject();
    int         iParenthood  = EvaluateParenthood( p3DCurObject, p3DRefObject );
    // For the analysis in EvaluateCondition, use iRelation as follows:
    //   1 - identical: current-atom identical to reference-atom
    //   2 - external : current-atom external to reference-atom
    //   3 - container: the Group of the reference-atom is an ancestor of the current-atom
    //   4 - son      : the Group of the reference-atom is a descendant of the current-atom
    // ... remark that "brother" is converted to "external"
    int     iRelation = iParenthood / 10;

    if( iRelation == 5 )
        iRelation = 2;                  // external

    return iRelation;
}

int CsDriver::EvaluateParenthood( C3DObject* p3DObsObject, C3DObject* p3DRefObject ) {
    // EvaluateParenthood: calculate the relationship of an observed-object against a reference-object
    // ... remark - the "reference" object will normally be the request-origin or the target
    //
    // Parenthood codes are as follows (2nd digit is reserved for future use, "contiguity")
    //   10 - identical: the reference-object and the observed-object are identical
    //   20 - external : no parenthood at all, objects are not related
    //   30 - container: the Group of the reference-object is an ancestor of the Group of the observed-object
    //   40 - son      : the Group of the reference-object is a descendant of the Group of the observed-object
    //   50 - brother  : the Group of the reference-object and the Group of the observed-object are equal
    int     iParenthood  = 0;       // assumes undefined parenthood
    bool    bNoObsObject = ( p3DObsObject == NULL || p3DObsObject->IsEmpty() );
    bool    bNoRefObject = ( p3DRefObject == NULL || p3DRefObject->IsEmpty() );

    if( bNoObsObject || bNoRefObject )
        iParenthood = 20;               // external
    else if( *p3DRefObject == *p3DObsObject )
        iParenthood = 10;               // identical
    else {
        // ... setup the observed' info
        int     iSymObs       = p3DObsObject->GetSymbol();
        SymbolType xSymObsType   = NPT(iSymObs)->GetType();
        bool    bObsIsGroup   = ( xSymObsType == SymbolType::Group );
        bool    bObsIsBlock   = ( xSymObsType == SymbolType::Block );
        bool    bObsIsItem    = ( xSymObsType == SymbolType::Variable );
        ASSERT( bObsIsGroup || bObsIsBlock || bObsIsItem );
        int     iSymObsGroup  = bObsIsGroup ? iSymObs : bObsIsBlock ? GetSymbolEngineBlock(iSymObs).GetGroupT()->GetSymbol() : VPT(iSymObs)->GetOwnerGroup();
        GROUPT* pObsGroupT    = GPT(iSymObsGroup);
        int     iObsLevel     = pObsGroupT->GetLevel();
        bool    bObsIsLevel   = ( (GROUPT::eGroupType) pObsGroupT->GetGroupType() == GROUPT::Level );

        // ... setup the reference' info
        int     iSymRef       = p3DRefObject->GetSymbol();
        SymbolType xSymRefType   = NPT(iSymRef)->GetType();
        bool    bRefIsGroup   = ( xSymRefType == SymbolType::Group );
        bool    bRefIsBlock   = ( xSymRefType == SymbolType::Block );
        bool    bRefIsItem    = ( xSymRefType == SymbolType::Variable );
        ASSERT( bRefIsGroup || bRefIsBlock || bRefIsItem );
        int     iSymRefGroup  = bRefIsGroup ? iSymRef : bRefIsBlock ? GetSymbolEngineBlock(iSymRef).GetGroupT()->GetSymbol() : VPT(iSymRef)->GetOwnerGroup();
        GROUPT* pRefGroupT    = GPT(iSymRefGroup);
        int     iRefLevel     = pRefGroupT->GetLevel();
        bool    bRefIsLevel   = ( (GROUPT::eGroupType) pRefGroupT->GetGroupType() == GROUPT::Level );

        // calculate the parenthood
        if( iObsLevel != iRefLevel )
            iParenthood = 20;           // external
        else if( bObsIsLevel && bRefIsLevel )
            iParenthood = 50;           // brother
        else if( bObsIsLevel )
            iParenthood = 40;           // son: reference is descendant of observed
        else if( bRefIsLevel )
            iParenthood = 30;           // container: reference is ancestor of observed
        else
        {
            // starting here, dealing with Groups, Blocks, and Items only
            CDimension::VDimType xObsDimType = pObsGroupT->GetDimType();
            CDimension::VDimType xRefDimType = pRefGroupT->GetDimType();
            bool                    bSameBranch = true; // TODO use the indexes to calculate this

            CNDIndexes aObsIndex = p3DObsObject->GetIndexes();
            CNDIndexes aRefIndex = p3DRefObject->GetIndexes();

            if( bObsIsBlock )
            {
                if( bRefIsBlock )
                {
                    if( iSymObsGroup == iSymRefGroup )
                        iParenthood = 50; // brother
                }

                else if( bRefIsGroup )
                {
                    if( iSymObsGroup == pRefGroupT->GetOwnerSymbol()  )
                        iParenthood = 50; // brother

                    else if( pObsGroupT->IsAncestor(iSymRefGroup, false) )
                        iParenthood = 30; // container: reference is ancestor of observed
                }

                else if( bRefIsItem )
                {
                    if( GetSymbolEngineBlock(iSymObs).ContainsField(iSymRef) )
                        iParenthood = 40; // son: reference is descendant of observed
                }
            }

            else if( bRefIsBlock )
            {
                if( bObsIsGroup )
                {
                    if( iSymRefGroup == pObsGroupT->GetOwnerSymbol()  )
                        iParenthood = 50; // brother

                    else if( iSymRefGroup == iSymObs || pRefGroupT->IsAncestor(iSymObsGroup, false) )
                        iParenthood = 40; // son: reference is descendant of observed
                }

                else if( bObsIsItem )
                {
                    if( GetSymbolEngineBlock(iSymRef).ContainsField(iSymObs) )
                        iParenthood = 30; // container: reference is ancestor of observed
                }
            }

            else if( bObsIsItem && bRefIsItem )
            {
                // 1) observed and reference are Item' objects
                if( iSymObsGroup == iSymRefGroup )
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 50;// brother
                }
            }

            else if( bObsIsGroup && bRefIsGroup )
            {
                // 2) observed and reference are Group' objects
                if( iSymObsGroup == iSymRefGroup )
                    iParenthood = 10;

                else if( pObsGroupT->GetOwnerSymbol() == pRefGroupT->GetOwnerSymbol() )
                    iParenthood = 50;   // brother

                else if( pRefGroupT->IsAncestor( iSymObsGroup, false ) )
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 40;// son: reference is descendant of observed
                }

                else if( pObsGroupT->IsAncestor( iSymRefGroup, false ) )
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 30;// container: reference is ancestor of observed
                }
            }

            else if( bObsIsGroup )
            {
                // 3) observed is a Group' object, reference is an Item' object
                if( pObsGroupT->GetOwnerSymbol() == iSymRefGroup )
                    iParenthood = 50;   // brother

                else if( iSymObsGroup == iSymRefGroup ) // victor Dec 10, 01
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 40;// son: reference is descendant of observed
                }

                else if( pRefGroupT->IsAncestor( iSymObsGroup, false ) )
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 40;// son: reference is descendant of observed
                }
            }

            else /* if( bRefIsGroup ) */
            {
                // 4) observed is an Item' object, reference is a Group' object
                if( pRefGroupT->GetOwnerSymbol() == iSymObsGroup )
                    iParenthood = 50;   // brother

                else if( iSymRefGroup == iSymObsGroup ) // victor Dec 10, 01
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 40;// container: reference is ancestor of observed
                }
                else if( pObsGroupT->IsAncestor( iSymRefGroup, false ) )
                {
                    // TODO calculate bSameBranch

                    if( bSameBranch )
                        iParenthood = 30;// container: reference is ancestor of observed
                }
            }
        }
    }

    // when parenthood has not been assigned...
    if( !iParenthood )
        iParenthood = 20;               // external

    return iParenthood;
}

void CsDriver::BuildPresetDynamicBitmaps( bool bPathOn ) {
    // * BitmapPool is either m_aDynamicPathOn or m_aDynamicPathOff
    // * BitmapPools are fixed-size, 3-axis array as follows:
    //     rows     one for each possible RequestNature
    //     cols     one for each AtomType
    //     layers   one for each condition-bitmap (up to m_iNumConditions, not all specified)
    // * the total size of BitmapPools (see constructor) is given by
    //
    //     m_iDynamicBitmapSize = ( m_iNumRequestNature * m_iNumAtomTypes ) * m_iNumConditions;
    //
    byte*               pBitmapPool = ( bPathOn ) ? m_aDynamicPathOn : m_aDynamicPathOff;
    int                 iRequestNature;
    RequestNature       xRequestNature;
    int                 iAtomType;
    CFlowAtom::AtomType xAtomType;
    byte*               pBitmap;

    for( iRequestNature = AdvanceTo; iRequestNature < m_iNumRequestNature; iRequestNature++ ) {
        xRequestNature = (RequestNature) iRequestNature;

        // no bitmaps required - solved by algorithm
        if( xRequestNature == EnterFlowEnd )            // victor Dec 10, 01
            continue;

        // excluding undefined requests // TEMPORARY    // victor May 14, 01
        switch( xRequestNature ) {      // TEMPORARY    // victor May 14, 01
            case LogicEndLevel:         // TEMPORARY    // victor May 14, 01
            case InterEndLevel:         // TEMPORARY    // victor May 14, 01
                continue;               // TEMPORARY    // victor May 14, 01
        }

        for( iAtomType = (int)CFlowAtom::AtomType::GroupHead; iAtomType < m_iNumAtomTypes; iAtomType++ ) {
            pBitmap   = pBitmapPool + GetDynamicBitmapIndex( xRequestNature, iAtomType );

            // each condition for the RequestNature/AtomType is specified by...
            SeedDynamicBitmaps( pBitmap, bPathOn, xRequestNature, (CFlowAtom::AtomType)iAtomType );
        }
    }
}

bool CsDriver::SeedDynamicBitmaps( byte* pBitmap, bool bPathOn, RequestNature xRequestNature, CFlowAtom::AtomType xAtomType ) {
    bool    bDone = true;

    // filtering RequestNature
    switch( xRequestNature ) {
        case AdvanceTo:
        case SkipTo:
        case Reenter:
        case AdvanceToNext:
        case SkipToNext:
        case LogicEndGroup:
        case LogicEndLevel:
        case EnterFlowEnd:                              // victor Jul 25, 01
        case NextField:
        case PrevField:
        case InterEndOccur:
        case InterEndGroup:
        case InterEndLevel:
            break;
        default:                        // unrecognized RequestNature
            bDone = false;
            break;
    }
    ASSERT( bDone );
    // filtering AtomType
    switch( xAtomType ) {
        case CFlowAtom::AtomType::GroupHead:
        case CFlowAtom::AtomType::GroupTail:
        case CFlowAtom::AtomType::HTOcc:
        case CFlowAtom::AtomType::BlockHead:
        case CFlowAtom::AtomType::BlockTail:
        case CFlowAtom::AtomType::Item:
            break;
        case CFlowAtom::AtomType::BeforeStrip:
        case CFlowAtom::AtomType::BeyondStrip:
        default:
            bDone = false;
            break;
    }
    ASSERT( bDone );

    if( !bDone )
        return bDone;

    // presetting bitmaps for each condition
    int     iCondition;

    switch( xRequestNature ) {
        case AdvanceToNext:             // ... equivalent to AdvanceTo
        case AdvanceTo:                 // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // any other HTOcc
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 1, 1, 1, 0 );

                    iCondition = 1;     // any non Target-Item
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 1, 1, 1, 0 );

                    iCondition = 2;     // Target
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case SkipToNext:                // ... equivalent to SkipTo
        case SkipTo:                    // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // Target
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );

                    iCondition = 2;     // Target Container
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );

                    iCondition = 3; // any other GroupHead // confirmed by RHF // victor Mar 17, 02
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );

                    iCondition = 1;     // External
                    // Notes: External to both, origin and target
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 2; // any other GroupTail  // confirmed by RHF // victor Mar 17, 02
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // Target Container
                    // Notes: Priority over next H/T (mutually exclusive)
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );

                    iCondition = 2;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 3;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 4;     // Any other HTOcc// RHF Mar 04, 2002
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                    iCondition = 0; // skipped over
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1; // skip to
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );

                    iCondition = 2; // skipped to something on block
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0; // skipped over
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1; // skipped from
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 1, 0, 0 );

                    iCondition = 1;     // non-Target
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );

                    iCondition = 2;     // Target
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case Reenter:                   // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

    /**new**/       iCondition = 1;     // Target & Origin Container // TODO check this, added // victor Mar 25, 02
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

    /*was 1*/       iCondition = 2;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );

    /*was 2*/       iCondition = 3;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

    /*was 3*/       iCondition = 4;     // Target
                    // SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 ); // OnFocus bit set on GroupTail atom type

    /*was 4*/       iCondition = 5;     // Son of origin // TODO should be provided by JOC // victor Dec 10, 01
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

    /*was 5*/       iCondition = 6;     // any other GroupHead // TODO should be provided by JOC // victor Feb 16, 02
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // Origin [will never happen]
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    // RHF INIC Mar 23, 2003
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                  // RHF END Mar 23, 2003

                    iCondition = 1;     // Origin container [will never happen]
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 2;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 3;     // Target, Target container
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );

                    // All other conditions are not important to be set/kept because
                    // all cases are handled by the Condition == 0 now
                    // See EvaluateCondition() method above, Reenter/HtOcc case.
                    // JOC+rcl, May 2005
                    break;

                    ASSERT(0);  // could never be here, now all cases are handled above // rcl, May 2005
                    iCondition = 1;     // Target Container
                    // Notes: Priority over next H/T (mutually exclusive)
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );

                    iCondition = 2;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );

                    iCondition = 3;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );
                    break;

                    iCondition = 4;     // Son of origin // TODO should be provided by JOC // victor Dec 10, 01
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                    iCondition = 0; // reentered something before while not on the block
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1; // reentered this block while on the block
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );

                    iCondition = 2; // reentered something before while on the block
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0; // reentered something before
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1; // reentered the block or something on the block
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 1, 0, 0 );
                    // RHF INIC Mar 23, 2003. When executed reenter from PostProc must execute OnFocus!!!!
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 1, 1, 0, 0 );
                    // RHF END Mar 23, 2003

                    iCondition = 1;     // except Target
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );

                    iCondition = 2;     // Target
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case LogicEndGroup:             // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // Son of Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 2;     // any other GroupHead
                    // RHF COM Feb 11, 2003 SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 ); // RHF Feb 11, 2003
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // Origin, in OnFocus
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );

                    iCondition = 1;     // Origin in PreProc, KillFocus
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 1, 0 );

                    iCondition = 2;     // Son Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 3;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );

                    iCondition = 4;     // External
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // Origin Container
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 2;     // any other HTOcc
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // Origin
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 1, 0, 0 );

                    iCondition = 1;     // Target
                    // Notes: "Target" is the first field of an External Group
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );

                    iCondition = 2;     // any non Target-Item
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );
                    break;
            }
            break;

        case LogicEndLevel:             // .................................
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // target-level' GroupTail
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );

                    iCondition = 1;     // any other GroupTail
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // any HTOcc
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // any Item
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;
            }
            break;

        case EnterFlowEnd:              // .................................
//          bDone = false; // TODO undefined!
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    break;
            }
            break;

        case NextField:                 // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // any HTOcc
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // any Item (the only one)
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case PrevField:                 // .................................
            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // any GroupHead
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // any HTOcc
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockTail:
                    iCondition = 0;
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // any Item (the only one)
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case InterEndOccur:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    // Notes: solved by algorithm       // RHF May 14, 01
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // any GroupTail (an Origin Container)
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // any HTOcc
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 1 );
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // any non Origin-Item
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );

                    iCondition = 1;     // Origin with value NOTAPPL
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 1, 0, 0 );

                    iCondition = 2;     // Origin with value
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 1, 1, 0 );
                    break;
            }
            break;

        case InterEndGroup:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            // ... same for PathOn and PathOff          // JOC+RHF May 13, 01
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                    iCondition = 0;     // Group Head External
                    // Notes: It has to be a Group next to the origin
                    SetEventBitmap( pBitmap + iCondition, 0, 1, 1, 0, 0, 0, 0 );

                    iCondition = 1;     // Group Head (Son)
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::GroupTail:
                    iCondition = 0;     // Group Tail Org. Container
                    // Notes: *(PostProc) Find out if EndGroup origin is PreProc of Grp.
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 1, 1, 0 );
                    break;

                case CFlowAtom::AtomType::HTOcc:
                    iCondition = 0;     // H/T Occ Tar. Owner Grp.
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 0, 0, 0, 0, 0 );

                    iCondition = 1;     // H/T Occ Org. Owner Grp
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );

                    iCondition = 2;     // H/T Occ Org. External Grp
                    // Notes: The same as target or adjacent group
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 0, 0, 0, 0 );
                    break;

                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                    break;

                case CFlowAtom::AtomType::Item:
                    iCondition = 0;     // Item (except Target)
                    SetEventBitmap( pBitmap + iCondition, 0, 0, 0, 1, 0, 0, 0 );

                    iCondition = 1;     // Item (Target)
                    // Notes: First field after External Group
                    SetEventBitmap( pBitmap + iCondition, 1, 1, 1, 1, 1, 1, 0 );
                    break;
            }
            break;

        case InterEndLevel:             // .................................
            if( bPathOn )
                break;                  // not allowed for PathOn

            bDone = false; // TODO undefined!
            switch( xAtomType ) {
                case CFlowAtom::AtomType::GroupHead:
                case CFlowAtom::AtomType::GroupTail:
                case CFlowAtom::AtomType::HTOcc:
                case CFlowAtom::AtomType::BlockHead:
                case CFlowAtom::AtomType::BlockTail:
                case CFlowAtom::AtomType::Item:
                    break;
            }
            break;
    }
    ASSERT( bDone );

    return bDone;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- elementary bitmap operations
//
/////////////////////////////////////////////////////////////////////////////

void CsDriver::SetupBitMasks( void ) {                  // victor May 10, 01
    // - definitions for bit operations (restricted to 1-byte only)
    //          m_aBitMaskOn;           // 128| 64| 32| 16|  8|  4|  2|  1
    m_aBitMaskOn [CsDriver::EventPreProc    ] = 128;
    m_aBitMaskOn [CsDriver::EventOnFocus    ] =  64;
    m_aBitMaskOn [CsDriver::EventInterface  ] =  32;
    m_aBitMaskOn [CsDriver::EventRefresh    ] =  16;
    m_aBitMaskOn [CsDriver::EventKillFocus  ]  =   8;
    m_aBitMaskOn [CsDriver::EventPostProc   ]  =   4;
    m_aBitMaskOn [CsDriver::EventOnOccChange]  =   2;
    m_aBitMaskOn [CsDriver::EventNone       ]  =   1;
    //          m_aBitMaskOff;          // 127|191|223|239|247|251|253|254
    m_aBitMaskOff[CsDriver::EventPreProc    ]  = 127;
    m_aBitMaskOff[CsDriver::EventOnFocus    ]  = 191;
    m_aBitMaskOff[CsDriver::EventInterface  ]  = 223;
    m_aBitMaskOff[CsDriver::EventRefresh    ]  = 239;
    m_aBitMaskOff[CsDriver::EventKillFocus  ]  = 247;
    m_aBitMaskOff[CsDriver::EventPostProc   ]  = 251;
    m_aBitMaskOff[CsDriver::EventOnOccChange]  = 253;
    m_aBitMaskOff[CsDriver::EventNone       ]  = 254;

    // constant full-bitmap, all 1's for defined event-types
    m_cFullBitmap = 255;                // 1111 1111
}

void CsDriver::SetEventBitmap( byte* pByte, bool bEventPreProc, bool bEventOnFocus, bool bEventInterface, bool bEventRefresh, bool bEventKillFocus, bool bEventPostProc, bool bEventOnOccChange ) {
    ResetBitmap( pByte );
    if( bEventPreProc )
        SetBitOfEventBitmap( pByte, CsDriver::EventPreProc );

    if( bEventOnFocus )
        SetBitOfEventBitmap( pByte, CsDriver::EventOnFocus );

    if( bEventInterface )
        SetBitOfEventBitmap( pByte, CsDriver::EventInterface );

    if( bEventRefresh )
        SetBitOfEventBitmap( pByte, CsDriver::EventRefresh );

    if( bEventKillFocus )
        SetBitOfEventBitmap( pByte, CsDriver::EventKillFocus );

    if( bEventPostProc )
        SetBitOfEventBitmap( pByte, CsDriver::EventPostProc );

    if( bEventOnOccChange )
        SetBitOfEventBitmap( pByte, CsDriver::EventOnOccChange );

    // set the EventNone to mark "loaded bitmap"
    SetBitOfEventBitmap( pByte, CsDriver::EventNone );
}



/////////////////////////////////////////////////////////////////////////////
//
// --- progress bitmap (event-procs execution)          // victor Aug 08, 01
//
/////////////////////////////////////////////////////////////////////////////

bool CsDriver::IsProgressBeforePreProc( void ) {
    byte    cProgress  = GetProgressBitmap();
    bool    bPreProcOn = GetBitOfEventBitmap( &cProgress, CsDriver::EventPreProc );
    return bPreProcOn;
}

bool CsDriver::IsProgressBeforeOnFocus( void ) {
    byte    cProgress  = GetProgressBitmap();
    bool    bOnFocusOn = GetBitOfEventBitmap( &cProgress, CsDriver::EventOnFocus );
    return bOnFocusOn;
}

bool CsDriver::IsProgressBeforeInterface( void ) {
    return( IsProgressBeforePreProc() || IsProgressBeforeOnFocus() );
}


void CsDriver::SetProgressForPreEntrySkip() // 20130415 so that skips launched from onchar/onkey/userbar treat the current field as unkeyed
{
    SetProgressBitmapOn(EventOnFocus);
}


/////////////////////////////////////////////////////////////////////////////
//
// --- saving/restoring environment for Enter-flow actions // victor Jul 25, 01
//
/////////////////////////////////////////////////////////////////////////////

void CsDriver::SaveEnvironmentInfo()
{
    // SaveEnvironmentInfo: saves current values of the environment for Enter-flow actions

    // info saved to allow later returning to this flow
    m_ProgType = m_pIntDriver->m_iProgType;
    m_ExLevel = m_pIntDriver->m_iExLevel;
    m_ExSymbol = m_pIntDriver->m_iExSymbol;
}

void CsDriver::RestoreEnvironmentInfo()
{
    // RestoreEnvironmentInfo: restores saved values of the environment for Enter-flow actions

    // info saved to allow later returning to this flow
    m_pIntDriver->m_iProgType = m_ProgType;
    m_pIntDriver->m_iExLevel = m_ExLevel;
    m_pIntDriver->m_iExSymbol = m_ExSymbol;
}


void CsDriver::SetEnterFlowLogicStack(const LogicStackSaver& logic_stack_saver)
{
    m_enterFlowLogicStack = std::make_unique<LogicStackSaver>(logic_stack_saver);
}

void CsDriver::ClearEnterFlowLogicStack()
{
    m_enterFlowLogicStack.reset();
}

bool CsDriver::RunEnterFlowLogicStack()
{
    // m_iSkipStmt & m_iStopExec were changed in 'exenter' to break execution,
    // and this reinitialization is needed here
    m_pIntDriver->m_iSkipStmt = FALSE;
    m_pIntDriver->m_iStopExec = FALSE;

    // process pending statements stored in the enter stack
    bool bRequestIssued = false;

    auto logic_stack_saver = std::move(m_enterFlowLogicStack);

    if( logic_stack_saver != nullptr )
    {
        try
        {
            while( !logic_stack_saver->Empty() )
            {
                int statement = logic_stack_saver->PopStatement();

                bRequestIssued = m_pIntDriver->ExecuteProgramStatements(statement);

                // this is copied from RunEventProcs and seems to allow the proper processing of
                // skips that occur in the same procedure as the enter statement; e.g.:
                //      enter EXT_FF;
                ///     skip to VALUE;
                if( bRequestIssued )
                {
                    SetRequestEventType(static_cast<EventType>(m_pIntDriver->m_iProgType));
                    SetRequestOrigin();
                }

                if( m_pIntDriver->m_iStopExec )
                    break;
            }
        }

        catch( const ExitProgramControlException& ) { }
    }

    return bRequestIssued;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- interacting with the flow-strip                  // victor Dec 10, 01
//
/////////////////////////////////////////////////////////////////////////////

CFlowAtom* CsDriver::GetCurrAtom( void ) {
    // GetCurrAtom: return the current atom in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    CFlowAtom*  pAtom = ( m_pFlowCore != NULL ) ? m_pFlowCore->FlowStripCurr() : NULL;

    return pAtom;
}

CFlowAtom* CsDriver::GetNextAtom( void ) {
    // GetNextAtom: return the next atom in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    CFlowAtom*  pAtom = ( m_pFlowCore != NULL ) ? m_pFlowCore->FlowStripNext() : NULL;

    return pAtom;
}

CFlowAtom* CsDriver::GetPrevAtom( void ) {
    // GetPrevAtom: return the previous atom in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    CFlowAtom*  pAtom = ( m_pFlowCore != NULL ) ? m_pFlowCore->FlowStripPrev() : NULL;

    return pAtom;
}

CFlowAtom* CsDriver::GetAtomAt( int iAtom ) {
    // GetAtomOfIndex: return the atom at given index in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    CFlowAtom*  pAtom = NULL;
    int         iCurrAtom = GetCurrAtomIndex(); // save the current-atom

    if( iAtom >= 0 ) {
        m_pFlowCore->FlowStripSetCurrIndex( iAtom );
        pAtom = GetCurrAtom();
    }
    SetCurrAtomIndex( iCurrAtom );      // restore the current-atom

    return pAtom;
}

int CsDriver::GetCurrAtomIndex( void ) {
    // GetCurrAtomIndex: return the index of the current atom in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    int         iCurrAtom = ( m_pFlowCore != NULL ) ? m_pFlowCore->FlowStripGetCurrIndex() : -1;

    return iCurrAtom;
}

int CsDriver::GetCurrAtomLevel( void ) {
    // GetCurrAtomLevel: return the level of the current atom in the flow-strip
    ASSERT( m_pFlowCore != NULL );
    CFlowAtom*  pAtom = GetCurrAtom();
    int         iCurrAtomLevel = ( m_pFlowCore != NULL ) ? m_pFlowCore->GetAtomLevel( pAtom ) : -1;

    return iCurrAtomLevel;
}

void CsDriver::SetCurrAtomIndex( int iAtom ) {
    // SetCurrAtomIndex: set the index of the current atom in the flow-strip to a given atom number
    ASSERT( m_pFlowCore != NULL );

    if( m_pFlowCore != NULL )
        m_pFlowCore->FlowStripSetCurrIndex( iAtom );
}

void CsDriver::RestartFlow( void ) {
    // RestartFlow: restart the flow-strip
    ASSERT( m_pFlowCore != NULL );

    if( m_pFlowCore != NULL )
        m_pFlowCore->FlowStripRestart();
}



/////////////////////////////////////////////////////////////////////////////
//
// --- engine links
//
/////////////////////////////////////////////////////////////////////////////

void CsDriver::SetEngineDriver( CEngineDriver* pEngineDriver, CEntryDriver* pEntryDriver )
{
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea   = pEngineDriver->m_pEngineArea;
    m_pIntDriver    = pEngineDriver->m_pIntDriver.get();
    m_pEntryDriver  = pEntryDriver;
    m_engineData    = pEntryDriver->m_engineData;
}



/////////////////////////////////////////////////////////////////////////////
//
// --- TRANSITION-TO-3D
//
/////////////////////////////////////////////////////////////////////////////

void CsDriver::SetCurDeFld( bool bFullDim ) {
    // SetCurDeFld: pass current 3D-object to the equivalent m_CurDeFld (only if a Var)
    C3DObject*  p3DObject = GetCurObject();
    DEFLD3*     pDeFld = &m_CurDeFld;

    PassFrom3DToDeFld( p3DObject, pDeFld, true );  // multi-index version
//    PassFrom3DToDeFld( p3DObject, pDeFld, false ); // 1-index version
}

DEFLD3* CsDriver::GetCurDeFld( void ) {
//  SetCurDeFld( true );                // multi-index version
    SetCurDeFld( false );               // 1-index version

    DEFLD3* pDeFld = ( m_CurDeFld.GetSymbol() ) ? &m_CurDeFld : NULL;

    return pDeFld;
}

bool CsDriver::PassLowestFieldToCurObject( void ) {     // victor Dec 10, 01
    CFlowCore*  pFlowCore = m_pEngineDriver->GetPrimaryFlow()->GetFlowCore();
    CFlowAtom*  pAtom     = pFlowCore->GetLowestFieldAtom();
    ASSERT( pAtom->GetAtomType() == CFlowAtom::AtomType::Item );
    bool    bDone = ( pAtom->GetAtomType() == CFlowAtom::AtomType::Item );
    if( bDone )
        SetCurObject( ExtractInfoFromAtom( pAtom ) );

    return bDone;
}

bool CsDriver::C_IsAutoEndGroup() {
    //SAVY Here check for path off as we want this to happen only in path off
    CSettings*          pSettings       = &m_pEngineDriver->m_EngineSettings;
    bool                bPathOff        = pSettings->IsPathOff();
    bool bAutoEndGroup = pSettings->IsAutoEndGroup();
    int  iActiveLevel   = m_pEntryDriver->GetActiveLevel();
    if (iActiveLevel > 0){
        bAutoEndGroup = !m_pEntryDriver->IsAddedNode( iActiveLevel - 1 );
    }
    return bAutoEndGroup;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CEntryrunView::ProcessFldAttrib(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CsDriver::ProcessSequentialFld(VART* pVarT)
{
    //SAVY Jul 30 ,2003 for sequential update in partial add mode
    //SAVY 2/6/2012 - Also in a two level app  check for new node
    bool bIsModification = m_pEntryDriver->IsModification() && m_pEntryDriver->GetPartialMode() != ADD_MODE && !m_pEntryDriver->GetEntryIFaz()->C_IsNewNode();

    if(!bIsModification &&  pVarT->GetDictItem()->GetContentType() == ContentType::Numeric && pVarT->IsSequential() ) {
        CDEField* pField = NULL;
        int iMaxDEField = -1;
        CDEGroup* pGroup = pVarT->GetOwnerGPT()->GetCDEGroup();
        if(pGroup->GetMaxDEField()){
            iMaxDEField = getGroupMaxOccsUsingMaxDEField(pVarT->GetOwnerGPT());
        }
        for(int iIndex =0; iIndex < pGroup->GetNumItems();iIndex++) {
            pField = (CDEField*)pGroup->GetItem(iIndex);
            if(pField->IsMirror())
                continue;
            if(pField->GetDictItem() == pVarT->GetDictItem()) {
                break;
            }
            pField = NULL; //reset;
        }
        if(!pField)
            return;

        VARX*   pVarX = pVarT->GetVarX();

        int iOcc=pField->GetRuntimeOccurrence();
        if( iOcc <= 0 )
            iOcc = pGroup->GetCurOccurrence();
        if(iMaxDEField != -1 && iOcc > iMaxDEField){
            return;//We have exceeded the max allowed for this controlled group
        }
        CNDIndexes theIndex( ZERO_BASED );
        pVarX->BuildIntParentIndexes( theIndex, iOcc < 1 ? 1 : iOcc );       // TRANSITION ?

        CString sData = pVarX->GetValue(theIndex);

        if( true ) {
            sData.Trim();
            if(sData.IsEmpty()) {

                CDEGroup* pGroup = pVarT->GetOwnerGPT()->GetCDEGroup();
                while(pGroup) {
                    if(pGroup->GetMaxLoopOccs() != 1) // See if it is multiple
                        break;
                    else  {
                        pGroup = pGroup->GetParent();
                    }
                }
                if(!pGroup)
                    return;
                //int iIndex = pGroup->GetCurOccurrence(); //probably can use iOccur
                int iIndex = iOcc;
                if(iIndex > 1) {//get previous val
                    CNDIndexes theIndex( ZERO_BASED );
                    pVarX->BuildIntParentIndexes( theIndex, iIndex - 1 < 1 ? 1 : iIndex - 1 );       // TRANSITION ?
                    CString sIndexData = pVarX->GetValue(theIndex);
                    int iVal = _ttoi(sIndexData);
                    if(iVal)
                        iIndex = iVal+1;
                }

                sData = IntToString(iIndex);

                pField->SetData( sData ); //probably can use this to set the data than doing a putval

#ifdef WIN_DESKTOP
                AfxGetApp()->GetMainWnd()->PostMessage(WM_IMSA_SETSEQUENTIAL, (WPARAM)pField ,(LPARAM)0 );
#endif
                if(pField->IsProtected()) {
                    csprochar*   pszVarvalue = sData.GetBuffer(128);
                    C_FldPutVal(pVarT,pszVarvalue );
                    sData.ReleaseBuffer();
                }
                //in the case of protected field you may have to do a putval 'cos you dont get the control
                //on the client side
            }
        }
    }
}


void CsDriver::C_FldPutVal( VART* pVarT, csprochar* pszNewVarvalue )
{
    if( pVarT == NULL )
        return;

    CDEField* pField = NULL;
    CDEGroup* pGroup = pVarT->GetOwnerGPT()->GetCDEGroup();
    for(int iIndex =0; iIndex < pGroup->GetNumItems();iIndex++) {
        pField = (CDEField*)pGroup->GetItem(iIndex);
        if(pField->IsMirror())
            continue;
        if(pField->GetDictItem() == pVarT->GetDictItem()) {
            break;
        }
        pField = NULL; //reset;
    }
    if(!pField)
        return;

    VARX*   pVarX = pVarT->GetVarX();

    int iOccur=pField->GetRuntimeOccurrence();
    if( iOccur <= 0 )
        iOccur = pGroup->GetCurOccurrence();

    int     iVarLen = pVarT->GetLength();
    int     iVarDec = pVarT->GetDecimals();
    csprochar*   pszVarvalue = (csprochar*) calloc( iVarLen + 1, sizeof(csprochar) );

    CNDIndexes theIndex( ZERO_BASED, DEFAULT_INDEXES );
    pVarX->BuildIntParentIndexes( theIndex, iOccur );             // TRANSITION ?

    // copy the new-value to pszVarValue
    strcpymax( pszVarvalue, pszNewVarvalue, iVarLen );
    pszVarvalue[iVarLen] = 0;           // unnecessary because calloc

    // get buffer ascii and float
    CIntDriver* pIntDriver = m_pEngineDriver->m_pIntDriver.get();
    csprochar*       pszAsciiAddr = pIntDriver->GetVarAsciiAddr( pVarX, theIndex );
    ASSERT( pszAsciiAddr );

    _tmemset( pszAsciiAddr, _T(' '), iVarLen );

    if( pVarT->IsNumeric() ) {
        const CDictItem* pItem = pVarT->GetDictItem();
        bool        bIsBlank = ( trimright(pszVarvalue) == 0 );
        bool        bZeroFill = ( pItem && pItem->GetZeroFill() );

        csprochar* pszAuxVarvalue = (csprochar*) calloc( iVarLen + 1, sizeof(csprochar) );
        _tmemset( pszAuxVarvalue, ( bZeroFill && !bIsBlank ) ? _T('0') : _T(' '), iVarLen );

        // RHF INIC Jul 27, 2000
        if( _tcslen(pszVarvalue) > 0 && _tcschr( _T("+-"), pszVarvalue[0] ) != NULL ) {    // BMD  07 Aug 2000
            memcpyright( pszAuxVarvalue, iVarLen, pszVarvalue + 1, _tcslen(pszVarvalue+1) );
            pszAuxVarvalue[0] = pszVarvalue[0];
        }
        else {
            // RHF END Jul 27, 2000
            memcpyright( pszAuxVarvalue, iVarLen, pszVarvalue, _tcslen(pszVarvalue) );
            _tmemcpy( pszVarvalue, pszAuxVarvalue, iVarLen );
        }// RHF Jul 27, 2000
        free( pszAuxVarvalue );
    }

    _tmemcpy( pszAsciiAddr, pszVarvalue, _tcslen(pszVarvalue) );

    if( pVarT->IsNumeric() ) {
        double      dValue      = chartodval( pszVarvalue, iVarLen, iVarDec );
        double*     pDoubleAddr = pIntDriver->GetVarFloatAddr( pVarX, theIndex );
        ASSERT( pDoubleAddr );

        *(pDoubleAddr) = pVarX->varinval( dValue, theIndex );

        // Prepare value for display in dValue
        dValue = pVarX->varoutval( theIndex );
        // RHF COM Aug 22, 2000 pVarT->dvaltochar( dValue, pszVarvalue );
        pVarT->dvaltochar( dValue, pszAsciiAddr );      // RHF Aug 22, 2000

        if( pVarX->iRelatedSlot >= 0 )
            pVarX->VarxRefreshRelatedData( theIndex );    // RHF 12/8/99
    }

    free( pszVarvalue );
}

// RHF INIC Jan 15, 2003 Fix protected id bug. Test in verify
void CsDriver::SetReenterTarget( VART* pVarT ) {
    Set3DTarget( NULL );

    if( !pVarT->IsProtectedOrNoNeedVerif() )
        return;

    bool    bIsIdField=false;
    int     iSymPrevIdNoProtected=0; // The id that is not protected
    int     iLevel = GetLevelOfCurObject();
    int     iSymItem = pVarT->GetSymbolIndex();
    int     iSymVar;
    for( int iItem = 0; !bIsIdField && ( iSymVar = m_pEntryDriver->QidVars[iLevel - 1][iItem] ) > 0; iItem++ ) {
        VART* pIdVarT=VPT(iSymVar);
        if( !pIdVarT->IsProtectedOrNoNeedVerif() )
            iSymPrevIdNoProtected = iSymVar;

        bIsIdField = ( iSymVar == iSymItem );
    }

    C3DObject   o3DTarget;

    if( bIsIdField && iSymPrevIdNoProtected > 0 ) {
        CNDIndexes aIndex( ONE_BASED, DEFAULT_INDEXES );

        o3DTarget.SetSymbol( iSymPrevIdNoProtected );
        o3DTarget.setIndexes( aIndex );

        Set3DTarget( &o3DTarget );
    }
    else
        Set3DTarget( NULL );
}
// RHF END Jan 15, 2003

// SAVY 12/03
CString CsDriver::FldGetVal(const CDictItem* pItem)
{
    DEFLD3 DeField3;
    DEFLD  DeFld;

    DeFld.SetSymbol( pItem->GetSymbol() );
    DeFld.useOnlyOneDimension(1);

    m_pEntryDriver->MakeField3( &DeField3, &DeFld );// RHF Jul 31, 2000

    VART* pVarT = VPT(DeField3.GetSymbol());
    VARX* pVarX = pVarT->GetVarX();

    CNDIndexes theIndex = DeField3.getIndexes();

    CString csValue = pVarX->GetValue(theIndex);
    TCHAR* pszVarValue = csValue.GetBuffer();

    // fix for interface
    if( pVarT->IsNumeric() && pVarT->GetDecimals() > 0 && *pszVarValue == '*' )
    {
        int iDecPos = pVarT->GetLength() - pVarT->GetDecimals() - 1;
        pszVarValue[iDecPos] = _T('.');
    }

    return csValue;
}
// RHF END Jul 31, 2000

// RHF INIC Sep 30, 2004
void CsDriver::TrimAllOcc( int iEndingLevel ) {
    CFlowCore*  pFlowCore       = GetFlowCore();
    CFlowAtom*  pAtom           = GetCurrAtom();

    while( 1 ) { // GetCurrAtomIndex() < iStopAtomIndex ) {
        if( pFlowCore->GetAtomLevel( pAtom ) != iEndingLevel )
            break;
            if( pAtom->GetAtomType() == CFlowAtom::AtomType::BeyondStrip )
                break;

            if( pAtom->GetAtomType() == CFlowAtom::AtomType::GroupTail ) {
                bool        bIsLevelTail = pFlowCore->IsLevelTail( pAtom );
            if( bIsLevelTail )
                break;

            GroupTrimOcc(); // takes care of empty occurrences
        }
        pAtom = GetNextAtom();
    }
}
// RHF END Sep 30, 2004

//////////////////////////////////////////////////////////////////////////
void CsDriver::SetCurObject( C3DObject* p3DObject )
{
    if( p3DObject == 0 )
        m_CurObject.SetEmpty();
    else
        m_CurObject = *p3DObject;
}

void CsDriver::setExtractedObject( int iSymbol ) // rcl Jun 28, 2004
{
    m_ExtractedObject.SetEmpty();
    m_ExtractedObject.SetSymbol( iSymbol );
}

void CsDriver::setExtractedObject( int iSymbol, C3DIndexes& the_Indexes ) // rcl Jun 28, 2004
{
    m_ExtractedObject.SetSymbol( iSymbol );
    m_ExtractedObject.setIndexes( the_Indexes );
}
