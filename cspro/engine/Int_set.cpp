//--------------------------------------------------------------------------
//
// INT_SET.cpp    : interpret SET commands
//
//--------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Dictionaries.h>
#include <CSEntry/MessageOverrides.h>


//--------------------------------------------------------------------------
//  exset: executes 'SET FORMAT',
//    *** forbidden 'SET HEADING', 'SET LINEPAGE', ***  // victor Apr 04, 01
//                  'SET BEHAVIOR',                     // victor Aug 26, 99
//               or invokes "exdictaccess" (see INTEXTDI.cpp)
//--------------------------------------------------------------------------


void CIntDriver::CleanExportTo()
{
    m_pEngineSettings->SetExportSPSS( false );
    m_pEngineSettings->SetExportSAS( false );
    m_pEngineSettings->SetExportSTATA( false );
    m_pEngineSettings->SetExportCSPRO( false );
    m_pEngineSettings->SetExportTabDelim( false );
    m_pEngineSettings->SetExportCommaDelim( false );
    m_pEngineSettings->SetExportSemiColonDelim( false );
    m_pEngineSettings->SetExportR( false );
}


double CIntDriver::exset(int iExpr)
{
    const SETOTHER_NODE* pset_ot = (SETOTHER_NODE*)PPT(iExpr);
    bool return_code = true;

    // ignore codes with no action and deprecated codes
    if( pset_ot->type == -5 || pset_ot->type == -6 || pset_ot->type == -7 || pset_ot->type == -9 || pset_ot->type == -10 )
    {
    }

    // -8: SET BEHAVIOR                                 // victor Aug 26, 99
    else if( pset_ot->type == static_cast<int>(SetAction::Behavior) ) {
        bool    bSetOn = true;
        bool    bConfirm = true;
        short   iBehaviorItem = pset_ot->setinfo.behavior_item & 0x0000ffff;
        short   iBehaviorOpt =  (pset_ot->setinfo.behavior_item & 0x00ff0000) >> 16;
        short   iBehaviorOpt2 = (pset_ot->setinfo.behavior_item & 0xff000000) >> 24;
        bool    bPreLevelZero = ( m_iProgType == PROCTYPE_PRE && m_iExLevel == 0 );
        bool    bPathOff = m_pEngineSettings->IsPathOff();
#ifdef  _DEBUG
        csprochar* fAction = _T("...... SessionSettings->SetBehavior() %s ......\n");
#endif
        csprochar const * pAction = _T("");

        LIST_NODE*  pListNode=NULL;

        // RHF INIC Sep 22, 2004
        switch( iBehaviorItem ) {
            case BEHAVIOR_CANENTER_NOTAPPL:
            case BEHAVIOR_CANENTER_OUTOFRANGE:
            {
                int     size_othe = sizeof(SETOTHER_NODE);
                int     real_size = size_othe/sizeof(int);
                pListNode = (LIST_NODE*) PPT(iExpr + real_size);
            }
        }
      // RHF END Sep 22, 2004

        switch( iBehaviorItem ) {       // get ON or OFF
            case BEHAVIOR_PATH:
            case BEHAVIOR_MESSAGES_DISPLAY:
            case BEHAVIOR_MESSAGES_ERRMSG:
            case BEHAVIOR_CANENTER_NOTAPPL:
            case BEHAVIOR_CANENTER_OUTOFRANGE:
            case BEHAVIOR_MOUSE:                        // victor Feb 16, 00
            case BEHAVIOR_ENDGROUP:                     // victor Feb 16, 00
            case BEHAVIOR_ENDLEVEL:                     // victor Feb 16, 00
            case BEHAVIOR_MSGNUMBER_DISPLAY:            // victor Jun 08, 00
            case BEHAVIOR_MSGNUMBER_ERRMSG:             // victor Jun 08, 00
            case BEHAVIOR_EXPORT_DATA:                  // victor Dec 18, 00
            case BEHAVIOR_EXPORT_SPSS:                  // victor Dec 18, 00
            case BEHAVIOR_EXPORT_SAS:                   // victor Dec 18, 00
            case BEHAVIOR_EXPORT_STATA:                 // victor Dec 18, 00
            case BEHAVIOR_EXPORT_R:

            case BEHAVIOR_EXPORT_ALL:                      // RHF Oct 08, 2004
            case BEHAVIOR_EXPORT_CSPRO:                 // RHF Oct 08, 2004
            case BEHAVIOR_EXPORT_ALL4:              // RHF Aug 02, 2006
            case BEHAVIOR_EXPORT_ALL5:

            case BEHAVIOR_EXPORT_TABDELIM:                // RHF Oct 13, 2004
            case BEHAVIOR_EXPORT_COMMADELIM:              // RHF Oct 13, 2004
            case BEHAVIOR_EXPORT_SEMICOLONDELIM:          // RHF Oct 13, 2004

            case BEHAVIOR_CHECKRANGES:                  // victor Dec 07, 00
            case BEHAVIOR_SKIPSTRUC:                    // victor Mar 14, 01
            case BEHAVIOR_SKIPSTRUCIMPUTE:              // RHF Nov 09, 2001

            case BEHAVIOR_EXIT: // RHF Aug 03, 2006
            case BEHAVIOR_SPECIALVALUES: // 20090827

                bSetOn = ( pset_ot->len >= 1 );

                // for CanEnterX ON, get "confirmation asked"
                if( ( iBehaviorItem == BEHAVIOR_CANENTER_NOTAPPL    ||
                      iBehaviorItem == BEHAVIOR_CANENTER_OUTOFRANGE ) &&
                    bSetOn ) {
                    bConfirm = ( pset_ot->len == 1 );   // 2: without confirmation
                }

                // RHF INIC Sep 12, 2001
                // OFF
                if( ( iBehaviorItem == BEHAVIOR_CANENTER_NOTAPPL    ||
                      iBehaviorItem == BEHAVIOR_CANENTER_OUTOFRANGE ) &&
                    !bSetOn ) {
                    bConfirm = ( pset_ot->len != -1 );
                }
                // RHF END Sep 12, 2001
                break;

            default:
                break;
        }

        switch( iBehaviorItem ) {
            case BEHAVIOR_PATH:
                pAction = ( bSetOn ) ? _T("Path ON") : _T("Path OFF");

                // allowed only in Level-0 PreProc      // victor Feb 16, 00
                if( !bPreLevelZero )
                    issaerror( MessageType::Warning, 91111, pAction );
                else {
                    if( bSetOn )
                        m_pEngineSettings->SetPathOn();
                    else
                        m_pEngineSettings->SetPathOff();

                    ResetAllVarsBehavior();
#ifdef  _DEBUG
                    TRACE( fAction, pAction );
#endif
                }
                break;

            case BEHAVIOR_CANENTER_NOTAPPL:
#ifdef  _DEBUG
                pAction = ( !bSetOn )  ? _T("CanEnterNotAppl OFF") :
                          ( bConfirm ) ? _T("CanEnterNotAppl ON(CONFIRM)") : _T("CanEnterNotAppl ON(noCONFIRM)");
#endif

                if( pListNode != NULL && pListNode->iNumElems > 0 )
                    ExSetBehaviorList( iBehaviorItem, pListNode, bSetOn, bConfirm );
                else {
                    if( bSetOn ) {
                        m_pEngineSettings->SetCanEnterNotappl();
                        if( bConfirm )
                            m_pEngineSettings->SetMustAskWhenNotappl();
                        else
                            m_pEngineSettings->SetDontAskWhenNotappl();
                    }
                    else {
                        m_pEngineSettings->SetCannotEnterNotappl();

                        // RHF INIC Sep 12, 2001
                        if( bConfirm )
                            m_pEngineSettings->SetMustAskWhenNotappl();
                        else
                            m_pEngineSettings->SetDontAskWhenNotappl();
                        // RHF END Sep 12, 2001
                    }

                    ResetAllVarsBehavior();
                }

#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                frm_capimode( 0, 1 );
                break;

            case BEHAVIOR_CANENTER_OUTOFRANGE:
#ifdef  _DEBUG
                pAction = ( !bSetOn )  ? _T("CanEnterOutOfRange OFF") :
                          ( bConfirm ) ? _T("CanEnterOutOfRange ON(CONFIRM)") : _T("CanEnterOutOfRange ON(noCONFIRM)");
#endif

                if( pListNode != NULL && pListNode->iNumElems > 0 )
                    ExSetBehaviorList( iBehaviorItem, pListNode, bSetOn, bConfirm );
                else {
                    if( bSetOn ) {
                        m_pEngineSettings->SetCanEnterOutOfRange();
                        if( bConfirm )
                            m_pEngineSettings->SetMustAskWhenOutOfRange();
                        else
                            m_pEngineSettings->SetDontAskWhenOutOfRange();
                    }
                    else {
                        m_pEngineSettings->SetCannotEnterOutOfRange();

                        // RHF INIC Sep 12, 2001
                        if( bConfirm )
                            m_pEngineSettings->SetMustAskWhenOutOfRange();
                        else
                            m_pEngineSettings->SetDontAskWhenOutOfRange();
                        // RHF END Sep 12, 2001
                    }

                    ResetAllVarsBehavior();
                }

#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_MESSAGES_DISPLAY:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Messages(DISPLAY) ON") : _T("Messages(DISPLAY) OFF");
#endif

                if( bSetOn )
                    m_pEngineSettings->SetDisplayMessageOn();
                else
                    m_pEngineSettings->SetDisplayMessageOff();
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_MESSAGES_ERRMSG:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Messages(ERRMSG) ON") : _T("Messages(ERRMSG) OFF");
#endif

                if( bSetOn )
                    m_pEngineSettings->SetErrmsgMessageOn();
                else
                    m_pEngineSettings->SetErrmsgMessageOff();
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_MOUSE:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Mouse ON") : _T("Mouse OFF");
#endif
                /* removed if( bSetOn )
                    m_pEngineSettings->SetMouseEnabled();
                else
                    m_pEngineSettings->SetMouseDisabled(); */
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_ENDGROUP:                     // victor Feb 16, 00
                pAction = ( bSetOn ) ? _T("EndGroup ON") : _T("EndGroup OFF");

                // allowed only in Path OFF environments
                if( !bPathOff )
                    issaerror( MessageType::Warning, 91112, pAction );
                else {
                    /* removed if( bSetOn )
                        m_pEngineSettings->SetCanUseEndGroup();
                    else
                        m_pEngineSettings->SetCannotUseEndGroup(); */
#ifdef  _DEBUG
                    TRACE( fAction, pAction );
#endif
                }
                break;

            case BEHAVIOR_ENDLEVEL:                     // victor Feb 16, 00
                pAction = ( bSetOn ) ? _T("EndLevel ON") : _T("EndLevel OFF");

                // allowed only in Path OFF environments
                if( bPathOff )
                    issaerror( MessageType::Warning, 91112, pAction );
                else {
                    if( bSetOn )
                        m_pEngineSettings->SetCanUseEndLevel();
                    else
                        m_pEngineSettings->SetCannotUseEndLevel();
#ifdef  _DEBUG
                    TRACE( fAction, pAction );
#endif
                }
                break;

            case BEHAVIOR_MSGNUMBER_DISPLAY:            // victor Jun 08, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("MsgNumber(DISPLAY) ON") : _T("MsgNumber(DISPLAY) OFF");
#endif

                /* removed if( bSetOn )
                    m_pEngineSettings->SetDisplayMsgNumberOn();
                else
                    m_pEngineSettings->SetDisplayMsgNumberOff(); */
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_MSGNUMBER_ERRMSG:             // victor Jun 08, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("MsgNumber(ERRMSG) ON") : _T("MsgNumber(ERRMSG) OFF");
#endif

                /* removed if( bSetOn )
                    m_pEngineSettings->SetErrmsgMsgNumberOn();
                else
                    m_pEngineSettings->SetErrmsgMsgNumberOff(); */
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_DATA:                  // victor Dec 18, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export Data ON") : _T("ExporT Data OFF");
#endif

                m_pEngineSettings->SetExportData( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_SPSS:                  // victor Dec 18, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export SPSS ON") : _T("ExporT SPSS OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportSPSS( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_SAS:                   // victor Dec 18, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export SAS ON") : _T("ExporT SAS OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportSAS( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_STATA:                 // victor Dec 18, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export STATA ON") : _T("ExporT STATA OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportSTATA( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_R:
                CleanExportTo();
                m_pEngineSettings->SetExportR( bSetOn );
                break;


            case BEHAVIOR_EXPORT_ALL:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export ALL ON") : _T("ExporT ALL OFF");
#endif

                m_pEngineSettings->SetExportSPSS( bSetOn );
                m_pEngineSettings->SetExportSAS( bSetOn );
                m_pEngineSettings->SetExportSTATA( bSetOn );
                m_pEngineSettings->SetExportR( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_ALL4:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export ALL4 ON") : _T("ExporT ALL4 OFF");
#endif
                CleanExportTo(); // RHF Aug 04, 2006

                m_pEngineSettings->SetExportSPSS( bSetOn );
                m_pEngineSettings->SetExportSAS( bSetOn );
                m_pEngineSettings->SetExportSTATA( bSetOn );
                m_pEngineSettings->SetExportCSPRO( bSetOn );// RHF Aug 03, 2006
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXPORT_ALL5:
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export ALL5 ON") : _T("ExporT ALL5 OFF");
#endif
                CleanExportTo(); // RHF Aug 04, 2006

                m_pEngineSettings->SetExportSPSS( bSetOn );
                m_pEngineSettings->SetExportSAS( bSetOn );
                m_pEngineSettings->SetExportSTATA( bSetOn );
                m_pEngineSettings->SetExportR( bSetOn );
                m_pEngineSettings->SetExportCSPRO( bSetOn );// RHF Aug 03, 2006
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;


            case BEHAVIOR_EXPORT_CSPRO:                // RHF Oct 08, 2004
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export CSPRO ON") : _T("ExporT CSPRO OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportCSPRO( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

                case BEHAVIOR_EXPORT_TABDELIM:               // RHF Oct 13, 2004
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export TABDELIM ON") : _T("ExporT TABDELIM OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportTabDelim( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

                case BEHAVIOR_EXPORT_COMMADELIM:               // RHF Oct 13, 2004
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export COMMA ON") : _T("ExporT COMMA OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportCommaDelim( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

                case BEHAVIOR_EXPORT_SEMICOLONDELIM:               // RHF Oct 13, 2004
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("Export SEMICOLON ON") : _T("ExporT SEMICOLON OFF");
#endif

                CleanExportTo();
                m_pEngineSettings->SetExportSemiColonDelim( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;


            case BEHAVIOR_CHECKRANGES:                  // victor Dec 07, 00
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("CheckRanges ON") : _T("CheckRanges OFF");
#endif

                m_pEngineSettings->SetAutoCheckRanges( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_SKIPSTRUC:                    // victor Mar 14, 01
            case BEHAVIOR_SKIPSTRUCIMPUTE:              // RHF Nov 09, 2001
#ifdef  _DEBUG
                if( iBehaviorItem == BEHAVIOR_SKIPSTRUC )
                    pAction = ( bSetOn ) ? _T("SkipStruc ON") : _T("SkipStruc OFF");
                else
                    pAction = ( bSetOn ) ? _T("SkipStruc ON IMPUTE") : _T("SkipStruc OFF");
#endif

                m_pEngineSettings->SetAutoSkipStruc( bSetOn );

                m_pEngineSettings->SetAutoSkipStrucImpute( iBehaviorItem == BEHAVIOR_SKIPSTRUCIMPUTE ); // RHF Nov 09, 2001


#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_EXIT: // RHF Aug 05, 2006
#ifdef _DEBUG
                pAction = ( bSetOn ) ? _T("Exit ON") : _T("Exit OFF");
#endif

                m_pEngineSettings->SetExitWhenFinish( bSetOn );
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            case BEHAVIOR_SPECIALVALUES: // 20090827
#ifdef  _DEBUG
                pAction = ( bSetOn ) ? _T("SpecialValues(Zero) ON") : _T("SpecialValues(Zero) OFF");
#endif

                m_pEngineSettings->SetTreatSpecialValuesAsZero(bSetOn);
#ifdef  _DEBUG
                TRACE( fAction, pAction );
#endif
                break;

            default:
                break;
        }

        switch( iBehaviorOpt ) {
        case BEHAVIOR_EXPORT_ITEMONLY:
            m_pEngineSettings->SetExportSubItemOnly( false );
            m_pEngineSettings->SetExportItemOnly( bSetOn );
            m_pEngineSettings->SetExportItemSubItem( false );
            break;
        case BEHAVIOR_EXPORT_SUBITEMONLY:
            m_pEngineSettings->SetExportSubItemOnly( bSetOn );
            m_pEngineSettings->SetExportItemOnly( false );
            m_pEngineSettings->SetExportItemSubItem( false );

            break;
        case BEHAVIOR_EXPORT_ITEMSUBITEM:
            m_pEngineSettings->SetExportSubItemOnly( false );
            m_pEngineSettings->SetExportItemOnly( false );
            m_pEngineSettings->SetExportItemSubItem( bSetOn );
            break;
        }

        m_pEngineSettings->SetExportForceANSI(( iBehaviorOpt2 & BEHAVIOR_EXPORT_UNICODE ) == 0); // 20120416
        m_pEngineSettings->SetExportCommaDecimal( ( iBehaviorOpt2 & BEHAVIOR_EXPORT_COMMA_DECIMAL ) != 0);
    }

    else // other SET (ACCESS, FIRST, LAST)
    {
        Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1);

        // the pre-8.0 node was a bit messed up
        Nodes::SetAccessFirstLast set_access_first_last_node;

        set_access_first_last_node.function_code = FunctionCode::SET_DICT_ACCESS_CODE;

        set_access_first_last_node.set_action = static_cast<SetAction>(pset_ot->len);
        ASSERT(set_access_first_last_node.set_action == SetAction::Access ||
               set_access_first_last_node.set_action == SetAction::First ||
               set_access_first_last_node.set_action == SetAction::Last );

        set_access_first_last_node.dictionary_symbol_index = pset_ot->type;

        set_access_first_last_node.dictionary_access = pset_ot->setinfo.behavior_item;

        return_code = exdictaccess(set_access_first_last_node);
    }


    return return_code;
}


typedef struct {
    int     m_iBehaviorItem;
    bool    m_bSetOn;
    bool    m_bConfirm;
} GROUPT_INFO;


int CIntDriver::exset_behavior( int iSymVar, void* pInfo ) {
    GROUPT_INFO*    pGroupTInfo=(GROUPT_INFO*) pInfo;
    VART*           pVarT=VPT(iSymVar);

    pVarT->m_iBehavior &= ~CANENTER_SET_VIA_VALIDATION_METHOD;

    switch( pGroupTInfo->m_iBehaviorItem ) {
        case BEHAVIOR_CANENTER_NOTAPPL:
            if( pGroupTInfo->m_bSetOn ) {
                pVarT->m_iBehavior |= CANENTER_NOTAPPL;

                if( pGroupTInfo->m_bConfirm )
                    pVarT->m_iBehavior &= ~CANENTER_NOTAPPL_NOCONFIRM;
                else
                    pVarT->m_iBehavior |= CANENTER_NOTAPPL_NOCONFIRM;
            }
            else {
                pVarT->m_iBehavior &= ~CANENTER_NOTAPPL;

                if( !pGroupTInfo->m_bConfirm ) // RHF Sep 12, 2001
                    pVarT->m_iBehavior |= CANENTER_NOTAPPL_NOCONFIRM;
                else
                    pVarT->m_iBehavior &= ~CANENTER_NOTAPPL_NOCONFIRM;
            }

            break;

        case BEHAVIOR_CANENTER_OUTOFRANGE:
            if( pGroupTInfo->m_bSetOn ) {
                pVarT->m_iBehavior |= CANENTER_OUTOFRANGE;
                if( pGroupTInfo->m_bConfirm )
                    pVarT->m_iBehavior &= ~CANENTER_OUTOFRANGE_NOCONFIRM;
                else
                    pVarT->m_iBehavior |= CANENTER_OUTOFRANGE_NOCONFIRM;
            }
            else {
                pVarT->m_iBehavior &= ~CANENTER_OUTOFRANGE;

                if( !pGroupTInfo->m_bConfirm ) // RHF Sep 12, 2001
                    pVarT->m_iBehavior |= CANENTER_OUTOFRANGE_NOCONFIRM;
                else
                    pVarT->m_iBehavior &= ~CANENTER_OUTOFRANGE_NOCONFIRM;
            }

            break;
    }

    return 0;
}

// Reset the behavior for all variables according to the global settings;
void CIntDriver::ResetAllVarsBehavior()
{
    bool    bCanEnterNotAppl    = m_pEngineSettings->CanEnterNotappl();
    bool    bCanEnterOutOfRange = m_pEngineSettings->CanEnterOutOfRange();
    bool    bAskWhenNotappl     = m_pEngineSettings->AskWhenNotappl();
    bool    bAskWhenOutOfRange  = m_pEngineSettings->AskWhenOutOfRange();

    for( VART* pVarT : m_engineData->variables )
    {
        pVarT->m_iBehavior = 0;

        if( bCanEnterNotAppl )
            pVarT->m_iBehavior |= CANENTER_NOTAPPL;
        if( !bAskWhenNotappl )
            pVarT->m_iBehavior |= CANENTER_NOTAPPL_NOCONFIRM;
        if( bCanEnterOutOfRange )
            pVarT->m_iBehavior |= CANENTER_OUTOFRANGE;
        if( !bAskWhenOutOfRange )
            pVarT->m_iBehavior |= CANENTER_OUTOFRANGE_NOCONFIRM;
    }
}

int CIntDriver::ExSetBehaviorList( int iBehaviorItem, LIST_NODE* pListNode, bool bSetOn, bool bConfirm ) {
    GROUPT_INFO     GroupTInfo;
    int             iNumChanges=0;

    GroupTInfo.m_iBehaviorItem = iBehaviorItem;
    GroupTInfo.m_bSetOn =  bSetOn;
    GroupTInfo.m_bConfirm = bConfirm;

    DICT*       pDicT;
    SECT*       pSecT;
    VART*       pVarT;
    GROUPT*     pGroupT;

    for( int i = 0; i < pListNode->iNumElems; i++ ) {
        int iSym = pListNode->iSym[i];
        Symbol* pSymbol = NPT(iSym);
        SymbolType eType = pSymbol->GetType();

        if( eType == SymbolType::Pre80Dictionary ) {
            pDicT=(DICT*) pSymbol;
            int     iSymSec = pDicT->SYMTfsec;

            while( iSymSec > 0 ) {
                pSecT = SPT(iSymSec);

                int iSymVar = pSecT->SYMTfvar;
                while( iSymVar > 0 ) {
                    pVarT = VPT(iSymVar);
                    iNumChanges += exset_behavior( iSymVar, &GroupTInfo );
                    iSymVar = pVarT->SYMTfwd;
                }

                iSymSec = pSecT->SYMTfwd;
            }
        }

        else if( eType == SymbolType::Form || eType == SymbolType::Group ) {
            if( eType == SymbolType::Group )
                pGroupT = (GROUPT*) pSymbol;
            else {
                FORM*   pFormT = (FORM*) pSymbol;
                int     iSymGroup = pFormT->GetSymGroup();
                pGroupT = (iSymGroup>0) ? GPT(iSymGroup) : NULL;
            }

            if( pGroupT != NULL )
                // RHF COM Aug 18, 2004 m_pEngineArea->GroupTtrip( pGroupT, NULL, (pGroupTripFunc2) pGroupT->Trip, 2, &GroupTInfo );
                m_pEngineArea->GroupTtrip( pGroupT, NULL, &GROUPT::Trip /*(pGroupTripFunc2) pGroupT->Trip*/, 2, &GroupTInfo ); // RHF Aug 18, 2004 .NET

        }
        else if( eType == SymbolType::Variable ) {
            pVarT= (VART*) pSymbol;
            int     iSymVar=pVarT->GetSymbolIndex();

            iNumChanges += exset_behavior( iSymVar, &GroupTInfo );
        }
        else
            ASSERT(0);
    }


    return iNumChanges;
}

int GROUPT::Trip( Symbol* sp, int iInfo, void* pInfo ) {
    int     iRet=0;
    if( sp->GetType() == SymbolType::Variable ) {
        VART*   pVarT= (VART*) sp;
        int     iSymVar=pVarT->GetSymbolIndex();

        if( iInfo == 1 )
            iRet = m_pEngineDriver->m_pIntDriver->exset_attr( iSymVar, 0, (SET_ATTR_NODE*) pInfo );

        else if( iInfo == 2 ) // 20130314 for some reason set behavior wasn't enabled for forms or groups
            iRet = m_pEngineDriver->m_pIntDriver->exset_behavior(iSymVar,pInfo);
    }

    return iRet;
}


double CIntDriver::exmessageoverrides(int iExpr) // 20100518
{
    const auto& pset_ac = GetNode<ACCESS_NODE>(iExpr);
    ASSERT(pset_ac.st_code == FNMESSAGEOVERRDIES_CODE);

    MessageOverrides message_overrides;

    // message overrides are being turned off
    if( pset_ac.idic == -1 && pset_ac.iidx == -1 )
    {
        message_overrides.mode = MessageOverrides::Mode::NoOverride;
    }

    // message overrides are being turned to system controlled mode
    else if( pset_ac.idic == -2 && pset_ac.iidx == -2 )
    {
        message_overrides.mode = MessageOverrides::Mode::SystemControlled;
    }

    // message overrides are being turned to operator controlled mode
    else
    {
        message_overrides.mode = MessageOverrides::Mode::OperatorControlled;

        // a custom error message is getting passed
        if( pset_ac.idic >= 0 )
            message_overrides.clear_text = EvalAlphaExpr(pset_ac.idic);

        // a custom keystroke is getting passed
        if( pset_ac.iidx >= 0 )
        {
            WPARAM keystroke = evalexpr<WPARAM>(pset_ac.iidx);

            if( keystroke > 0 && keystroke < 256 ) // more robust checking could be done...
                message_overrides.clear_key_code = keystroke;
        }
    }

#ifdef WIN_DESKTOP
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_SET_MESSAGE_OVERRIDES, (WPARAM)&message_overrides);
#endif

    return 0;
}
