//---------------------------------------------------------------------------
//
//  intDEntr: interpreting Data-Entry exclusive commands
//
//---------------------------------------------------------------------------
//  File name: IntDEntr.cpp
//
//  Description:
//          Implementation for Entry' interpreter-driver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              10 Nov 99   RHF     Basic conversion
//              07 Apr 00   vc      Now the skip-target can be either a Var or Group
//              03 Aug 00   RHF     Fix problem with endsect command
//              18 Aug 00   RHF     Fine tunning of skip, advance without target and reenter itself
//              08 Mar 01   vc      Adding support for executing selected ENTRY commands in BATCH
//              14 Mar 01   vc      Adding BatchExScanOccur & BatchExDisplayDirty for modularity
//              20 Mar 01   vc      Adding BatchExAdvance & BacthExEndLevel
//              26 Mar 01   vc      Full remake to deal with @target (or 'At' reference) for skip-to/skip-to-next commands
//                                  (requires change to VARX::PassTheOnlyIndex)
//              16 May 01   vc      Expanding for CsDriver
//              25 Jul 01   vc      Tailoring for Enter-flow in CsDriver
//              10 Dec 01   vc      Isolating CsDriver operation
//              10 May 05   rcl     Groups allowed in advance and reenter [for entry]
//
//---------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "EXENTRY.H"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Block.h>
#include <zEngineO/WorkString.h>
#include <zMessageO/Messages.h>
#include <Zissalib/CsDriver.h>
#include <Zissalib/CFlAdmin.h>
#include <zUtilO/TraceMsg.h>


namespace
{
    void GetIndexArrayWithNewOccurrence(UserIndexesArray dIndex, const VART* pVarT, GROUPT* pGroupT, int iNewOcc)
    {
        // used by skip and ask to get the index of a variable but with a new occurrence
        initUserIndexArray(dIndex);

        // initialize the dIndex with currentO3dObject indexes
        int number_dimensions = ( pVarT != nullptr ) ? pVarT->GetNumDim() : pGroupT->GetNumDim();

        for( int i = 0; i < number_dimensions; i++ )
            dIndex[i] = pGroupT->GetCurrent3DObject().getIndexes().getIndexValue(i);

        // set the correct index with the new occ that we should skip to
        // as per the logic in exskip, when using next  - refer to login ::exskip
        if( number_dimensions - 1 >= 0 )
            dIndex[number_dimensions - 1] = iNewOcc;
    }
}

//--------------------------------------------------------
//  exskipto: executes an skip to
//--------------------------------------------------------

double CIntDriver::exskipto( int iExpr ) {
    // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001

    if( Issamod != ModuleType::Entry )
        return BatchExSkipTo( iExpr );                  // victor Mar 08, 01

    m_iSkipStmt = TRUE;                    // to break the current proc

    // --- target can be either a Var or Group          // victor Apr 07, 00
    SKIP_NODE*  pSkipNode = (SKIP_NODE*) (PPT(iExpr));

    // solving 'At' reference if any                    // victor Mar 26, 01
    if( pSkipNode->m_iSymAt || pSkipNode->m_iAlphaExpr ) // 20120325 added second condition
        return( EntryExSkipToAt( iExpr ) );             // victor Mar 26, 01

    bool bSkipToNext = ( pSkipNode->var_exprind == SKIP_TO_NEXT_OCCURRENCE );

    ASSERT( IsUsing3D_Driver() );

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);

    int         iSymTarget;

    if( bSkipToNext ) // SKIP TO NEXT
    {
        iSymTarget = pSkipNode->var_ind;

        GROUPT* pGroupT = m_pEngineArea->GetGroupTOfSymbol(iSymTarget);

        int iOccTarget = // transition
            dIndex[0] = pGroupT->GetCurrentOccurrences() + 1;

        // probably dIndex should be filled with information coming from
        // pGroupT->GetCurrent3DObject().
        // RTODO: Remove this comment
        CNDIndexes theCurrentIndexes = pGroupT->GetCurrent3DObject().getIndexes();

        if( iOccTarget /*dIndex[0]*/ > pGroupT->GetMaxOccs() ) {
            GetCsDriver()->SetLogicRequestNature( CsDriver::LogicEndGroup ); // SkipToNext
            SetRequestIssued();
            return 0;
        }
    }
    else {
        SVAR_NODE* pSVarNode = (SVAR_NODE*) (PPT(pSkipNode->var_ind));
        MVAR_NODE* pMVarNode = (MVAR_NODE*) pSVarNode;
        GRP_NODE*  pGrpNode  = (GRP_NODE*) pSVarNode;

        if( pSVarNode->m_iVarType == SVAR_CODE ) {
            iSymTarget = pSVarNode->m_iVarIndex;
            dIndex[0] = 1;
        }
        else if( pMVarNode->m_iVarType == MVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            mvarGetSubindexes( pMVarNode, dIndex );
        }
        else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes(GetSymbolEngineBlock(iSymTarget).GetGroupT(), pGrpNode, dIndex);
        }
        else {                          // GRP_NODE
            ASSERT(pMVarNode->m_iVarType == GROUP_CODE);
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes( pGrpNode, dIndex );
        }

        // there was a bug whereby skipping to the next field (A->B) in a postproc left A marked as skipped
        // while logic was run (only to be marked as filled once logic stopped); so if the skip target is
        // the next field, advance to it instead
        if( ( m_iProgType == PROCTYPE_KILLFOCUS || m_iProgType == PROCTYPE_POST ) &&
            NPT(m_iExSymbol)->IsA(SymbolType::Variable) && NPT(iSymTarget)->IsA(SymbolType::Variable) )
        {
            GROUPT* pCurrentGroupT = m_pEngineArea->GetGroupTOfSymbol(m_iExSymbol);

            if( pCurrentGroupT == m_pEngineArea->GetGroupTOfSymbol(iSymTarget) )
            {
                int iCurOcc = pCurrentGroupT->GetCurrentExOccurrence();
                int iCurItem = pCurrentGroupT->GetItemIndex(m_iExSymbol);
                int iNewOcc = 0;
                int iNewItem = 0;

                int iSymNewItem = pCurrentGroupT->SearchNextItemInGroup(iCurOcc, iCurItem, &iNewOcc, &iNewItem);

                if( iSymTarget == iSymNewItem )
                {
                    bool occurrence_matches = true;

                    if( pMVarNode->m_iVarType == MVAR_CODE )
                    {
                        UserIndexesArray dNewItemIndex;
                        GetIndexArrayWithNewOccurrence(dNewItemIndex, VPT(m_iExSymbol), pCurrentGroupT, iNewOcc);
                        occurrence_matches = ( memcmp(dIndex, dNewItemIndex, sizeof(dIndex)) == 0 );
                    }

                    if( occurrence_matches )
                        return exadvance(iExpr);
                }
            }
        }
    }

    m_pCsDriver->PassTo3D( &o3DTarget, iSymTarget, dIndex );

    // set request & target
    m_pCsDriver->SetLogicRequestNature( CsDriver::SkipTo );
    m_pCsDriver->Set3DTarget( &o3DTarget );

    bool    bFailed=!m_pCsDriver->IsValidSkipTarget();
    /* RHF COM INIC Dec 04, 2003  Use this code when you want to go to the next field when there is a fail
    // check the target - if invalid, set an innocuous NextField
    if( bFailed) {
        m_pCsDriver->ResetRequest();
        m_pCsDriver->SetInterRequestNature( CsDriver::RequestNature::NextField );
    }
    SetRequestIssued();
    RHF COM END Dec 04, 2003 */

    // RHF INIC Dec 04, 2003 Use this code when you don't want to go to the next field when there is a fail
    if( bFailed )
        m_pCsDriver->ResetRequest();
    else
        SetRequestIssued();
    // RHF END Dec 04, 2003


    return 0;
}

//--------------------------------------------------------
//  exadvance : executes advance to a var
//--------------------------------------------------------

double CIntDriver::exadvance( int iExpr ) {
    // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001

    if( Issamod != ModuleType::Entry )
        return BatchExAdvance( iExpr );                 // victor Mar 20, 01

    m_iSkipStmt = TRUE;                // to break the current proc

    SKIP_NODE*  pAdvanceNode  = (SKIP_NODE*) (PPT(iExpr));

    // solving 'At' reference if any                    // victor Mar 26, 01
    if( pAdvanceNode->m_iSymAt || pAdvanceNode->m_iAlphaExpr )    // victor Mar 26, 01; 20120521 added second condition
        return( EntryExAdvanceToAt( iExpr ) );             // victor Mar 26, 01

    bool        bInfinite = ( pAdvanceNode->var_ind <= 0 ); // ADVANCE no target // RHF Aug 18, 2000 change from == 0 to <= 0
    SVAR_NODE*  pSVarNode;
    MVAR_NODE*  pMVarNode;
    GRP_NODE*   pGrpNode;

    ASSERT( IsUsing3D_Driver() );

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;
    int         iSymTarget;

    initUserIndexArray(dIndex);

    if( bInfinite )
        o3DTarget.SetEmpty();
    else {
        pSVarNode = (SVAR_NODE*) (PPT(pAdvanceNode->var_ind));
        pMVarNode = (MVAR_NODE*) pSVarNode;
        pGrpNode  = (GRP_NODE*) pSVarNode;

        if( pSVarNode->m_iVarType == SVAR_CODE ) {
            iSymTarget = pSVarNode->m_iVarIndex;
            dIndex[0] = 1;
        }
        else if( pMVarNode->m_iVarType == MVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            mvarGetSubindexes( pMVarNode, dIndex );
        }
        else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes(GetSymbolEngineBlock(iSymTarget).GetGroupT(), pGrpNode, dIndex);
        }
        else
        {
            ASSERT( pMVarNode->m_iVarType == GROUP_CODE );
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes( pGrpNode, dIndex );
        }

        m_pCsDriver->PassTo3D( &o3DTarget, iSymTarget, dIndex );
    }

    // set request & target
    m_pCsDriver->SetLogicRequestNature( CsDriver::AdvanceTo );
    m_pCsDriver->Set3DTarget( &o3DTarget );

    bool    bFailed=!m_pCsDriver->IsValidAdvanceTarget();
    /* RHF COM INIC Dec 04, 2003 Use this code when you want to go to the next field when there is a fail
    // check the target - if invalid, set an innocuous NextField
    if( bFailed  ) {
        m_pCsDriver->ResetRequest();
        m_pCsDriver->SetInterRequestNature( CsDriver::RequestNature::NextField );
    }
    SetRequestIssued();
    RHF COM END Dec 04, 2003 */

    // RHF INIC Dec 04, 2003 Use this code when you don't want to go to the next field when there is a fail

    // Note: The above comment is not true // rcl, Jul 21, 2004
    //       when there is a fail, it goes to the next field anyway
    if( bFailed )
        m_pCsDriver->ResetRequest();
    else
        SetRequestIssued();
    // RHF END Dec 04, 2003

    return 0;
}

//--------------------------------------------------------
//  exreenter : executes reenter of a var
//--------------------------------------------------------

double CIntDriver::exreenter( int iExpr ) {
   // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001

    if( Issamod != ModuleType::Entry )
        return BatchExReenter( iExpr );                 // victor Mar 08, 01

    m_iSkipStmt = TRUE;                    // to break the current proc

    SKIP_NODE*  pReenterNode = (SKIP_NODE*) (PPT(iExpr));

    // solving 'At' reference if any                    // victor Mar 26, 01
    if( pReenterNode->m_iSymAt || pReenterNode->m_iAlphaExpr ) // victor Mar 26, 01; 20120521 added second condition
        return( EntryExReenterToAt( iExpr ) );             // victor Mar 26, 01

    bool        bToItself = ( pReenterNode->var_ind <= 0 );
    SVAR_NODE*  pSVarNode;
    MVAR_NODE*  pMVarNode;
    GRP_NODE*   pGrpNode;

    ASSERT( IsUsing3D_Driver() );

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);

    if( bToItself ) {
        // set request
        m_pCsDriver->SetInterRequestNature( CsDriver::Reenter, true );// RHF Oct 17, 2002 Add true
        o3DTarget.SetEmpty();
        m_pCsDriver->Set3DTarget( &o3DTarget ); // RHF Dec 22, 2003
        SetRequestIssued();
    }
    else {
        pSVarNode = (SVAR_NODE*) PPT( pReenterNode->var_ind );
        pMVarNode = (MVAR_NODE*) pSVarNode;
        pGrpNode  = (GRP_NODE*) pSVarNode;

        int         iSymTarget;

        if( pSVarNode->m_iVarType == SVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            dIndex[0] = 1;
        }
        else if( pMVarNode->m_iVarType == MVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            mvarGetSubindexes( pMVarNode, dIndex );
        }
        else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes(GetSymbolEngineBlock(iSymTarget).GetGroupT(), pGrpNode, dIndex);
        }
        else { // GRP_NODE
            // rcl, May 2005
            ASSERT( pMVarNode->m_iVarType == GROUP_CODE );
            iSymTarget = pGrpNode->m_iGrpIndex;
            // grpGetSubindexes( pGrpNode, dIndex );
            // Patch to get to 1st occurrence of roster or group
            // rcl, May 2005
            /**/
            for( int i = 0; i < DIM_MAXDIM; i++ )
                dIndex[i] = 0;
                    /**/
        }

                // when using multiple-vars will use else (complete) logic
        // RHF INIC Dec 22, 2003
        if( iSymTarget == m_iExSymbol && pMVarNode->m_iVarType != BLOCK_CODE && pMVarNode->m_iVarType != GROUP_CODE && ( ( pMVarNode->m_iVarType != MVAR_CODE ) // itself (and not multiple var)
            || ( VPT(iSymTarget)->GetParentGPT()->GetCurrentExOccurrence() == dIndex[0] ) ) ) // 20130306 (m_iVarType == MVAR_CODE) we don't want to set a target if the occurrence is the same
        {
            // set request
            m_pCsDriver->SetInterRequestNature( CsDriver::Reenter, true );// RHF Oct 17, 2002 Add true
            o3DTarget.SetEmpty();
            m_pCsDriver->Set3DTarget( &o3DTarget );
            SetRequestIssued();
        }
        // RHF END Dec 22, 2003
        else { // RHF Dec 22, 2003

            m_pCsDriver->PassTo3D( &o3DTarget, iSymTarget, dIndex );


            // set request & target
            m_pCsDriver->SetLogicRequestNature( CsDriver::Reenter, true );// RHF Oct 17, 2002 Add true
            m_pCsDriver->Set3DTarget( &o3DTarget );

            bool     bFailed=!m_pCsDriver->IsValidReenterTarget();
            /* RHF COM INIC Dec 04, 2003  Use this code when you want to go to the next field when there is a fail
            // check the target - if invalid, set an innocuous NextField
            if( bFailed  ) {
            m_pCsDriver->ResetRequest();
            m_pCsDriver->SetInterRequestNature( CsDriver::RequestNature::NextField );
            }
            SetRequestIssued();
            RHF COM END Dec 04, 2003 */

            // RHF INIC Dec 04, 2003 Use this code when you don't want to go to the next field when there is a fail
            if( bFailed )
                m_pCsDriver->ResetRequest();
            else
                SetRequestIssued();
            // RHF END Dec 04, 2003
        }
    }

    return 0;
}

//--------------------------------------------------------
//  exnoinput : marks noinput for a field
//--------------------------------------------------------
double CIntDriver::exnoinput( int iExpr ) {
   // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001

    if( Issamod != ModuleType::Entry )
        return 0;

    ASSERT( IsUsing3D_Driver() );

    m_pCsDriver->EnableNoInput();                       // victor Dec 10, 01

    return 0;
}

//--------------------------------------------------------
//  exendsect : marks end of section
//--------------------------------------------------------
double CIntDriver::exendsect( int iExpr ) {
    // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001

// RHF INIC Dec 04, 2003
    bool    bValid=true;
    // Not valid in Group (Kill/Post) & in Level PROCS
    if( NPT(m_iExSymbol)->IsA(SymbolType::Group) &&
        (m_iProgType == PROCTYPE_KILLFOCUS || m_iProgType == PROCTYPE_POST) ||
        NPT(m_iExSymbol)->IsA(SymbolType::Group) &&  GPT(m_iExSymbol)->GetGroupType() == 1 ) { // Level
        bValid = false;
    }

    if( !bValid ) {
        issaerror( MessageType::Error, 9190 );
        return 0;
    }
// RHF END Dec 04, 2003


    if( Issamod != ModuleType::Entry )
        return BatchExEndsect( iExpr );                 // victor Mar 08, 01

    m_iSkipStmt = TRUE;                    // to break the current proc

    ASSERT( IsUsing3D_Driver() );
    m_pCsDriver->SetLogicRequestNature( CsDriver::LogicEndGroup );
    SetRequestIssued();

    return 0;
}

//--------------------------------------------------------
//  exendlevl : marks end of level
//--------------------------------------------------------
double CIntDriver::exendlevl( int iExpr ) {
   // RHF INIC Nov 02, 2001
    if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        return 0;
    }
    // RHF END Nov 02, 2001
    if( m_iExSymbol > 0 ) { // MAY 18, 2004
        Symbol* pSymbol = NPT(m_iExSymbol);
        // Level 0 killFocus/postproc not allowed
        if( pSymbol->IsA(SymbolType::Group) && ((GROUPT*) pSymbol)->GetLevel() == 0 &&
            (m_iProgType == PROCTYPE_KILLFOCUS || m_iProgType == PROCTYPE_POST)
            )
        {
            issaerror( MessageType::Error, 9194 );
            return DEFAULT;
        }

        // RHF INIC Dec 22, 2003

        if( pSymbol->IsA(SymbolType::Group) && ((GROUPT*) pSymbol)->GetSubType() != SymbolSubType::Primary
            ||
            pSymbol->IsA(SymbolType::Variable) && ((VART*) pSymbol)->GetSubType() != SymbolSubType::Input ) {
                issaerror( MessageType::Error, 9192);
                return DEFAULT;
            }
            // RHF END Dec 22, 2003
    }// MAY 18, 2004

    if( Issamod != ModuleType::Entry )
        return BatchExEndLevel( iExpr );                // victor Mar 20, 01

    m_iSkipStmt = TRUE;                    // to break the current proc

    ASSERT( IsUsing3D_Driver() );

    m_pCsDriver->SetLogicRequestNature( CsDriver::LogicEndLevel );
    SetRequestIssued();

    return 0;
}

CString m_csSkipStructMsg;

    // ------------------------------------------------
    // executing selected ENTRY commands in BATCH
    // ------------------------------------------------
    //     ... exskipto  is switched to BatchExSkipTo   // victor Mar 08, 01
    //     ... exadvance is switched to BatchExAdvance  // victor Mar 20, 01
    //     ... exreenter is switched to BatchExReenter  // victor Mar 08, 01
    //     ... exendsect is switched to BatchExEndsect  // victor Mar 08, 01
    //     ... exendsect is switched to BatchExEndLevel // victor Mar 20, 01
    // ------------------------------------------------
    //
    // iSkipType is as follows:
    //     0   generic 'skip to'
    //     1   skip to Group
    //     2   skip to next Var
    //     3   skip to Var
    //     4   skip to Var(i)
    //     5   skip to Var(i) and 'i' is invalid
    //     6   endsect
    //     6   endlevel
    //
    // ------------------------------------------------

double CIntDriver::BatchExSkipTo( int iExpr ) {         // victor Mar 08, 01
    // BatchExSkipTo: check that every after-the-source and before-the-target are blank

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    m_iSkipStmt = TRUE;                    // to break the current proc

    SKIP_NODE*  pSkipNode   = (SKIP_NODE*) (PPT(iExpr));

    // solving 'At' reference if any                    // victor Mar 26, 01
    if( pSkipNode->m_iSymAt )                           // victor Mar 26, 01
        return( BatchExSkipToAt( iExpr ) );             // victor Mar 26, 01

    bool        bSkipToNext = ( pSkipNode->var_exprind == SKIP_TO_NEXT_OCCURRENCE );
    int         iSkipType   = 0;        // 0-generic 'skip to'

    // get the source                   // enhanced
    int         iSymSource      = m_iExSymbol;                   // victor Apr 19, 01
    SymbolType eType = NPT(iSymSource)->GetType(); // victor Apr 19, 01
    ASSERT( eType == SymbolType::Group || eType == SymbolType::Block || eType == SymbolType::Variable );                     // victor Apr 19, 01
    int iSymSourceGroup =
        ( eType == SymbolType::Group ) ? iSymSource :
        ( eType == SymbolType::Block ) ? GetSymbolEngineBlock(iSymSource).GetGroupT()->GetSymbol() :
        VPT(iSymSource)->GetOwnerGroup();

    int         iOccSource      = GPT(iSymSourceGroup)->GetCurrentExOccurrence();
    bool        bSeeSource      = ( m_iProgType == PROCTYPE_PRE || m_iProgType == PROCTYPE_ONFOCUS );
    int         iProgSource     = m_iProgType;

    if( iOccSource < 1 )
        iOccSource = 1;

    // get the target
    int         iSymTarget      = 0;
    int         iOccTarget      = 1;
    int         iProgTarget     = PROCTYPE_PRE;

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);

    if( bSkipToNext ) {                  // SKIP TO NEXT
        iSymTarget = pSkipNode->var_ind;
        GROUPT* pGroupT =
            ( NPT(iSymTarget)->IsA(SymbolType::Group) ) ? GPT(iSymTarget) :
            ( NPT(iSymTarget)->IsA(SymbolType::Block) ) ? GetSymbolEngineBlock(iSymTarget).GetGroupT() :
            VPT(iSymTarget)->GetOwnerGPT();

        iOccTarget = dIndex[0] = pGroupT->GetCurrentExOccurrence() + 1;

        iSkipType = 2;                  // ... to next Var


         // RHF INIC May 24, 2002 Fix problem skip to next in batch (when max occ is reached)
        // if( iOccTarget > m_pEngineArea->GroupMaxNumOccs( iSymTargetGroup ) ) {
        if( iOccTarget > pGroupT->GetMaxOccs() ) {
            return BatchExEndsect( iExpr );
        }
        // RHF END May 24, 2002

        // RHF INIC Oct 08, 2003 Fix problem skip to next in batch (when data occ is reached). This
        // happens when in Entry the variable that generate occurrence has an ENDGROUP and the ocurrence
        // is not generated in the data file. In Batch, skip to next reach an occurrence greater than DataOcc
        //if( iOccTarget > GPT(iSymTargetGroup)->GetDataOccurrences() ) {
        if( iOccTarget > pGroupT->GetDataOccurrences() ) {
            bool    bShowWarning=false;

            if( bShowWarning ) {
                if( pGroupT->GetMaxOccs() == 1 )
                    issaerror( MessageType::Warning, 88225, NPT(iSymTarget)->GetName().c_str(), pGroupT->GetDataOccurrences() );
                else
                    issaerror( MessageType::Warning, 88226, NPT(iSymTarget)->GetName().c_str(), iOccTarget, pGroupT->GetDataOccurrences()  );
            }

            return BatchExEndsect( iExpr );
        }
        // RHF END Oct 08, 2003

        CNDIndexes  theCurrentIndexes( ONE_BASED );
        if( NPT(iSymTarget)->IsA(SymbolType::Group) )
            GetCurrentGroupSubIndexes( iSymTarget, theCurrentIndexes );
        else if( NPT(iSymTarget)->GetType() == SymbolType::Block )
            GetCurrentGroupSubIndexes( GetSymbolEngineBlock(iSymTarget).GetGroupT()->GetSymbol(), theCurrentIndexes );
        else
            GetCurrentVarSubIndexes( iSymTarget, theCurrentIndexes );

        for( int i=0; i < DIM_MAXDIM; i++ ) {
            dIndex[i] = theCurrentIndexes.getIndexValue(i);
        }

        CDimension::VDimType vType = pGroupT->GetDimType();
        ASSERT( vType == CDimension::Record || vType == CDimension::Item ||
            vType == CDimension::SubItem );
        dIndex[vType]++;

        ASSERT( dIndex[vType] == iOccTarget );
    }
    else {                              // SKIP TO
        SVAR_NODE*  pSVarNode = (SVAR_NODE*) (PPT(pSkipNode->var_ind));
        MVAR_NODE*  pMVarNode = (MVAR_NODE*) pSVarNode;
        GRP_NODE*   pGrpNode  = (GRP_NODE*) pSVarNode;

        if( pSVarNode->m_iVarType == SVAR_CODE ) {
            iSymTarget = pSVarNode->m_iVarIndex;
            iSkipType = 3;              // ... to Var
            dIndex[0] = 1;
        }
        else if( pMVarNode->m_iVarType == MVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            mvarGetSubindexes( pMVarNode, dIndex );
            iOccTarget = (int) dIndex[0];       // ***TRANSITION***xxx
            iSkipType = 4;              // ... to Var(i)
        }
        else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes(GetSymbolEngineBlock(iSymTarget).GetGroupT(), pGrpNode, dIndex);
            iSkipType = INT_MAX; // unused
        }
        else {                          // GRP_NODE
            ASSERT(pMVarNode->m_iVarType == GROUP_CODE);
            iSymTarget = pGrpNode->m_iGrpIndex;
            grpGetSubindexes( pGrpNode, dIndex );
            iSkipType = 1;              // ... to Group
        }
    }

    if( bSkipToNext ) { // dIndex contains full 3d dimensions
        o3DTarget.SetEmpty();
        o3DTarget.SetSymbol( iSymTarget );
        for( int iDim=0; iDim < DIM_MAXDIM; iDim++ )
            o3DTarget.setIndexValue( iDim, (int) dIndex[iDim] );
        o3DTarget.getIndexes().setAsInitialized();

        //CsDriver::PassTo3D( &o3DTarget, NPT(iSymTarget), dIndex );
    }
    else // mvarGetSubindexes get collapsed dims.
    CsDriver::PassTo3D( &o3DTarget, NPT(iSymTarget), dIndex );

    // setup the skipping-flag info for the engine
    // BatchExSetSkipping( iSymSource, iOccSource, iProgSource, iSymTarget, iOccTarget, iProgTarget ); // victor Mar 26, 01

    // definitivo:
    // BatchExSetSkipping( iSymSource, iOccSource, iProgSource, o3DTarget, iProgTarget ); // rcl, Sep 04, 04


    C3DObject   o3DSourceDummy;

    BatchExSetSkipping( o3DSourceDummy, iProgSource, o3DTarget, iProgTarget );

    // 88200 %d inconsistent fields detected following a 'skip to' command in %p
    m_csSkipStructMsg.Format( MGF::GetMessageText(88190).c_str(), bSkipToNext ? _T("SKIP TO NEXT") : _T("SKIP"), ProcName().GetString() );

    return 0;
}

double CIntDriver::BatchExAdvance( int iExpr ) {        // victor Mar 20, 01
    // BatchExAdvance: just set the flag to break the current proc

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    SKIP_NODE*  pReenterNode   = (SKIP_NODE*) (PPT(iExpr));
    int         iSymAt      = pReenterNode->m_iSymAt;
    if( iSymAt > 0 )  {
        int     iOccDummy;
        bool    bExplicitOccDummy;
        GetReferredReenterAdvanceTargetSymbol( iSymAt, true, false, &iOccDummy, &bExplicitOccDummy );
    }
    // RHF END Nov 24, 2003

    m_iSkipStmt = TRUE;                // to break the current proc

    return 0;
}

double CIntDriver::BatchExReenter( int iExpr ) {        // victor Mar 08, 01
    // BatchExReenter: warn of 'reenter' command arised

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    m_iSkipStmt = TRUE;                    // to break the current proc

    SKIP_NODE*  pReenterNode = (SKIP_NODE*) (PPT(iExpr));


    // RHF INIC Nov 24, 2003
    // solving 'At' reference if any
    if( pReenterNode->m_iSymAt )  {
        issaerror( MessageType::Error, 88230 );
        return 0;
    }
    // RHF END Nov 24, 2003


    int         iSymTarget   = 0;
    int         iOccTarget   = 0;       // ***TRANSITION***
    bool        bVarTarget   = true;
    bool        bMulTarget   = false;

    if( pReenterNode->var_ind > 0 ) {   // REENTER with parameter
        SVAR_NODE*  pSVarNode = (SVAR_NODE*) PPT( pReenterNode->var_ind );
        MVAR_NODE*  pMVarNode = (MVAR_NODE*) pSVarNode;
        double      dIndex[DIM_MAXDIM];

        if( pSVarNode->m_iVarType == SVAR_CODE )
            iSymTarget = pSVarNode->m_iVarIndex;
        else if( pMVarNode->m_iVarType == MVAR_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            mvarGetSubindexes( pMVarNode, dIndex );
            iOccTarget = (int) dIndex[0];       // ***TRANSITION***
            bMulTarget = true;
        }
        else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
            iSymTarget = pMVarNode->m_iVarIndex;
            const EngineBlock& engine_block = GetSymbolEngineBlock(iSymTarget);
            GROUPT* pGroupT = engine_block .GetGroupT();
            grpGetSubindexes(pGroupT, (GRP_NODE*)pMVarNode, dIndex);
            iOccTarget = (int)dIndex[0];        // ***TRANSITION***
            bMulTarget = ( pGroupT->GetMaxOccs() > 1 );
        }
        else { // GRP_NODE
            ASSERT(pMVarNode->m_iVarType == GROUP_CODE);
            bVarTarget = false;
            ASSERT(0); // 'reenter' cannot (yet) have a Group-target
        }
    }

    CString csTargetName = ( iSymTarget != 0 ) ? WS2CS(NPT(iSymTarget)->GetName()) : CString();

    if( !iSymTarget || csTargetName.IsEmpty() )
        issaerror( MessageType::Error, 88230 );
    else if( !bMulTarget )
        issaerror( MessageType::Error, 88231, csTargetName.GetString() );
    else
        issaerror( MessageType::Error, 88232, csTargetName.GetString(), iOccTarget );

    return 0;
}


double CIntDriver::BatchExEndsect( int iExpr ) {        // victor Mar 08, 01
    // BatchExEndsect: check that every after-the-source and in-same-group are blank

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    m_iSkipStmt = TRUE;                    // to break the current proc

    C3DObject   o3DSourceDummy;
    int         iSymSource      = m_iExSymbol;
    int         iProgSource     = m_iProgType;
    int         iProgTarget     = PROCTYPE_POST;

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);

    CsDriver::PassTo3D( &o3DTarget, NPT(iSymSource), dIndex );

    o3DTarget.SetSymbol(-iSymSource); // don't use dIndex, stop in next group that is not my son


    BatchExSetSkipping( o3DSourceDummy, iProgSource, o3DTarget, iProgTarget );

    // 88200 %d inconsistent fields detected following a 'skip to' command in %p
    m_csSkipStructMsg.Format( MGF::GetMessageText(88190).c_str(), _T("ENDGROUP"), ProcName().GetString() );

    return 0;
}


double CIntDriver::BatchExEndLevel( int iExpr ) {       // victor Mar 20, 01

    // BatchExEndLevel: check that every after-the-source and in-same-level are blank
    // TODO - once completed, erase these lines <begin>
    if( m_pEngineSettings->HasSkipStruc() ) {
        // RHF COM Nov 08, 2001 issaerror( MessageType::Error, 88207, 0 );
    }
    return 0;
    // TODO - once completed, erase these lines <end>

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    m_iSkipStmt = TRUE;                    // to break the current proc

    int     iSkipType = 7;              // 6-EndLevel

    // TODO - review & tailor everyting below
    // get the source
    int         iSymSource      = m_iExSymbol;
    int         iOccSource      = 0;
    int         iSymSourceGroup = 0;
    bool        bSeeSource      = false;
    int         iProgSource     = m_iProgType;

    if( NPT(iSymSource)->IsA(SymbolType::Variable) ) {
        iSymSourceGroup = VPT(iSymSource)->GetOwnerGroup();
        iOccSource      = GPT(iSymSourceGroup)->GetCurrentExOccurrence();
        bSeeSource      = ( m_iProgType == PROCTYPE_PRE || m_iProgType == PROCTYPE_ONFOCUS );
    }
    else {                              // the source is a Group
        ASSERT(NPT(iSymSource)->IsA(SymbolType::Group));
        ASSERT( iProgSource == PROCTYPE_PRE || iProgSource == PROCTYPE_ONFOCUS );

        iSymSourceGroup = iSymSource;
        iOccSource = 1;
        bSeeSource = true;
    }
    if( iOccSource < 1 )
        iOccSource = 1;

    // set the target: issued at Level/PreProc - PAST the iSymLevelGroup, PROCTYPE_POST
    //                 otherwise               - the iSymLevelGroup, PROCTYPE_POST
    int         iSymTarget      = iSymSourceGroup;
    int         iOccTarget      = 1;
    int         iSymTargetGroup = iSymSourceGroup;
    ProcType    proc_target     = ProcType::PostProc;

    bool    bFromLevelPreProc=false;

    if( m_pEngineArea->IsLevel(iSymSource) && (iProgSource == PROCTYPE_PRE || iProgSource == PROCTYPE_ONFOCUS) )
        bFromLevelPreProc = true;

    if( NPT(iSymSource)->IsA(SymbolType::Variable) || !bFromLevelPreProc )
        proc_target = ProcType::KillFocus;
    else
        proc_target = ProcType::None; // change behavior...... Post del nivel menor

    // setup the skipping-flag and the target for the engine
    BatchExSetSkipping( iSymSource, iOccSource, iProgSource, iSymTarget, iOccTarget, (int)proc_target ); // victor Mar 26, 01

    // returns if checking of skip-struc is OFF
    if( !m_pEngineSettings->IsCheckingSkipStruc() )
        return 0;

    // gather the list of "dirty" fields between the source and the target
    std::vector<int> aDirtySymbol;
    std::vector<int> aDirtyOccur;
    int             iSymCheck  = iSymSource;
    int             iOccCheck;
    int             iSymGroup  = iSymSourceGroup;
    GROUPT*         pGroupT    = GPT(iSymGroup);
    int             iItemCheck = pGroupT->GetItemIndex( iSymCheck );
    bool            bTargetReached = false;
    int             iOccMax    = pGroupT->GetDataOccurrences();

    for( iOccCheck = iOccSource; !bTargetReached && iOccCheck <= iOccMax; iOccCheck++ ) {
        if( iSymCheck == iSymSource && iOccCheck == iOccSource && !bSeeSource )
            iItemCheck++;               // check the source only when required

        bTargetReached = BatchExScanOccur( aDirtySymbol, aDirtyOccur, pGroupT, iItemCheck, iOccCheck, (int)proc_target );

        if( !bTargetReached )           // next occurrence begin at first item
            iItemCheck = 0;
    }

    // display the list of messages on problems found
    BatchExDisplayDirty( aDirtySymbol, aDirtyOccur, iSkipType );

    return 0;
}


void CIntDriver::BatchExSetSkipping( int iSymSource, int iOccSource, int iProgSource,             // victor Mar 26, 01
                                     int iSymTarget, int iOccTarget, int iProgTarget ) {
    // BatchExSetSkipping: setup the skipping-flag and the target for the engine
    int     aIndex[DIM_MAXDIM];

    m_pEngineDriver->ResetSkipping();

    aIndex[0] = iOccSource - 1; aIndex[1] = aIndex[2] = 0; // ***TRANSITION***
    m_pEngineDriver->SetSkipping( iSymSource, aIndex, iProgSource );

    aIndex[0] = iOccTarget - 1; aIndex[1] = aIndex[2] = 0; // ***TRANSITION***
    m_pEngineDriver->SetSkipping( iSymTarget, aIndex, iProgTarget, true );
}

void CIntDriver::BatchExSetSkipping( C3DObject& objSource, int iProgSource,  // rcl, Sept 04, 04
                                     C3DObject& objTarget, int iProgTarget ) {
    // BatchExSetSkipping: setup the skipping-flag and the target for the engine

    m_pEngineDriver->ResetSkipping();

    m_pEngineDriver->SetSkippingSource( objSource, iProgSource );
    m_pEngineDriver->SetSkippingTarget( objTarget, iProgTarget );
}



bool CIntDriver::BatchExScanOccur( std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur,    // victor Mar 14, 01
                                   GROUPT* pGroupT, int iItemCheck, int iOccCheck, int iProgTarget ) {
    // BatchExScanOccur: check there is no "dirty" fields in pGroupT, occ iCheckOccur
    int     aIndex[DIM_MAXDIM];
    bool    bTargetReached = false;
    int     iSymCheck = pGroupT->GetItemSymbol( iItemCheck );

    aIndex[0] = iOccCheck - 1; aIndex[1] = aIndex[2] = 0; // ***TRANSITION***

    while( !bTargetReached && iSymCheck ) {
        bTargetReached = m_pEngineDriver->IsSkippingTargetReached( iSymCheck, aIndex, iProgTarget );

        if( !bTargetReached ) {
            // Group-item: scan it starting at its first item
            if( NPT(iSymCheck)->IsA(SymbolType::Group) )
                bTargetReached = BatchExScanOccur( aDirtySymbol, aDirtyOccur,
                                                   GPT(iSymCheck), 0, iOccCheck, iProgTarget );
            // Var-item: if a blank-field, add it to the list of "dirty" fields
            else if( !m_pEngineDriver->IsBlankField( iSymCheck, iOccCheck ) ) {
                /*
                VART*   pVarT=VPT(iSymCheck);
                if( pVarT->IsInAForm() ) {
                    pDirtySymbol->Add( iSymCheck );
                    pDirtyOccur->Add( pVarT->IsArray() ? iOccCheck : 0 );
                }
                */

                aDirtySymbol.emplace_back( iSymCheck );
/*??????*/      aDirtyOccur.emplace_back( VPT(iSymCheck)->IsArray() ? iOccCheck : 0 );

//?????? The alternate instruction below is needed if   // victor Mar 26, 01
//?????? the suggested change to VARX::PassTheOnlyIndex // victor Mar 26, 01
//?????? is not applied.  Be aware that providing an    // victor Mar 26, 01
//?????? 'iOccur' non-zero for a Sing-var is against    // victor Mar 26, 01
//?????? our own rules.  Remark also that the suggested // victor Mar 26, 01
//?????? modif simplifies passing "the index" not only  // victor Mar 26, 01
//?????? here, but in many other places.  Enjoy it.     // victor Mar 26, 01
//??????        pDirtyOccur->Add( iOccCheck );
            }

            if( !bTargetReached )       // set the next item to be checked
                iSymCheck = pGroupT->GetItemSymbol( ++iItemCheck );
        }
    }

    return bTargetReached;
}


void CIntDriver::BatchExDisplayDirty( std::vector<int>& aDirtySymbol, std::vector<int>& aDirtyOccur, // victor Mar 14, 01
                                      int iSkipType ) {
    // BatchExDisplayDirty: display a list of messages on "dirty" fields found
    int     iDirtyFound = (int)aDirtySymbol.size();

    if( iDirtyFound || iSkipType == 7 ) { // TODO - erase this line once BatchExEndLevel ready
//  if( iDirtyFound )                    // TODO - uncomment this line once BatchExEndLevel ready
        // adjusting iSkipType if needed
        C3DObject* pSkippingSource = m_pEngineDriver->GetSkippingSource();
        C3DObject* pSkippingTarget = m_pEngineDriver->GetSkippingTarget();
        CString csTargetName;
        int iTargetOcc = -1;

        if( pSkippingTarget != NULL ) {
            int     iTargetSymbol = pSkippingTarget->GetSymbol();

            if( iTargetSymbol > 0 ) {
                csTargetName = WS2CS(NPT(iTargetSymbol)->GetName());

                if( iSkipType == 4 ) {
                    iTargetOcc = pSkippingTarget->getIndexValue( 0 ) + 1; // ***TRANSITION*** // victor Apr 19, 01
                    if( iTargetOcc < 1 )
                        iSkipType = 5;  // change to Var(i) and 'i' is invalid
                }
            }
            else if( iSkipType != 7 )   // TODO - erase this line once BatchExEndLevel ready
//          else                        // TODO - uncomment this line once BatchExEndLevel ready
                iSkipType = 0;          // ... change to generic
        }
        else if( iSkipType < 6 )        // is 'skip to' (not endsect)...
            iSkipType = 0;              // ... change to generic

        // displaying the heading message (88200 thru 88207)
        int         iHeadMessage = 88200 + iSkipType;

        switch( iSkipType ) {
            case 1:                     // to Group
            case 2:                     // to next Var
            case 3:                     // to Var
                issaerror( MessageType::Error, iHeadMessage, iDirtyFound, csTargetName.GetString() );
                break;
            case 4:                     // to Var(i)
                issaerror( MessageType::Error, iHeadMessage, iDirtyFound, csTargetName.GetString(), iTargetOcc );
                break;
            case 5:                     // to Var(i) and 'i' is invalid
                issaerror( MessageType::Error, iHeadMessage, iDirtyFound, csTargetName.GetString() );
                break;
            case 0:                     // to generic
            case 6:                     // endsect
            case 7:                     // endlevel
            default:
                issaerror( MessageType::Error, iHeadMessage, iDirtyFound );
                break;
        }

        // displaying each "dirty" field
        int         iSymCheck;
        int         iOccCheck;
        VART*       pVarT;
        CNDIndexes  theIndex( ZERO_BASED ); // partial initialization ->
                                            // PassTheOnlyIndex will do the rest

        for( int iDirty = 0; iDirty < iDirtyFound; iDirty++ ) {
            iSymCheck = aDirtySymbol[iDirty];
            iOccCheck = aDirtyOccur[iDirty];

            if( NPT(iSymCheck)->IsA(SymbolType::Group) )
                issaerror( MessageType::Error, 88211, NPT(iSymCheck)->GetName().c_str() );
            else {
                pVarT = VPT(iSymCheck);

                if( !pVarT->IsSkipStrucImpute() ) continue; // RHF Nov 09, 2001

                VARX*   pVarX=pVarT->GetVarX();

                pVarX->PassTheOnlyIndex( theIndex, iOccCheck );

                const TCHAR* pVarAsciiAddr = GetVarAsciiAddr( pVarT, theIndex );
                CString csDirtyTxt(pVarAsciiAddr, pVarT->GetLength());

                // RHF INIC Nov 09, 2001
                //88212 ... %s should be blank (currently '%s')
                //88213 ... %s(%d) should be blank (currently '%s')
                //88214 ... %s should be blank (currently '%s'). %s will be assigned.
                //88215 ... %s(%d) should be blank (currently '%s'). %s will be assigned.

                if( m_pEngineSettings->IsCheckingSkipStrucImpute() ) {
                    CString csAssignText;

                    if( pVarT->IsNumeric() ) {
                        double  dValue;

                        // set to true as part of CSPro 7.2 refactoring as part of the introduction of the ValueProcessor;
                        // if BEHAVIOR_SKIPSTRUCIMPUTE is ever fully fleshed out, this decision can be revisited
                        //if( *(Flotbase+pVarT->nap ) == MASKBLK || *(Flotbase+pVarT->nap) != NOTAPPL ) {
                        if( true ) {
                            csAssignText = _T("NOTAPPL");
                            dValue = NOTAPPL;
                        }
                        else {
                            csAssignText = _T("DEFAULT (NOTAPPL mask undefined)");
                            dValue = DEFAULT;
                        }

                        //  ---- Assign Value
                        bool    bOk;

                        bOk = SetVarFloatValue( dValue, pVarX, theIndex );
                        ASSERT( bOk );


                        //TODO: all below MUST be done by 'SetVarFloatValue'    // victor Jul 25, 00
                        // RHF 12/8/99 Items/SubItems
                        if( bOk && pVarX != NULL ) {
                            if( Issamod == ModuleType::Entry || pVarX->iRelatedSlot >= 0 ) {// RHF Mar 01, 2000
                                ModuleType eOldMode = Issamod;

                                Issamod  = ModuleType::Batch; // Truco: in order to call varoutval and dvaltochar
                                // TODO SetAsciiValid( pVarX, false );
                                m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );  // write to ascii buffer
                                Issamod  = eOldMode;

                                if( pVarX->iRelatedSlot >= 0 )
                                    pVarX->VarxRefreshRelatedData( theIndex );
                            }
                        }
                        // RHF 12/8/99 Items/SubItems
                        //  ---- Assign Value
                    }
                    else {
                        csAssignText.Format( _T("'%*c'"), pVarT->GetLength(), ' ' );
                    }

                    if( !pVarT->IsArray() )
                        issaerror( MessageType::Error, 88214, pVarT->GetName().c_str(), csDirtyTxt.GetString(), csAssignText.GetString() );
                    else
                        issaerror( MessageType::Error, 88215, pVarT->GetName().c_str(), iOccCheck, csDirtyTxt.GetString(), csAssignText.GetString() );
                }
                // RHF END Nov 09, 2001
                else {
                    if( !pVarT->IsArray() )
                        issaerror( MessageType::Error, 88212, pVarT->GetName().c_str(), csDirtyTxt.GetString() );
                    else
                        issaerror( MessageType::Error, 88213, pVarT->GetName().c_str(), iOccCheck, csDirtyTxt.GetString() );
                }
            }
        }
    }
}

    // solving 'At' reference if any            <begin> // victor Mar 26, 01
double CIntDriver::EntryExSkipToAt( int iExpr ) {       // victor Mar 26, 01
    SKIP_NODE*  pSkipNode   = (SKIP_NODE*) (PPT(iExpr));
    bool        bSkipToNext = ( pSkipNode->var_exprind == SKIP_TO_NEXT_OCCURRENCE );

    // get the target-symbol and check if can go ahead
    int         iSymAt      = pSkipNode->m_iSymAt;

    int     iOccTargetAt;
    bool    bExplicitOcc;
    int     iSymTarget;

    if( pSkipNode->m_iAlphaExpr ) // 20120325
    {
        CString csTargetName = EvalAlphaExpr<CString>(pSkipNode->m_iAlphaExpr);
        csTargetName.TrimLeft();
        csTargetName.TrimRight();
        iSymTarget = GetReferredTargetSymbolChar(csTargetName,bSkipToNext,false,&iOccTargetAt,&bExplicitOcc);
    }

    else
    {
        if( iSymAt <= 0 )
            return 0;

        iSymTarget = GetReferredTargetSymbol( iSymAt, bSkipToNext, false, &iOccTargetAt, &bExplicitOcc );
    }

    if( iSymTarget <= 0 || !NPT(iSymTarget)->IsOneOf(SymbolType::Group, SymbolType::Block, SymbolType::Variable) )
        return 0;

    ASSERT( IsUsing3D_Driver() );

    C3DObject o3DTarget;
    double dIndex[DIM_MAXDIM];

    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        dIndex[iDim] = 0;

    Symbol* symbol = NPT(iSymTarget);

    GROUPT* pGroupT = symbol->IsA(SymbolType::Group) ? assert_cast<GROUPT*>(symbol) :
                      symbol->IsA(SymbolType::Block) ? assert_cast<const EngineBlock*>(symbol)->GetGroupT() :
                                                       assert_cast<VART*>(symbol)->GetOwnerGPT();

    if( symbol->IsOneOf(SymbolType::Block, SymbolType::Variable) )
    {
        if( bSkipToNext ) // SKIP TO NEXT
        {
            dIndex[0] = pGroupT->GetCurrentOccurrences() + 1;

            if( dIndex[0] > pGroupT->GetMaxOccs() )
            {
                GetCsDriver()->SetLogicRequestNature( CsDriver::LogicEndGroup ); // SkipToNext
                SetRequestIssued();
                return 0;
            }
        }

        else
        {
            // Sing var
            if( symbol->IsA(SymbolType::Variable) && !assert_cast<VART*>(symbol)->IsArray() )
                dIndex[0] = 1;

            // singly occurring block
            else if( symbol->IsA(SymbolType::Block) && pGroupT->GetMaxOccs() == 1 )
                dIndex[0] = 1;

            else
            {
                // Mult var or block - assuming target in current occurrence
                // RHF INIC Dec 09, 2003
                if( bExplicitOcc )
                    dIndex[0] = iOccTargetAt;

                else
                    // RHF END Dec 09, 2003
                    dIndex[0] = pGroupT->GetCurrentOccurrences();
            }
        }
    }

    else                                // target is a Group
        dIndex[0] = 1;

    int iOccTarget = (int)dIndex[0];               // TRANSITION-TO-3D

    // --- fixing unexpected 0 found!           <begin> // RHF Nov 22, 2002
        if( iOccTarget < 1 )                                // RHF Nov 22, 2002
            iOccTarget = 1;                                 // RHF Nov 22, 2002
        // --- fixing unexpected 0 found!           <end>   // RHF Nov 22, 2002


    m_pCsDriver->PassTo3D( &o3DTarget, iSymTarget, iOccTarget );

    // set request & target
    m_pCsDriver->SetLogicRequestNature( CsDriver::SkipTo );

    m_pCsDriver->Set3DTarget( &o3DTarget );

    bool    bFailed=!m_pCsDriver->IsValidSkipTarget();
    /* RHF COM INIC Dec 04, 2003 Use this code when you want to go to the next field when there is a fail
    // check the target - if invalid, set an innocuous NextField
    if( bFailed ) {
        m_pCsDriver->ResetRequest();
        m_pCsDriver->SetInterRequestNature( CsDriver::RequestNature::NextField );
    }
    SetRequestIssued();
    RHF COM END Dec 04, 2003 */

    // RHF INIC Dec 04, 2003 Use this code when you don't want to go to the next field when there is a fail
    if( bFailed )
        m_pCsDriver->ResetRequest();
    else
        SetRequestIssued();
    // RHF END Dec 04, 2003

    return 0;
}


double CIntDriver::BatchExSkipToAt( int iExpr ) {       // victor Mar 26, 01
    // BatchExSkipToAt: extract and check the referred target-symbol, then
    //                  check that every after-the-source and before-the-target are blank

    // works only if 'behavior() SkipStruc' specified, no matter if ON or OFF
    if( !m_pEngineSettings->HasSkipStruc() )
        return 0;

    m_iSkipStmt = TRUE;                    // to break the current proc

    SKIP_NODE*  pSkipNode   = (SKIP_NODE*) (PPT(iExpr));
    bool        bSkipToNext = ( pSkipNode->var_exprind == SKIP_TO_NEXT_OCCURRENCE );
    int         iSkipType   = 0;        // 0-generic 'skip to'

    // get the target-symbol and check if can go ahead
    int         iSymAt      = pSkipNode->m_iSymAt;

    if( iSymAt <= 0 )
        return 0;

    int     iOccTargetAt;
    bool    bExplicitOcc;
    int     iSymTarget = GetReferredTargetSymbol( iSymAt, bSkipToNext, false, &iOccTargetAt, &bExplicitOcc );

    if( !iSymTarget )
        return 0;

    // get the source
    int         iSymSource      = m_iExSymbol;
    int         iOccSource      = 0;
    int         iSymSourceGroup = 0;
    bool        bSeeSource      = false;
    int         iProgSource     = m_iProgType;

    if( NPT(iSymSource)->IsOneOf(SymbolType::Block, SymbolType::Variable) ) {
        iSymSourceGroup = NPT(iSymSource)->IsA(SymbolType::Block) ?
            GetSymbolEngineBlock(iSymSource).GetGroupT()->GetSymbol() : VPT(iSymSource)->GetOwnerGroup();
        iOccSource      = GPT(iSymSourceGroup)->GetCurrentExOccurrence();
        bSeeSource      = ( m_iProgType == PROCTYPE_PRE || m_iProgType == PROCTYPE_ONFOCUS );
    }
    if( iOccSource < 1 )
        iOccSource = 1;

    // get the (remainder of the) target
    int         iOccTarget   = 1;
    int         iProgTarget  = PROCTYPE_PRE;

    if( NPT(iSymTarget)->IsOneOf(SymbolType::Block, SymbolType::Variable) ) { // target is a block or Var:
        GROUPT* pGroupTTarget = NPT(iSymTarget)->IsA(SymbolType::Block) ?
            GetSymbolEngineBlock(iSymTarget).GetGroupT() : VPT(iSymTarget)->GetOwnerGPT();

        if( bSkipToNext ) {             // SKIP TO NEXT
            iOccTarget = pGroupTTarget->GetCurrentExOccurrence() + 1;
            iSkipType = 2;              // ... to next Var


            // RHF INIC Oct 08, 2003 Copied from BatchExSkipTo!!!
            // RHF INIC May 24, 2002 Fix problem skip to next in batch (when max occ is reached)
            if( iOccTarget > pGroupTTarget->GetMaxOccs() ) {
                return BatchExEndsect( iExpr );
            }
            // RHF END May 24, 2002
            // RHF END Oct 08, 2003

            // RHF INIC Oct 08, 2003 Fix problem skip to next in batch (when data occ is reached). This
            // happens when in Entry the variable that generate occurrence has an ENDGROUP and the ocurrence
            // is not generated in the data file. In Batch, skip to next reach an occurrence greater than DataOcc
            if( iOccTarget > pGroupTTarget->GetDataOccurrences() ) {
                bool    bShowWarning=false;

                if( bShowWarning ) {
                    if( pGroupTTarget->GetMaxOccs()  == 1 )
                        issaerror( MessageType::Warning, 88225, NPT(iSymTarget)->GetName().c_str(), pGroupTTarget->GetDataOccurrences() );
                    else
                        issaerror( MessageType::Warning, 88226, NPT(iSymTarget)->GetName().c_str(), iOccTarget, pGroupTTarget->GetDataOccurrences()  );
                }

                return BatchExEndsect( iExpr );
            }
            // RHF END Oct 08, 2003


        }
        else {                          // SKIP TO
            if( NPT(iSymTarget)->IsA(SymbolType::Variable) && !VPT(iSymTarget)->IsArray() ) {
                iOccTarget = 1;
                iSkipType  = 3;         // ... to Var
            }
            else {
                // RHF INIC Dec 09, 2003
                if( bExplicitOcc )
                    iOccTarget = iOccTargetAt;
                else
                    // RHF END Dec 09, 2003
                    iOccTarget = pGroupTTarget->GetCurrentExOccurrence();
                iSkipType  = 4;         // ... to Var(i)
            }
        }
    }
    else {                              // target is a Group
        iOccTarget      = 1;
        iSkipType       = 1;            // ... to Group
    }

    C3DObject   o3DTarget;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);
    dIndex[0] = iOccTarget;

    CsDriver::PassTo3D( &o3DTarget, NPT(iSymTarget), dIndex );

    C3DObject   o3DSourceDummy;

    BatchExSetSkipping( o3DSourceDummy, iProgSource, o3DTarget, iProgTarget );

    // 88200 %d inconsistent fields detected following a 'skip to' command in %p
    m_csSkipStructMsg.Format( MGF::GetMessageText(88190).c_str(), bSkipToNext ? _T("SKIP TO NEXT (by reference)") : _T("SKIP (by reference)"), ProcName().GetString() );

    return 0;
}

#include <Zentryo/hreplace.h>
bool CIntDriver::CheckAtSymbol(const CString& csFullName, int* piSymTarget, int* iOccTarget, bool* bExplicitOcc ) {
    bool   bRet=true;
    CIMSAString csTargetName, csTargetOcc;

    *piSymTarget  = 0;
    *iOccTarget   = 0;
    *bExplicitOcc = false;
    std::vector<std::wstring> aComponents = SO::SplitString(csFullName, '(');
    if( aComponents.size() == 2 ) {
        *bExplicitOcc = true;
        csTargetName = WS2CS(aComponents[0]);
        csTargetOcc = WS2CS(aComponents[1]);

        if( csTargetOcc.GetLength() == 0 ||
            csTargetOcc.GetLength() > 0 && csTargetOcc.GetAt(csTargetOcc.GetLength()-1) != ')' ) {
            bRet = false;
        }
        else {
            CIMSAString csExpandedText;
            bool    bSomeError=false;

            //csTargetOcc.SetAt(csTargetOcc.GetLength()-1, ' ' ); // Remove  ')'
            csTargetOcc.TrimRight( ')' );

            if( csTargetOcc.IsNumeric() ) {
                csExpandedText = csTargetOcc;
            }
            else {
                CIMSAString csText;
                csText.Format( _T("%lc%ls%lc"), HELP_OPENVARCHAR, csTargetOcc.GetString(), HELP_CLOSEVARCHAR );
                csExpandedText = ExpandText( csText, false, &bSomeError );
            }

            if( bSomeError || !csExpandedText.IsNumeric() )
                bRet = false;
            else {
                *iOccTarget = _ttoi( csExpandedText );
            }
        }
    }
    else if( aComponents.size() == 1 ) {
        csTargetName = WS2CS(aComponents.front());
        *iOccTarget = 1;
    }
    else {
        return false;
    }

    if( bRet )
    {
        const static std::vector<SymbolType> allowable_symbol_types
        {
            SymbolType::Variable,
            SymbolType::Block,
            SymbolType::Group,
        };

        *piSymTarget = m_pEngineArea->SymbolTableSearch(csTargetName, allowable_symbol_types);

        if( *piSymTarget == 0 )
            bRet = false;

        else if( NPT(*piSymTarget)->IsA(SymbolType::Variable) && *bExplicitOcc && !VPT(*piSymTarget)->IsArray() ) // Sing var
            bRet = false;

        else if( ( NPT(*piSymTarget)->IsA(SymbolType::Group) && *iOccTarget != 1 ) ||
            ( NPT(*piSymTarget)->IsA(SymbolType::Block) && ( *iOccTarget > GetSymbolEngineBlock(*piSymTarget).GetGroupT()->GetMaxOccs() || *iOccTarget < 1 ) ) ||
            ( NPT(*piSymTarget)->IsA(SymbolType::Variable) && ( *iOccTarget > VPT(*piSymTarget)->GetFullNumOccs(true) || *iOccTarget < 1 ) ) )
        {
            bRet = false;
        }
    }

    return bRet;
}

int CIntDriver::GetReferredTargetSymbol( int iSymAt, bool bSkipToNext, bool bMove, int* iOccTargetAt, bool* bExplicitOcc ) { // victor Mar 26, 01
    ASSERT( iSymAt > 0 );
    Symbol* symbol = NPT(iSymAt);
    CString csTargetName;

    if( symbol->IsA(SymbolType::WorkString) )
    {
        csTargetName = WS2CS(assert_cast<const WorkString*>(symbol)->GetString());
    }

    else
    {
        VART* pAtVarT = assert_cast<VART*>(symbol);
        ASSERT( !pAtVarT->IsNumeric() );// 'AtSymbol' must be alpha
        ASSERT( !pAtVarT->IsArray() );  // 'AtSymbol' must be Sing-var

        if( pAtVarT->GetLogicStringPtr() ) // 20140403 a variable length string
        {
            csTargetName = *( pAtVarT->GetLogicStringPtr() );
        }

        else
        {
            // get the referred target-name from 'AtSymbol'
            const TCHAR* pVarAsciiAddr = GetSingVarAsciiAddr(pAtVarT);
            int iAtLength = pAtVarT->GetLength();
            csTargetName = CString(pVarAsciiAddr, iAtLength);
        }
    }

    csTargetName.Trim();

    return GetReferredTargetSymbolChar(csTargetName, bSkipToNext, bMove, iOccTargetAt, bExplicitOcc, symbol);
}



int CIntDriver::GetReferredTargetSymbolChar(const CString& csTargetName, bool bSkipToNext, bool bMove,
    int *iTargetOcc, bool *bExplicitOcc, const Symbol* symbol_holding_name/* = nullptr*/) // 20120325
{
    int iErrorMsg = 0;
    int iSymTarget = 0;

    if( !CheckAtSymbol( csTargetName, &iSymTarget, iTargetOcc, bExplicitOcc ) || bSkipToNext && bExplicitOcc && *bExplicitOcc ) // RHF Mar 01, 2004
        iErrorMsg = bMove ? 88141 : 88111;

    // invalid reference, the skip is ignored
    else if( iSymTarget <= 0 )
        iErrorMsg = bMove ? 88141 : 88111;

    // target must be either VA, Block, or GR
    else if( !NPT(iSymTarget)->IsOneOf(SymbolType::Variable, SymbolType::Block, SymbolType::Group) )
        iErrorMsg = bMove ? 88145 : 88115;

    else if( bSkipToNext )
    {
        Symbol* target_symbol = NPT(iSymTarget);

        VART* pVarTTarget = target_symbol->IsA(SymbolType::Variable) ? (VART*)target_symbol :
                                                                       nullptr;

        GROUPT* pTargetGroupT = target_symbol->IsA(SymbolType::Block) ? assert_cast<const EngineBlock*>(target_symbol)->GetGroupT() :
                                ( pVarTTarget != nullptr )            ? pVarTTarget->GetOwnerGPT() :
                                                                        nullptr;

        Symbol* source_symbol = NPT(m_iExSymbol);
        GROUPT* pSourceGroupT = source_symbol->IsA(SymbolType::Block)    ? assert_cast<const EngineBlock*>(source_symbol)->GetGroupT() :
                                source_symbol->IsA(SymbolType::Variable) ? ((VART*)source_symbol)->GetOwnerGPT() :
                                                                           nullptr;

        // both source and target must belong to the same group
        if( pSourceGroupT != pTargetGroupT )
            iErrorMsg = 88114;

        // the source must be a Var or block
        else if( source_symbol == nullptr )
            iErrorMsg = 88113;

        // the target must be a Mult-Var or block
        else if( ( pVarTTarget != nullptr && !pVarTTarget->IsArray() ) || ( pVarTTarget == nullptr && pTargetGroupT->GetMaxOccs() < 2 ) )
            iErrorMsg = 88112;
    }

    if( iErrorMsg != 0 ) // issue message and forgets the target
    {
        CString csAtName = ( symbol_holding_name != nullptr ) ? WS2CS(symbol_holding_name->GetName()) : csTargetName;
        issaerror( MessageType::Error, iErrorMsg, csAtName.GetString(), csTargetName.GetString(), _T(" - the procedure is abandoned anyway") );
        iSymTarget = 0;
    }

    return iSymTarget;
}

// solving 'At' reference if any            <end>   // victor Mar 26, 01




// RHF INIC Nov 24, 2003
int CIntDriver::GetReferredReenterAdvanceTargetSymbol( int iSymAt, bool bAdvance, bool bMove, int* iOccTargetAt, bool* bExplicitOcc ) {
    ASSERT( iSymAt > 0 );
    Symbol* symbol = NPT(iSymAt);
    CString csTargetName;

    if( symbol->IsA(SymbolType::WorkString) )
    {
        csTargetName = WS2CS(assert_cast<const WorkString*>(symbol)->GetString());
    }

    else
    {
        VART* pAtVarT = assert_cast<VART*>(symbol);
        ASSERT( !pAtVarT->IsNumeric() );// 'AtSymbol' must be alpha
        ASSERT( !pAtVarT->IsArray() );  // 'AtSymbol' must be Sing-var

        if( pAtVarT->GetLogicStringPtr() ) // 20140403 a variable length string
        {
            csTargetName = *( pAtVarT->GetLogicStringPtr() );
        }

        else
        {
            // get the referred target-name from 'AtSymbol'
            const csprochar* pVarAsciiAddr = GetSingVarAsciiAddr( pAtVarT );
            int iAtLength = pAtVarT->GetLength();
            csTargetName = CString(pVarAsciiAddr, iAtLength);
        }
    }

    csTargetName.Trim();

    return GetReferredReenterAdvanceTargetSymbolChar(csTargetName, bAdvance, bMove, iOccTargetAt, bExplicitOcc, symbol);
}


int CIntDriver::GetReferredReenterAdvanceTargetSymbolChar(const CString& csTargetName, bool bAdvance, bool bMove,
    int* iTargetOcc, bool* bExplicitOcc, const Symbol* symbol_holding_name/* = nullptr*/) // 20120521
{
    int     iErrorMsg  = 0;
    int     iSymTarget = 0;

    // RHF INIC Dec 09, 2003
    if( !CheckAtSymbol( csTargetName, &iSymTarget, iTargetOcc, bExplicitOcc ) )
        iErrorMsg = bMove       ? 88141 :
                    bAdvance    ? 88131 : 88121;
    // RHF END Dec 09, 2003

    else if( iSymTarget <= 0 ) {
        // invalid reference, the reenter is ignored

        // RHF COM Dec 10, 2003iErrorMsg = bAdvance ? 88131 : 88121;

        iErrorMsg = bMove       ? 88141 :
                    bAdvance    ? 88131 : 88121;
    }
    else if( !NPT(iSymTarget)->IsOneOf(SymbolType::Variable, SymbolType::Block) ) {
        // target must be a VA or Block
        // RHF COM Dec 10, 2003 iErrorMsg = bAdvance ? 88135 : 88125;
        iErrorMsg = bMove       ? 88145 :
                    bAdvance    ? 88135 : 88125;
    }

    if( iErrorMsg ) {                   // issue message and forgets the target
        CString csAtName = ( symbol_holding_name != nullptr ) ? WS2CS(symbol_holding_name->GetName()) : csTargetName;
        issaerror( MessageType::Error, iErrorMsg, csAtName.GetString(), csTargetName.GetString(), _T(" - the procedure is abandoned anyway") );
        iSymTarget = 0;
    }

    return iSymTarget;
}

    // solving 'At' reference if any
double CIntDriver::EntryExReenterAdvanceToAt( int iExpr, bool bAdvance ) {
    if( !IsUsing3D_Driver() ) {
        ASSERT(0);
        return 0;
    }

    SKIP_NODE*  pReenterNode   = (SKIP_NODE*) (PPT(iExpr));

    // get the target-symbol and check if can go ahead
    int         iSymAt      = pReenterNode->m_iSymAt;

    int     iOccTargetAt;
    bool    bExplicitOcc;
    int     iSymTarget;

    if( pReenterNode->m_iAlphaExpr ) // 20120521
    {
        CString csTargetName = EvalAlphaExpr<CString>(pReenterNode->m_iAlphaExpr);
        csTargetName.TrimLeft();
        csTargetName.TrimRight();
        iSymTarget = GetReferredReenterAdvanceTargetSymbolChar(csTargetName,bAdvance,false,&iOccTargetAt,&bExplicitOcc);
    }

    else
    {
        if( iSymAt <= 0 )
            return 0;

        iSymTarget = GetReferredReenterAdvanceTargetSymbol( iSymAt, bAdvance, false, &iOccTargetAt, &bExplicitOcc );
    }

    if( iSymTarget <= 0 || !NPT(iSymTarget)->IsOneOf(SymbolType::Block, SymbolType::Variable) )
        return 0;

    C3DObject o3DTarget;
    double dIndex[DIM_MAXDIM];

    for( int iDim = 0; iDim < DIM_MAXDIM; iDim++ )
        dIndex[iDim] = 0;

    Symbol* symbol = NPT(iSymTarget);

    GROUPT* pGroupT = symbol->IsA(SymbolType::Block) ? assert_cast<const EngineBlock*>(symbol)->GetGroupT() :
                                                       assert_cast<VART*>(symbol)->GetOwnerGPT();

    // Sing var
    if( symbol->IsA(SymbolType::Variable) && !VPT(iSymTarget)->IsArray() )
        dIndex[0] = 1;

    // singly occurring block
    else if( symbol->IsA(SymbolType::Block) && pGroupT->GetMaxOccs() == 1 )
        dIndex[0] = 1;

    else
    {
        // Mult var or block - assuming target in current occurrence
        // RHF INIC Dec 09, 2003
        if( bExplicitOcc )
            dIndex[0]  = iOccTargetAt;

        else
            // RHF END Dec 09, 2003
            dIndex[0] = pGroupT->GetCurrentOccurrences();
    }

    int iOccTarget = (int) dIndex[0];               // TRANSITION-TO-3D

    // --- fixing unexpected 0 found!           <begin> // RHF Nov 22, 2002
    if( iOccTarget < 1 )                                // RHF Nov 22, 2002
        iOccTarget = 1;                                 // RHF Nov 22, 2002
    // --- fixing unexpected 0 found!           <end>   // RHF Nov 22, 2002

    bool bFailed = false;

    // RHF INIC Dec 22, 2003
    if( !bAdvance && iSymTarget == m_iExSymbol ) {// itself
        // set request
        m_pCsDriver->SetInterRequestNature( CsDriver::Reenter, true );
        o3DTarget.SetEmpty();
        m_pCsDriver->Set3DTarget( &o3DTarget );
    }
    else {
        // RHF END Dec 22, 2003

        m_pCsDriver->PassTo3D( &o3DTarget, iSymTarget, iOccTarget );

        // set request & target
        m_pCsDriver->SetLogicRequestNature( bAdvance ?  CsDriver::AdvanceTo : CsDriver::Reenter );
        m_pCsDriver->Set3DTarget( &o3DTarget );

        bFailed= (!bAdvance && !m_pCsDriver->IsValidReenterTarget() || bAdvance && !m_pCsDriver->IsValidAdvanceTarget() );
    }

    /* RHF COM INIC Dec 04, 2003 Use this code when you want to go to the next field when there is a fail
     // check the target - if invalid, set an innocuous NextField
    if( bFailed ) {
        m_pCsDriver->ResetRequest();
        m_pCsDriver->SetInterRequestNature( CsDriver::RequestNature::NextField ); // RHF COM Dec 03, 2003  Comment this line when you don't want to go to the next field
    }
    SetRequestIssued();
    RHF COM END Dec 04, 2003 */

    // RHF INIC Dec 04, 2003 Use this code when you don't want to go to the next field when there is a fail
    if( bFailed )
        m_pCsDriver->ResetRequest();
    else
        SetRequestIssued();
    // RHF END Dec 04, 2003

    return 0;
}

// solving 'At' reference if any
double CIntDriver::EntryExReenterToAt( int iExpr ) {
    return EntryExReenterAdvanceToAt( iExpr, false );
}

double CIntDriver::EntryExAdvanceToAt( int iExpr ) {
    return EntryExReenterAdvanceToAt( iExpr, true );
}
// RHF END Nov 24, 2003

// RHF INIC Dec 09, 2003
//--------------------------------------------------------
//  exmoveto: executes an move to command
//--------------------------------------------------------

    // BUCEN_DEC2003 Changes Init
double CIntDriver::exmoveto( int iExpr ) {
    double  dRet=0;
    bool    bError=false;
    int     iSymTarget=0;
    int     iOccTarget=0;
    bool    bSkipMode=true;
    bool    bExplicitOccDummy=false;
    UserIndexesArray dIndex;

    initUserIndexArray(dIndex);

    if( !IsUsing3D_Driver() ) {
        ASSERT(0);
        bError = true;
    }
    else if( m_bExecSpecFunc ) {
        issaerror( MessageType::Error, 9100 );
        bError = true;
    }

    else { // !bError
        m_iSkipStmt = TRUE;                    // to break the current proc

        SKIP_NODE*  pMoveNode = (SKIP_NODE*) (PPT(iExpr));
        bSkipMode = ( pMoveNode->var_exprind == 0 ||  pMoveNode->var_exprind == 1 );

        // 20140730 rewrote how the alpha expressions are evaluated
        if( pMoveNode->m_iSymAt || pMoveNode->m_iAlphaExpr )
        {
            if( pMoveNode->m_iSymAt > 0 )
            {
                iSymTarget = GetReferredTargetSymbol( pMoveNode->m_iSymAt, false, true, &iOccTarget, &bExplicitOccDummy );
            }

            else
            {
                CString csTargetName = EvalAlphaExpr<CString>(pMoveNode->m_iAlphaExpr);
                csTargetName.TrimLeft();
                csTargetName.TrimRight();
                iSymTarget = GetReferredTargetSymbolChar(csTargetName,false,true,&iOccTarget,&bExplicitOccDummy);
            }

            bError = iSymTarget <= 0 || !NPT(iSymTarget)->IsOneOf(SymbolType::Group, SymbolType::Block, SymbolType::Variable);

            if( !bError )
            {
                if( !bExplicitOccDummy && NPT(iSymTarget)->IsA(SymbolType::Variable) )
                {
                    VART * pVarT = VPT(iSymTarget);
                    GROUPT * pGroupT = pVarT->GetParentGPT();

                    if( pGroupT != NULL )
                        iOccTarget = pGroupT->GetCurrentExOccurrence();
                }

                if( iOccTarget < 1 )
                    iOccTarget = 1;

                dIndex[0] = iOccTarget;
            }
        }

        else {
            SVAR_NODE*  pSVarNode;
            MVAR_NODE*  pMVarNode;
            GRP_NODE*   pGrpNode;

            // RHF COM Dec 09, 2003GROUPT*     pGroupT;

            pSVarNode = (SVAR_NODE*) (PPT(pMoveNode->var_ind));
            pMVarNode = (MVAR_NODE*) pSVarNode;
            pGrpNode  = (GRP_NODE*) pSVarNode;

            if( pSVarNode->m_iVarType == SVAR_CODE ) {
                iSymTarget = pSVarNode->m_iVarIndex;
                dIndex[0] = 1;
            }
            else if( pMVarNode->m_iVarType == MVAR_CODE ) {
                iSymTarget = pMVarNode->m_iVarIndex;
                mvarGetSubindexes( pMVarNode, dIndex );
            }
            else if( pMVarNode->m_iVarType == BLOCK_CODE ) {
                iSymTarget = pGrpNode->m_iGrpIndex;
                grpGetSubindexes(GetSymbolEngineBlock(iSymTarget).GetGroupT(), pGrpNode, dIndex);
            }
            else {                          // GRP_NODE
                ASSERT(pMVarNode->m_iVarType == GROUP_CODE);
                iSymTarget = pGrpNode->m_iGrpIndex;
                grpGetSubindexes( pGrpNode, dIndex );
            }

            iOccTarget = (int) dIndex[0];               // TRANSITION-TO-3D

            // --- fixing unexpected 0 found!           <begin> // victor Jun 27, 02
            if( iOccTarget < 1 )                                // victor Jun 27, 02
                iOccTarget = 1;                                 // victor Jun 27, 02
            // --- fixing unexpected 0 found!           <end>   // victor Jun 27, 02
        }
    }

    if( !bError ) {

        bool    bValidSkip=false;
        bool    bValidReenter=false;
        bool    bValidAdvance=false;
        bool    bSilent=true;
        int     iLocation;

        C3DObject o3DTarget;
        C3DObject* p3DTarget = nullptr; // used for entry checks

        if( Issamod != ModuleType::Entry ) {
            int     iSourceSym=-1;
            int     iSourceOcc=-1;

            iLocation = m_pEngineDriver->CompareAbsoluteFlowOrder( iSourceSym, iSourceOcc, iSymTarget, iOccTarget);

            if( iLocation == -1 || iLocation == 0 )
                bValidReenter = true;
            else if( iLocation == 1 ) {
                if( bSkipMode )
                    bValidSkip = true;
                else
                    bValidAdvance = true;
            }
            else {
                issaerror( MessageType::Error, 88142 );
                bError = true;
            }
        }
        else {
            p3DTarget = &o3DTarget;

            m_pCsDriver->PassTo3D( p3DTarget, iSymTarget, dIndex );

            m_pCsDriver->SetLogicRequestNature( CsDriver::SkipTo );
            m_pCsDriver->Set3DTarget( p3DTarget );
            bValidSkip = m_pCsDriver->IsValidSkipTarget(bSilent);

            m_pCsDriver->SetLogicRequestNature( CsDriver::Reenter );
            m_pCsDriver->Set3DTarget( p3DTarget );
            bValidReenter = m_pCsDriver->IsValidReenterTarget(true,bSilent);

            m_pCsDriver->SetLogicRequestNature( CsDriver::AdvanceTo );
            m_pCsDriver->Set3DTarget( p3DTarget );
            bValidAdvance = m_pCsDriver->IsValidAdvanceTarget(bSilent);

            m_pCsDriver->ResetRequests();

            iLocation = m_pCsDriver->SearchTargetLocation( p3DTarget );
        }


        if( iLocation <= 0 )  { // Target before current atom or in the same position
            if( bValidReenter ) {
                dRet = exreenter( iExpr );
            }
            else {
                if( Issamod == ModuleType::Entry )
                    m_pCsDriver->IsValidReenterTarget(true, false, p3DTarget); // Show error message
                bError = true;
            }
        }
        else if( iLocation > 0 )  { // Target after current atom
            if( bSkipMode ) {
                if( bValidSkip ) {
                    dRet =  exskipto( iExpr );
                }
                else {
                    if( Issamod == ModuleType::Entry )
                       m_pCsDriver->IsValidSkipTarget(false, p3DTarget); // Show error message
                    bError = true;
                }
            }
            else {
                if( bValidAdvance ) {
                    dRet = exadvance( iExpr );
                }
                else {
                    if( Issamod == ModuleType::Entry )
                        m_pCsDriver->IsValidAdvanceTarget(false, p3DTarget); // Show error message
                    bError = true;
                }
            }
        }
        else { // iLocation = 0
            ASSERT(0); // See above
        }
    }

    return bError ? 0 : dRet;
}
// RHF END Dec 09, 2003
   //BUCEN_DEC2003 Changes End


double CIntDriver::exask(int iExpr) // for ask-if and targetless skips
{
    ASK_NODE* pAskNode = (ASK_NODE*)PPT(iExpr);
    bool bIsTargetlessSkip = ( pAskNode->ask_universe < 0 );
    const TCHAR* const skip_name = bIsTargetlessSkip ? _T("skip") : _T("ask-if");

    if( !bIsTargetlessSkip )
    {
        double dCondition = evalexpr(pAskNode->ask_universe);

        if( dCondition != 0 ) // if the ask-if condition is valid, then quit out
            return 0;
    }

    // get out in Run as Batch if not applicable
    if( Issamod != ModuleType::Entry && !m_pEngineSettings->HasSkipStruc() )
        return 0;

    // make sure that the skip is being done in a valid place
    bool bCallComesFromUserbar = ( bIsTargetlessSkip && ( m_FieldSymbol != 0 ) );

    if( m_iExSymbol <= 0 || m_pEngineArea->IsLevel(m_iExSymbol) || ( m_iProgType != PROCTYPE_PRE && !bCallComesFromUserbar ) )
    {
        issaerror(MessageType::Error, 88151, skip_name);
        return 0;
    }

    m_iSkipStmt = TRUE; // to break the current proc

    // evaluate where we should skip
    Symbol* pSymbol = NPT(m_iExSymbol);
    bool bEndGroup = true;

    if( pSymbol->IsOneOf(SymbolType::Block, SymbolType::Variable) )
    {
        VART* pVarT = nullptr;
        const EngineBlock* engine_block = nullptr;
        GROUPT* pGroupT = nullptr;

        if( pSymbol->IsA(SymbolType::Block) )
        {
            engine_block = assert_cast<const EngineBlock*>(pSymbol);
            pGroupT = engine_block->GetGroupT();
        }

        else
        {
            pVarT = (VART*)pSymbol;
            pGroupT = pVarT->GetOwnerGPT();
        }


        int iCurOcc = pGroupT->GetCurrentExOccurrence();
        int iCurItem = pGroupT->GetItemIndex(m_iExSymbol);

        int iNewOcc = 0;
        int iNewItem = 0;
        int iSymNewItem = 0;

        while( true )
        {
            iSymNewItem = pGroupT->SearchNextItemInGroup(iCurOcc, iCurItem, &iNewOcc, &iNewItem);

            // if executed from a block, we cannot stop on a field on the block
            if( engine_block != nullptr && iSymNewItem > 0 )
            {
                if( NPT(iSymNewItem)->IsA(SymbolType::Variable) && engine_block->ContainsField(iSymNewItem) )
                {
                    ++iCurItem;
                    continue;
                }
            }

            break;
        }

        if( iSymNewItem > 0 ) // if it is 0, then there are no more fields in this group
        {
            bEndGroup = false;

            UserIndexesArray dIndex;
            GetIndexArrayWithNewOccurrence(dIndex, pVarT, pGroupT, iNewOcc);

            //prepare the o3DTarget with the indexes that are expected by skip to
            C3DObject o3DTarget;
            CsDriver::PassTo3D(&o3DTarget, NPT(iSymNewItem), dIndex);

            if( Issamod == ModuleType::Entry )
            {
                m_pCsDriver->SetLogicRequestNature(CsDriver::SkipTo);
                m_pCsDriver->Set3DTarget(&o3DTarget);

                ASSERT(m_pCsDriver->IsValidSkipTarget());
            }

            else
            {
                C3DObject o3DSourceDummy;
                BatchExSetSkipping(o3DSourceDummy, m_iProgType, o3DTarget, PROCTYPE_PRE);

                // 88200 %d inconsistent fields detected following a 'skip to' command in %p
                m_csSkipStructMsg.Format(MGF::GetMessageText(88190).c_str(), skip_name, ProcName().GetString());
            }
        }
    }

    if( bEndGroup )
    {
        if( Issamod == ModuleType::Entry )
        {
            m_pCsDriver->SetLogicRequestNature(CsDriver::LogicEndGroup);
        }

        else
        {
            BatchExEndsect(0);
        }
    }

    SetRequestIssued();

    return 0;
}
