//---------------------------------------------------------------------
//
//  compEXPORT  compiler for 'export' sentences
//
//---------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Export.h"
#include "COMPILAD.H"
#include <zEngineO/File.h>
#include <zToolsO/Tools.h>
#include <zLogicO/BaseCompilerSettings.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

int CEngineCompFunc::compexport() {
    clearSyntaxErrorStatus();

    // RHF INIC Jul 01, 2005
    if( GetIdChanger() ) {
        return( SetSyntErr(1046) );
    }
    // RHF END Jul 01, 2005

    if( Appl.ApplicationType != ModuleType::Batch )
        return( SetSyntErr(31002) );      // only in Batch appls

    //  NEW_VERSION                     //      <begin> // victor Jun 20, 00
    if( !m_pEngineArea->IsLevel( InCompIdx ) || LvlInComp < 1 /* RHF COM Oct 20, 2004 || ProcInComp != PROCTYPE_POST*/ )
        return( SetSyntErr(31003) );      // export cannot appear here
    //  NEW_VERSION                     //      <end>   // victor Jun 20, 00

    int     iptnode = Prognext;
#ifdef GENCODE
    EXPORT_NODE* pExpoNode = (EXPORT_NODE*) (PPT(Prognext));
    if( Flagcomp ) {
        OC_CreateCompilationSpace(sizeof(EXPORT_NODE) / sizeof(int));
        pExpoNode->st_code = EXPORT_CODE;
    }
#endif

    // creating this export' description
    CExport* pCurExport = new CExport;
#ifdef GENCODE
    pExpoNode->m_pExport = (void*) pCurExport;


#endif

    pCurExport->SetEngineArea( m_pEngineArea );
    m_pEngineArea->SetCurExport( pCurExport );

    if( GetSyntErr() == 0 ) {
        // --- creating export program-strip
        if( !pCurExport->CreateProgStrip() )
            SetSyntErr(31001);            // no memory for export tables
        else {
            // compile this export sentence
            comp_export();
        }
    }

    if( GetSyntErr() != 0 ) {
        pCurExport->DeleteProgStrip();
        delete pCurExport;
        m_pEngineArea->SetCurExport( NULL );
        return GetSyntErr();
    }

    // closing this export' compilation
        //  exp_dump();                         // TEST TRACE ONLY
    m_pEngineArea->ExportAdd( pCurExport );

    m_pEngineSettings->m_bHasExport = true;                  // mark presence of Export sentence

    return iptnode;
}

int CEngineCompFunc::comp_export()
{
    CExport* pCurExport = m_pEngineArea->GetCurExport();
    EXP_HEADER_NODE* pHeadNode = pCurExport->GetHeadNode();

    // setup header identification
    pHeadNode->m_iNodeType     = Exp_HCODE;

    // fill export header
    pHeadNode->m_iExportModel       = Exp_mENSEMBLED;
    pHeadNode->m_iExportLevel       = LvlInComp;
    pHeadNode->m_iExportAppLevel    = LvlInComp; // RHF INIC Feb 25, 2005

    pHeadNode->m_bHasCaseId      = false;

    pHeadNode->m_iExportProcSymbol = InCompIdx;
    pHeadNode->m_iExportProcType   = ProcInComp;

    pHeadNode->m_iNumCaseId = 0;        // # of vars in CASE_ID
    pHeadNode->m_iLenCaseId      = 0; // length of CASE_ID list
    pHeadNode->m_iLenRecId       = 0;
    pHeadNode->m_iLenCaseIdUnicode = 0;

    //TO Clause now is mandatory
    if( NextKeyword({ _T("TO") }) != 1 )
        IssueError(31062);

    NextToken();
    IssueErrorOnTokenMismatch(TOKFILE, 31060); // FILE object expected

    pCurExport->m_iFileSymbol = Tokstindex;
    GetSymbolLogicFile(Tokstindex).SetUsed();

    // default to reading sections (over groups) during the export compilation
    Logic::BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
    compiler_settings_modifier.SetNextTokenPreferredSymbolType(SymbolType::Section);

    NextToken();

    // export of specific models
    if( pHeadNode->m_iExportModel == Exp_mENSEMBLED )
        exEnsembledModel();
    else
        SetSyntErr(31070);                // invalid model

    if( GetSyntErr() == 0 )
        CompExpDelimiterNode( Exp_EOCASE );

    if( GetSyntErr() == 0 && pCurExport->m_aSymbols.GetSize() == 0 )
        SetSyntErr(31100);                // empty list

    return 0;
}

int CEngineCompFunc::excase_id() {
    // excase_id: compile CASE_ID - Dict vars required
    CExport*            pCurExport = m_pEngineArea->GetCurExport();
    EXP_HEADER_NODE*    pHeadNode = pCurExport->GetHeadNode();

    NextToken();
    if( Tkn != TOKLPAREN )
        return( SetSyntErr(517) );
    NextToken();

    pHeadNode->m_iLenCaseId = 0;
    pHeadNode->m_iLenCaseIdUnicode = 0;
    pHeadNode->m_bHasCaseId = true;

    int aLenByLevel[MaxNumberLevels];

    for( size_t i=0; i < MaxNumberLevels; i++ )
        aLenByLevel[i] = 0;

    while( true ) {
        if( Tkn == TOKRPAREN )
            break;
        if( !CompExpIsValidVar(false) ) { // // RHF Feb 21, 2005 Case_id must allow variable from a higher level
            if( GetSyntErr() == 0 )
                SetSyntErr(31024);
            return GetSyntErr();  // only vars allowed in CASE_ID
        }
        if( pHeadNode->m_iNumCaseId >= MAXQIDVARS )
            return( SetSyntErr(31025) );  // too many vars in CASEID list

        int     iSymVar = Tokstindex;
        VART*   pVarT = VPT(iSymVar);
        int     isec = pVarT->GetOwnerSec();
        SECT*   pSecT = SPT(isec);

        CString csItemName = WS2CS(pVarT->GetName());

        // RHF INIC Feb 17, 2005 NEW_EXPORT

        //id item with occurrence
        CString     csOccExpr;
        if( !CompConstantList( pVarT, csOccExpr ) )
            return GetSyntErr();
        // RHF END Nov 05, 2004

        MVAR_NODE*  pMVarNode=NULL;
        if( pVarT->IsArray() ) {
            int iOccExpr = varsanal( pVarT->GetFmt() );

            //Export is only allowed in Level procedures (see error 31003),
            //so if pMVarNode->m_iVarSubindex=='G' the index can't be solved at runtime
#ifdef GENCODE
            pMVarNode = (MVAR_NODE*)PPT(iOccExpr);

            bool    bOkIndex=true;
            for( int i =0; bOkIndex && i < pVarT->GetNumDim(); i++ ) {
                if( pMVarNode->m_iVarSubindexType[i] != MVAR_EXPR )
                    bOkIndex = false;
            }

            if( !bOkIndex ) {
                return( SetSyntErr(31034) );
            }
#endif
            pCurExport->m_aCaseIdOccExpr[pHeadNode->m_iNumCaseId] = iOccExpr;
            pCurExport->m_aCaseIdOccExprProcSymbol[pHeadNode->m_iNumCaseId] = InCompIdx; // RHF Jun 08, 2005
        }
        else {
            pCurExport->m_aCaseIdOccExpr[pHeadNode->m_iNumCaseId] = -1;
            pCurExport->m_aCaseIdOccExprProcSymbol[pHeadNode->m_iNumCaseId] = InCompIdx; // RHF Jun 08, 2005

            NextToken();
        }

        pCurExport->m_aCaseIdOccExprString[pHeadNode->m_iNumCaseId] = csOccExpr;

        if( !csOccExpr.IsEmpty() )
            csItemName += csOccExpr;
        // RHF END Feb 17, 2005 NEW_EXPORT

        int iDummy;
        if( pCurExport->m_aItemNames.Lookup( csItemName, iDummy ) )
            return( SetSyntErr(31110) );
        pCurExport->m_aItemNames.SetAt( csItemName, 0 );

        bool    bIsSubItem=(pVarT->GetOwnerSymItem()>0);
        if( bIsSubItem )
            return( SetSyntErr(31029) );  // ... must be item independent of SubItemOnly/ItemOnly/SubItem option

        // add var to CASE_ID list
        pVarT->SetUsed( true );

        //if( CompExpIsExternalOrWork( iSymVar ) )
        //    pHeadNode->bGenRecTypes = true;
        pCurExport->m_iCaseIdItems[pHeadNode->m_iNumCaseId] = iSymVar;

        pHeadNode->m_iLenCaseId += pVarT->GetLength();
        pHeadNode->m_iLenCaseIdUnicode += pVarT->GetLength() * ( pVarT->IsAlpha() ? 4 : 1 ); // GHM 20130502 in case any of the ID fields are alphas
        pHeadNode->m_iNumCaseId++;



        ASSERT( pSecT->GetLevel(true) >= 1 );
        aLenByLevel[pSecT->GetLevel(true)-1] += pVarT->GetLength();

        if( aLenByLevel[pSecT->GetLevel(true)-1] >= MAX_DICTKEYSIZE )
            return( SetSyntErr(31028) );

        // RHF Feb 17,2005 NEW_EXPORT nexttkn already done
        //NextToken();

        while( Tkn == TOKCOMMA )
            NextToken();
    }

    if( Tkn != TOKRPAREN )
        return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED) );
    NextToken();

        // if CASE_ID() used, generate default ID. This step will be done at runtime because Cspro export
        // needs to know others EXPORT (to the same file) to generate the caseid
        //pCurExport->MakeDefaultCaseId(pCurExport->m_pHeadNode->m_iExportLevel);

    return 0;
}

CString MakeOccString( CArray<double,double>& aValues ) {
        CString csValue;
        CString csOcc;
        for( int i=0; i < aValues.GetSize(); i++ ) {
                csValue.Format( _T("_%d"), (int) aValues.GetAt(i) );
                csOcc += csValue;
        }

        return csOcc;
}


bool CEngineCompFunc::CompConstantList( VART* pVarT, CString& csOccExpr ) {
    CArray<double,double> aValues;
    bool                  bOnlyCte = true;
    csOccExpr.Empty();

    // Pre-scan looking for constants
    if( pVarT->IsArray() ) {
        // PUSH Lexer State
        int iOldFlagcomp   = Flagcomp;
        int iOldTokstindex = Tokstindex; // RHF Feb 10, 2005

        Flagcomp = FALSE;
        MarkInputBufferToRestartLater();

        NextToken();
        if( Tkn == TOKLPAREN ) {
            do {
                NextToken();
                if( Tkn == TOKCTE ) {
                    if( (int) Tokvalue != Tokvalue ) {
                        bOnlyCte = false;
                        SetSyntErr(82); //integer constant expected
                        return false;
                    }
                    else {
                        double dCteValue=Tokvalue;
                        aValues.Add(dCteValue);
                        NextToken();
                    }
                }
                else
                    bOnlyCte = false;

            } while( Tkn == TOKCOMMA && bOnlyCte );

            if( Tkn != TOKRPAREN )
                bOnlyCte = false;
        }
        else
            bOnlyCte = false;

        // POP Lexer State
        // get ready to read input again
        RestartFromMarkedInputBuffer();

        Tokstindex = iOldTokstindex; // RHF Feb 10, 2005
        Flagcomp = iOldFlagcomp;

        if( !bOnlyCte )
            aValues.RemoveAll();
    }

    if( bOnlyCte )
        csOccExpr = MakeOccString( aValues );

    return true;
}

int CEngineCompFunc::exEnsembledModel( void ) {         // victor Dec 12, 00
    // exEnsembledModel: compiler for ENSEMBLED model - compiles one RECORD clause
    CExport*         pCurExport = m_pEngineArea->GetCurExport();
    EXP_HEADER_NODE* pHeadNode  = pCurExport->GetHeadNode();

    // preparing "ensemble" node
    EXP_ENSEMBLE_NODE*  pEnsembNode = (EXP_ENSEMBLE_NODE*) CompExpNewNode( sizeof(EXP_ENSEMBLE_NODE) ); // victor Oct 20, 00

    if( pEnsembNode == NULL )                           // victor Oct 20, 00
        return 0;                       // too many elements in export list

    pEnsembNode->m_iNodeType = Exp_ENSEMBLE;
    pEnsembNode->m_pEndNode  = NULL;     // address of the end-of-ensemble

    int  iRecNameExpr = -1;
    int  iRecTypeExpr = -1;
    bool bHasRecName = false;
    bool bHasRecType = false;
    bool bIsSymbolRecName = false;
    bool bIsSymbolRecType = false;

    bool bHasSomeItem=false;
    bool bCaseIdAfterRecType=false;

    while( Tkn != TOKSEMICOLON && Tkn != TOKEOP ) {
        bool    bGetNextToken = true;

        if( Tkn == TOKCOMMA )
            ;
        else if( Tkn == TOKCASEID ) {
            if( bHasSomeItem )
                return( SetSyntErr(31098) );

            if( !CompCaseId( pHeadNode ) )
                return 0;
            bGetNextToken = false;

            bCaseIdAfterRecType = bHasRecType;
        }
        else if( Tkn == TOKRECNAME || Tkn == TOKRECTYPE ) {
                bool    bIsRecName=(Tkn == TOKRECNAME);

                if( Tkn == TOKRECNAME ) {
                    if( bHasSomeItem )
                        return( SetSyntErr(31098) );

                    if( bHasRecName  )
                        return( SetSyntErr(31094) );
                    bHasRecName = true;
                }
                else {
                    if( bHasSomeItem )
                        return( SetSyntErr(31098) );
                    if( bHasRecType )
                        return( SetSyntErr(31096) );
                    bHasRecType = true;
                }

                NextToken();
                if( Tkn != TOKLPAREN )
                    return( SetSyntErr(517) );
                NextToken();

                int iSymSec=0;
                if( Tkn == TOKSECT ) {
                    iSymSec = Tokstindex;
                    NextToken();
                }
                else if( !IsCurrentTokenString() )
                    return( SetSyntErr(31092) ); // Alpha expresion expected

                if( iSymSec != 0 ) {
                    // RHF COM Nov 09, 2004 if( Tkn == TOKRECNAME ) {
                    if( bIsRecName ) {// RHF Nov 09, 2004
                        iRecNameExpr = iSymSec;
                        bIsSymbolRecName = true;
                    }
                    else {
                        iRecTypeExpr = iSymSec;
                        bIsSymbolRecType = true;
                    }
                }
                else {
                    if( bIsRecName )
                        iRecNameExpr = CompileStringExpression();
                    else
                        iRecTypeExpr = CompileStringExpression();
                }

                if( GetSyntErr() != 0 )
                    return 0;

                if( Tkn != TOKRPAREN )
                    return( SetSyntErr(17) );
                //NextToken();
            }
        else if( Tkn == TOKVAR ) {
            bHasSomeItem = true;
            if( !CompExpIsValidVar(true) ) {
                if( GetSyntErr() == 0 )
                    SetSyntErr(31083);
                return GetSyntErr();  // Variable expected
            }

            int     iSymVar = Tokstindex;
            VART*   pVarT = VPT(iSymVar);

            pVarT->SetMarked( false );
            pVarT->SetUsed( true );

            pCurExport->m_aSymbols.Add( iSymVar );

            /* RHF COM INIC Nov 05, 2004
            CString csItemName=pVarT->GetName();
            int             iDummy;
            if( pCurExport->m_aItemNames.Lookup( csItemName, iDummy ) )
            return( SetSyntErr(31110) );
            pCurExport->m_aItemNames.SetAt( csItemName, 0 );
            RHF COM END Nov 05, 2004 */

            // RHF INIC Nov 05, 2004
            // Pre-scan looking for constants
            CString     csOccExpr;
            if( !CompConstantList( pVarT, csOccExpr ) )
                return GetSyntErr();
            // RHF END Nov 05, 2004

            MVAR_NODE* pMVarNode = 0;
            if( pVarT->IsArray() ) {
                int iOccExpr = varsanal( pVarT->GetFmt() );

                // when exporting data it is not an error not to have
                // enough subindexes because the export command will
                // complete what was not specified.
                //   varsanal() sometimes produces an error code
                // [ERROR_NOT_ENOUGH_SUBINDEXES], but we change it here
                // to prevent further error handling behaviour.
                // rcl, Feb 2005
                if( GetSyntErr() == ERROR_NOT_ENOUGH_SUBINDEXES )
                    clearSyntaxErrorStatus();

                pMVarNode = (MVAR_NODE*) PPT(iOccExpr);

                pCurExport->m_aOccExpr.Add(iOccExpr);
            }
            else {
                pCurExport->m_aOccExpr.Add(-1);
                NextToken();
            }

            pCurExport->m_aOccProcSymbols.Add( InCompIdx ); // RHF Jun 08, 2005
            bGetNextToken = false;

            pCurExport->m_aOccExprString.Add(csOccExpr);

            // RHF INIC Nov 05, 2004
            CString csItemName = WS2CS(pVarT->GetName());
            if( !csOccExpr.IsEmpty() )
                csItemName += csOccExpr;

            int  iDummy   = 0;
            bool bHasName = pCurExport->m_aItemNames.Lookup( csItemName, iDummy ) != 0;
            if( bHasName )
                return( SetSyntErr(31110) );

            pCurExport->m_aItemNames.SetAt( csItemName, 0 );
            // RHF END Nov 05, 2004

            // RHF INIC Oct 19, 2004
            bool bIsSubItem = (pVarT->GetOwnerSymItem()>0);
            bool bIsItem = !bIsSubItem;

            //Generate nodes for subitems multiples because they can be used at runtime
            if( bIsItem ) {
                VART*   pMultSubItem=pVarT->GetNextSubItem();

                while( pMultSubItem != NULL ) {
                    // Multiple
                    if( pMultSubItem->GetMaxOccs() >= 2 ) {
                        int iSymMultSubItem=pMultSubItem->GetSymbolIndex();

                        int iMultSubItemOccExpr = genMVARNode( iSymMultSubItem, pMVarNode );
                        MVAR_NODE* pMultSubItemMVarNode = (MVAR_NODE*) PPT(iMultSubItemOccExpr);

                        int iLastDim = pMultSubItem->GetNumDim() - 1; // This is the last dimension

                        GROUPT* pGrpT = pMultSubItem->GetParentGPT();

                        ASSERT( pGrpT );
                        int iGroupIndex = ( pGrpT != NULL ) ? pGrpT->GetSymbolIndex() : 0;

                        FillImplicitIndex( pMultSubItemMVarNode, iSymMultSubItem, iLastDim, iGroupIndex );

                        pCurExport->m_aOcExprForMultSubItems.SetAt( iSymMultSubItem, iMultSubItemOccExpr );

                        pCurExport->m_aOcExprStringForMultSubItems.SetAt( iSymMultSubItem, csOccExpr );
                    }

                    pMultSubItem = pMultSubItem->GetNextSubItem();
                }
            }
            // RHF END Oct 19, 2004
        }

        else if( Tkn == TOKDICT_PRE80 ) {
            bHasSomeItem = true;

            DICT*   pDicT = DPT(Tokstindex);

            if( pDicT->GetSubType() != SymbolSubType::Input )
                return( SetSyntErr(31045) );

            int     iSymSec = pDicT->SYMTfsec;
            while( iSymSec > 0 ) {
                SECT*   pSecT = SPT(iSymSec);

                if( !pSecT->IsCommon() && pSecT->GetLevel(true) <= pHeadNode->m_iExportLevel ) {
                    if( !AddExportSection( iSymSec, true, -1 ) )
                        return 0;
                }

                iSymSec = pSecT->SYMTfwd;
            }
        }
        else if( Tkn == TOKSECT ) {
            bHasSomeItem = true;
            int     iSymSec    = Tokstindex;
            SECT*   pSecT      = SPT(iSymSec);
            int     iMaxOccs   = pSecT->GetMaxOccs();

            if( pSecT->GetLevel(true) > pHeadNode->m_iExportLevel ) {
                return( SetSyntErr(31000) );
            }

            //if( CompExpIsExternalOrWork( iSymSec ) )
            //      pHeadNode->bGenRecTypes = true;

            int             iSecOccExpr=-1;

            NextToken();
            bGetNextToken = false;

            if( Tkn == TOKLPAREN ) {        // (: there must be an index-expression
                if( pSecT->GetMaxOccs() < 2 )
                    return( SetSyntErr(31030) );

                NextToken();

                iSecOccExpr = exprlog();

                if( GetSyntErr() != 0 )
                    return 0;

                if( Tkn != TOKRPAREN )
                    return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED) );
                NextToken();

                iMaxOccs = 1;               // section processed as single-occ
            }

            // RHF INIC May 16, 2001
            bool    bInclude=(Tkn==TOKINCLUDE);

            int     iSymVar = pSecT->SYMTfvar;
            while( iSymVar > 0 ) {
                VART*   pVarT = VPT(iSymVar);

                // RHF INIC Dec 23, 2003
                /* RHF COM INIC Oct 19, 2004

                bool    bIsTrueItem = !pVarT->GetOwnerSymItem();
                if( bIsTrueItem && pVarT->GetNextSubItem() != NULL ) //skip items when they have subitems
                pVarT->SetMarked( true );
                else
                RHF COM END Oct 19, 2004 */
                // RHF END Dec 23, 2003

                pVarT->SetMarked( bInclude ? true: false ); // Is better to be sure the flag is false when exclude or not clause used.
                iSymVar = pVarT->SYMTfwd;
            }

            if( Tkn == TOKINCLUDE || Tkn == TOKEXCLUDE ) {
                NextToken();
                if( Tkn != TOKLPAREN )
                    return( SetSyntErr(517) );
                NextToken();

                int     iNumItems=0;
                while( true ) {
                    if( Tkn == TOKRPAREN )
                        break;
                    if( !CompExpIsValidVar(true) ) {
                        if( GetSyntErr() == 0 )
                            SetSyntErr(11);
                        return GetSyntErr();  // Variable expected
                    }

                    iSymVar = Tokstindex;
                    VART*   pVarT = VPT(iSymVar);

                    if( iSymSec != pVarT->GetOwnerSec() )
                        return( SetSyntErr(31085) );  // Variable doesn't belong to the section

                    pVarT->SetMarked( bInclude ? false : true );
                    iNumItems++;

                    NextToken();
                    while( Tkn == TOKCOMMA )
                        NextToken();
                }

                if( iNumItems == 0 )
                    return( SetSyntErr(31086) );

                if( Tkn != TOKRPAREN )
                    return( SetSyntErr(ERROR_RIGHT_PAREN_EXPECTED) );

                NextToken();
            }
            // RHF END May 16, 2001

            if( !AddExportSection( iSymSec, false, iSecOccExpr ) )
                return 0;

        } // Section

        else {
            return( SetSyntErr(31083), 0 );
        }

        if( bGetNextToken )
            NextToken();
    }

    pHeadNode->m_iRecNameExpr = iRecNameExpr;
    pHeadNode->m_iRecTypeExpr = iRecTypeExpr;
    pHeadNode->m_bIsSymbolRecName = bIsSymbolRecName;
    pHeadNode->m_bIsSymbolRecType = bIsSymbolRecType;
    pHeadNode->m_bCaseIdAfterRecType = bCaseIdAfterRecType;

    // saves the address of the end-of-ensemble
    pEnsembNode->m_pEndNode = CompExpDelimiterNode( Exp_EOENSEMBLE );

    return 0;
}

bool CEngineCompFunc::AddExportSection( int iSymSec, bool bUseAll, int iSecOccExpr ) {
        CExport* pCurExport = m_pEngineArea->GetCurExport();
        int iDummy;
        SECT* pSecT = SPT(iSymSec);
        VART* pVarT;

        if( !pSecT->GetDicT()->GetDataDict()->GetAllowExport() )
            IssueError(31007, pSecT->GetDicT()->GetName().c_str());

        // RHF INIC Oct 04, 2004
        int iSymVar = pSecT->SYMTfvar;

        if( bUseAll ) {
                while( iSymVar > 0 ) {
                        pVarT = VPT(iSymVar);
                        pVarT->SetMarked( false );
                        iSymVar = pVarT->SYMTfwd;
                }
        }

        iSymVar = pSecT->SYMTfvar;
        while( iSymVar > 0 ) {
            pVarT = VPT(iSymVar);
        if( bUseAll || !pVarT->IsMarked() ) {
            pVarT->SetUsed( true );
            pCurExport->m_aSymbols.Add(iSymVar);
            CString csItemName = WS2CS(pVarT->GetName());

            if( pCurExport->m_aItemNames.Lookup( csItemName, iDummy ) ) {
                SetSyntErr(31110);
                return false;
            }
            pCurExport->m_aItemNames.SetAt( csItemName, 0 );

            if( pVarT->IsArray() ) {
                int iOccExpr = FillImplicitIndexes( iSymVar );

                pCurExport->m_aOccExpr.Add(iOccExpr);

#ifdef GENCODE
                if( Flagcomp ) {
                    //Replace first dimension when section has an explicit subindex
                    if( iSecOccExpr != -1 && pVarT->GetDimType(0) == CDimension::Record ) {
                        MVAR_NODE*  pMVarNode=(MVAR_NODE*)PPT(iOccExpr);
                        pMVarNode->m_iVarSubindexExpr[0] = iSecOccExpr;
                        pMVarNode->m_iVarSubindexType[0] = MVAR_EXPR;
                    }
                }
#endif
            }
            else
                pCurExport->m_aOccExpr.Add(-1);

            pCurExport->m_aOccProcSymbols.Add( InCompIdx ); // RHF Jun 08, 2005
            pCurExport->m_aOccExprString.Add(_T(""));

        } else
            pVarT->SetMarked( false );
        iSymVar = pVarT->SYMTfwd;
    }
    // RHF END Oct 04, 2004

    return true;
}

///////////////////////////////////////////////////////////////////////
//
// --- export compiler - nodes generation
//
///////////////////////////////////////////////////////////////////////

char* CEngineCompFunc::CompExpDelimiterNode( int iNodeType ) {
    // CompExpDelimiterNode: single-csprochar delimiters
    char*   pExpoNode = (char*) CompExpNewNode( sizeof(char) ); // victor Oct 20, 00

    if( pExpoNode != NULL )
        *pExpoNode = iNodeType;

    return pExpoNode;                                   // victor Dec 12, 00
}

char* CEngineCompFunc::CompExpNewNode( int iNodeSize ) {
    // CompExpNewNode: return a pointer to a new node to be inserted into export program-strip (NULL if impossible)
    // ... formerly 'ex_canadd', now 'CompExpNewNode'    // victor Oct 20, 00
    CExport*            pCurExport = m_pEngineArea->GetCurExport();
    char*               pNewNode = NULL;
    bool                bCanAdd = pCurExport->CanAddNode( iNodeSize );

    if( !bCanAdd ) {
        pCurExport->EnlargeProgStrip();

        // check again
        bCanAdd = pCurExport->CanAddNode( iNodeSize );
    }

    if( bCanAdd ) {
        pNewNode = pCurExport->GetNextNode();           // victor Oct 20, 00
        pCurExport->AdvanceNodeIndex( iNodeSize );
    }
    else
        SetSyntErr(31005);                // too many elements in export list

    return pNewNode;                                    // victor Oct 20, 00
}

///////////////////////////////////////////////////////////////////////
//
// --- export compiler - miscellaneous checking
//
///////////////////////////////////////////////////////////////////////

bool CEngineCompFunc::CompExpIsExternalOrWork( int iSymbol ) {
    csprochar    cInDic = CompExpWhichDicType( iSymbol );

    return( cInDic == ExpDicTypeExternal || cInDic == ExpDicTypeWork );
}

csprochar CEngineCompFunc::CompExpWhichDicType( int iSymbol ) {
    // CompExpWhichDicType: in Input/External/Work' dicts, or not in a dict
    csprochar    cInDic = ExpDicTypeNone;    // assuming not in a Dict
    int     i = iSymbol;                                // RHF Apr 07, 2000
    int     iSymDic = m_pEngineArea->ownerdic( i );

    if( iSymDic > 0 ) {
        DICT*   pDicT = DPT(iSymDic);

        if( pDicT->GetSubType() == SymbolSubType::Input )
            cInDic = ExpDicTypeInput;
        else if( pDicT->GetSubType() == SymbolSubType::External )
            cInDic = ExpDicTypeExternal;
        else if( pDicT->GetSubType() == SymbolSubType::Work )
            cInDic = ExpDicTypeWork;
    }

    return cInDic;
}

// CASE_ID process: Dict vars are used
bool CEngineCompFunc::CompCaseId(EXP_HEADER_NODE* pHeadNode ) {
    bool            bRet=false;

    ASSERT( Tkn == TOKCASEID );
    if( Tkn == TOKCASEID ) {
        if( pHeadNode->m_bHasCaseId ) {
            SetSyntErr(31006);  // only one CASE_ID allowed in Export
        }
        else {
            excase_id();          // compile CASE_ID
            if( GetSyntErr() == 0 )
                bRet = true;
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- program-strip
//
//////////////////////////////////////////////////////////////////////////////

bool CExport::CreateProgStrip( void ) {
    int     iSize = sizeof(EXP_HEADER_NODE) + m_iExpLstSlot;

    m_pExpLstBase = (char*) calloc( iSize, sizeof(char) );

    bool    bDone = ( m_pExpLstBase != NULL );

    if( bDone ) {
        m_iExpLstNext = sizeof(EXP_HEADER_NODE);
        m_iExpLstSize = iSize;
        SetHeadNode();
    }

    return bDone;
}


bool CExport::EnlargeProgStrip( void ) {
    bool    bDone = true;

    if( m_pExpLstBase == NULL )
        bDone = CreateProgStrip();
    else {
        issaerror( MessageType::Abort, 31005 );                      // victor Oct 20, 00

        // forbidden code - not working properly<begin> // victor Oct 20, 00
        int     iNewSize = m_iExpLstSize + m_iExpLstSlot;
        char*   pNewBase = (char*) calloc( iNewSize, sizeof(char) );

        bDone = ( pNewBase != NULL );

        if( bDone ) {
            memmove( pNewBase, m_pExpLstBase, m_iExpLstSize );
            free( m_pExpLstBase );
            m_iExpLstSize = iNewSize;
            m_pExpLstBase = pNewBase;
            SetHeadNode();
        }
        // forbidden code - not working properly<end>   // victor Oct 20, 00
    }

    return bDone;
}


////////////////////////////////////////////////////////////////////////////
//
// --- engine links
//
////////////////////////////////////////////////////////////////////////////

void CExport::SetEngineArea(CEngineArea* pEngineArea)
{
    m_pEngineArea   = pEngineArea;
    m_pEngineDriver = pEngineArea->m_pEngineDriver;
    m_engineData    = &m_pEngineArea->GetEngineData();
}

//////////////////////////////////////////////////////////////////////////////
//
// --- constructor & initialization
//
//////////////////////////////////////////////////////////////////////////////

void CExport::Init( void ) {
    // --- export gate & export interface
    SetExportInactive();                // not yet open
    SetExpoName( _T(""), true );                  // shared export-file-name
    // named output (several exports)                   // victor Dec 18, 00
    m_iFileSymbol      = -1;

    // --- exporting options
    SetExportToDat( false );            // export data to data-file
    SetExportToSPSS( false );           // export description to SPSS
    SetExportToSAS( false );            // export description to SAS
    SetExportToSTATA( false );          // export description to STATA
    SetExportToCSPRO( false );          // export description to CSPRO
    SetExportToTabDelim( false );       // export description to Tab Delimited
    SetExportToCommaDelim( false );     // export description to Comma Delimited
    SetExportToSemiColonDelim( false ); // export description to Semicolon delimited
    SetExportToR( false );

    SetExportItemOnly( false );
    SetExportSubItemOnly( false );
    SetExportItemSubItem( false );

    m_iExporItem = EXPORT_ITEMSUBITEM;

    // --- output files
    m_pFileDat       = NULL;            // exported data-file
    m_bNeedClose     = true;
    m_pFileSPSS      = NULL;            // SPSS codebook-file
    m_pFileSAS       = NULL;            // SAS codebook-file
    m_pFileSTATAdct  = NULL;            // STATA DCT-file
    m_pFileSTATAdo   = NULL;            // STATA DO-file
    m_pRecordCSPRO   = NULL;            // CSPRO record
    m_pFileR         = NULL;            // R codebook-file
    m_iCurOcc = 0;
    m_csLastCaseId.Empty();

    // --- program-strip
//  m_iExpLstSlot    = 8192;            // slot size for program-strip
    m_iExpLstSlot    = 524288;          // slot size for program-strip
    m_pExpLstBase    = NULL;            // the program-strip itself
    m_iExpLstSize    = 0;               // total of bytes in program-strip
    m_iExpLstNext    = 0;               // next byte free in program-strip

    // --- record-area
    m_pExpRecArea    = NULL;            // the record-area itself
    m_iExpRecMax     = 0;               // size of record-area
    m_iExpRecNext    = 0;               // next byte free in record-area

    // --- generating descriptions
    m_pHeadNode      = NULL;
    m_bPendingSlash  = false;
    m_bCaseIdReady   = false;           // case-id already generated (or not needed)
    m_cNameSeparator = _T('_');

    *m_pszSectionCode = 0;

    // --- array of categories for one variable
    m_papVarCategs = nullptr;

    SetNamesExported(false); // RHF Feb 03, 2005

    SetRemoveFiles( false ); // RHF Feb 03, 2005

    m_iCaseIdItems.SetSize(MAXQIDVARS);

    // RHF INIC Feb 14, 2005
    m_aCaseIdOccExpr.SetSize(MAXQIDVARS);
    m_aCaseIdOccExprString.SetSize(MAXQIDVARS);
    // RHF END Feb 14, 2005

    m_aCaseIdOccExprProcSymbol.SetSize(MAXQIDVARS); // RHF Jun 08, 2005

    // RHF INIC Feb 17, 2005 NEW_EXPORT
    SetNextExport( NULL );
    SetUseMergedArray( false );
    SetRecordNumber( -1 );

    m_bNewExportRecord = false;
    m_iRunExportRecordNumber = -1;
    // RHF END Feb 17, 2005 NEW_EXPORT

}

bool CExport::SetHeadNode( void ) {
    m_pHeadNode = (EXP_HEADER_NODE*) m_pExpLstBase;

    return( m_pHeadNode != NULL );
}

void CExport::DeleteProgStrip( void ) {
    if( m_pExpLstBase != NULL )
        free( m_pExpLstBase );
    m_pExpLstBase = NULL;
    m_iExpLstNext = 0;
    m_iExpLstSize = 0;
    m_pHeadNode = NULL; // RHF Feb 25, 2005
}
