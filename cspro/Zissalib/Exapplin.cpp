//-----------------------------------------------------------------------//
//                                                                       //
//  exapplinit: common initialization tasks for entry & batch processors //
//                                                                       //
//-----------------------------------------------------------------------//

#include "StdAfx.h"
#include <zPlatformO/PlatformInterface.h>
#include <engine/Ctab.h>
#include <engine/Exappl.h>
#include <engine/Dicx.h>
#include <engine/Engine.h>
#include <engine/COMMONIN.H>
#include <engine/Comp.h>
#include <zEngineO/ApplicationLoader.h>
#include <zToolsO/Serializer.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/ConnectionString.h>
#include <zCapiO/CapiQuestionManager.h>
#include <ZBRIDGEO/npff.h>
#include <zListingO/ErrorLister.h>
#include <zLogicO/SpecialFunction.h>


#ifdef WIN_DESKTOP
#include <zLogicO/SourceBuffer.h>
#endif

#if defined(USE_BINARY) || defined(GENERATE_BINARY)

#else
#define PortableFunctions::FileExists(x) true
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


///////////////////////// ISSA-based functions ///////////////////////////////


bool CEngineDriver::exapplinit()
{
    MessageType abort_type = MessageType::Abort;
#ifdef USE_BINARY
    abort_type = MessageType::Error;
#endif

    Application* pApp = GetApplication();
    ASSERT( pApp );

    InitAppName();

    if( m_pPifFile->GetStartModeString().GetLength() > 1 )
        ExMode = _totupper( m_pPifFile->GetStartModeString()[0] );


    if(!pApp->IsCompiled() ) {  //SAVY March 2002

        // load the messages
        BuildMessageManagers();

        // potentially produce an error file with application errors (when not running in the portable environment)
#ifdef WIN_DESKTOP
        if( Issamod == ModuleType::Batch || Issamod == ModuleType::Entry )
            m_compilerErrorLister = std::make_unique<Listing::ErrorLister>(*m_pPifFile);
#endif

        m_pEngineArea->inittables();


        if( UseNewDriver() )
        {
            LoadApplication();
        }

        else
        {
            // load application itself, insert names of main members in symbol table
            if( !attrload() || io_Err )
            {
#ifdef WIN_DESKTOP
                issaerror( abort_type, 10004, ApplName.GetString(), Failmsg.GetString() );
#endif
                return false;
            }


            // load application main members (Dicts & Flows)
            if( !LoadApplChildren(NULL) ) // RHF Jun 12, 2003 Add Null
            {
#ifdef WIN_DESKTOP
                issaerror( abort_type, 10004, ApplName.GetString(), Failmsg.GetString() );
#endif
                return false;
            }
        }


        if( GetApplication()->GetAppLoader()->GetBinaryFileLoad() && !m_bBinaryLoaded )
        {
#ifdef WIN_DESKTOP
            issaerror( abort_type, 10004, ApplName.GetString(), Failmsg.GetString() );
#endif
            return false;
        }

        // load the CAPI questions if not reading from a .pen file
        CapiQuestionManager* question_text_manager = nullptr;

        if( Issamod == ModuleType::Entry )
        {
            CEntryDriver* pEntryDriver = assert_cast<CEntryDriver*>(this);

            try
            {
                pEntryDriver->BuildQuestMgr();
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
                return false;
            }
            
            question_text_manager = pEntryDriver->GetQuestMgr();
        }

        // create basic tables to support 'compall' of app' declarations
        m_pIntDriver->AllocExecBase();

#ifdef WIN_DESKTOP
        if (!GetApplication()->GetAppLoader()->GetBinaryFileLoad()) {
            //////////////////////////////////////////////////////////////////////////
            //  -> Compile everything now
            int iErrors = m_pEngineCompFunc->getErrors();
            // compile app' declarations
            m_pEngineCompFunc->compall( 1 );
            m_pEngineCompFunc->incrementErrors(iErrors);

            // compile remaining procedures
            m_pEngineCompFunc->compall( 2 );

            // compile the CAPI conditions and fills
            if( m_pApplication->GetUseQuestionText() && question_text_manager != nullptr )
            {
                std::function<int(const CapiLogicParameters&)> compile_callback = [&](const CapiLogicParameters& capi_logic_parameters)
                {
                    return ( m_pEngineCompFunc->getErrors() == 0 ) ? m_pEngineCompFunc->CompileCapiLogic(capi_logic_parameters) : -1;
                };

                question_text_manager->CompileCapiLogic(compile_callback);

                if( m_pEngineCompFunc->getErrors() > 0 )
                {
                    issaerror(MessageType::Abort, 10010, PortableFunctions::PathGetFilename(m_pApplication->GetApplicationFilename()), m_pEngineCompFunc->getErrors());
                    return false;
                }
            }

            if( UseOldDriver() )
            {
                // create remaining executor' tables
                m_pIntDriver->AllocExecTables();    // was 'exallocdict'
            }
        }
#endif

#ifdef GENERATE_BINARY
        if( BinaryGen::isGeneratingBinary() )
        {
            try
            {
                SaveCompiledBinary();
            }

            catch(...)
            {
                ErrorMessage::Display(FormatText(_T("There was an error writing to the binary file: %s"), BinaryGen::GetBinaryName().c_str()));
                // TODO: Decide what to do when binary writing does not work
                //  - report any error?
                //  - abort?
                #pragma message( "TODO: Add behavior when binary generation does not work" )
            }
        }
#endif // GENERATE_BINARY

        try
        {
            if( pApp->GetApplicationLoader() != nullptr )
            {
                ASSERT(m_userMessageManager != nullptr);
                pApp->GetApplicationLoader()->ProcessUserMessagesPostCompile(*m_userMessageManager);
            }

            if( m_pIntDriver->HasSpecialFunction(SpecialFunction::OnSystemMessage) )
                UpdateMessageIssuers(true);
        }

        catch( const ApplicationLoadException& exception ) // APP_LOAD_TODO centralize exception handling
        {
#ifdef WIN_DESKTOP
            ErrorMessage::Display(exception);
#else
            throw;
#endif
        }


#ifndef USE_BINARY
        if (!GetApplication()->GetAppLoader()->GetBinaryFileLoad()) {
            m_pEngineCompFunc->CheckUnusedFileNames();
        }
#endif

#ifdef WIN_DESKTOP
        Appl.m_AppTknSource.reset();
#endif

        if (!GetApplication()->GetAppLoader()->GetBinaryFileLoad()) {
#ifdef WIN_DESKTOP
            // Binary is for ENTRY, so the if() below will always be false in USE_BINARY
            // compilation -> do not call CheckProcTables()
            if( Issamod != ModuleType::Entry ) // RHF Feb 10, 2003
                m_pEngineCompFunc->CheckProcTables(); // RHF INIC Jan 23, 2003
#endif
        }

        // RHF INIC Nov 02, 2000
        // Fix problem in Data Entry. If the sub-items are in the forms but not in the logic, the sub-items
        // are not used. Then the content of the item is not refresh.
        // Is necesary mark as used all sub-items in form if the item is used. See changes in LookForUsedSubItems method
        // RHF END Nov 02, 2000
        // any appl type, all dicts: "used subitem" implies "used item" also
        m_pEngineArea->LookForUsedSubItems();               // victor Aug 13, 99

        m_pEngineArea->DicxStart(); // RHF 12/8/99
        InitializeData();

        if( UseOldDriver() )
        {
            DICT*   pDicT = DIP(0);
            int     isymCommonSecLevel;
            int     isymCommonVar;

            QidLength = 0;
            for( int iLevel = 0; iLevel < (int)MaxNumberLevels && pDicT->qloc[iLevel] > 0; iLevel++ ) {
                int    iLevelidLen = 0;
                int    iIdVarOrder = 0;
                isymCommonSecLevel = pDicT->Common_GetCommonSection( iLevel );
                isymCommonVar = SPT(isymCommonSecLevel)->SYMTfvar;
                while( isymCommonVar > 0 ) {
                    // RHF INIC Feb 20, 2001
                    if( iIdVarOrder >= MAXQIDVARS - 1 )
                        issaerror( MessageType::Abort, 77 );
                    // RHF END Feb 20, 2001

                    QidVars[iLevel][iIdVarOrder++] = isymCommonVar;

                    iLevelidLen += VPT(isymCommonVar)->GetLength();

                    isymCommonVar = VPT(isymCommonVar)->SYMTfwd;
                }

                // mark end of IdVars list for this level
                QidVars[iLevel][iIdVarOrder] = -1;  //***enlarge MAXQIDVARS pls!!!

                if( iLevelidLen != pDicT->qlen[iLevel] )
                    issaerror( MessageType::Abort, 1024, iLevel + 1, iLevelidLen, pDicT->qlen[iLevel] );

                QidLength += pDicT->qlen[iLevel];
            }

            if( Issamod == ModuleType::Entry ) {
                for( int i = 0; i < pDicT->maxlevel && pDicT->qloc[i] > 0; i++ ) {
                    int     j;
                    int     len;

                    for( len = j = 0; QidVars[i][j] >= 0; j++ )
                        len += VPT(QidVars[i][j])->GetLength();
                    if( len != pDicT->qlen[i] )
                    {
#ifdef WIN_DESKTOP
                        issaerror( MessageType::Abort, 1024, i + 1, len, pDicT->qlen[i] );
#endif
                    }
                }
            }

            m_pIntDriver->CompleteHiddenGroup();// RHF Nov 09, 2000
            m_pIntDriver->BuildRecordsMap();    // was 'levassign'
        }

        m_pEngineArea->get_acum();

        // execution mode
        if( Appl.ApplicationType == ModuleType::Entry ) {
            ExMode = _T('A');
        }
        else
            ExMode = _T(' ');


        // load or save the CAPI questions (when loading/saving .pen files)
        if( question_text_manager != nullptr && ( BinaryGen::isGeneratingBinary() ||
            ( GetApplication()->GetAppLoader() != nullptr && GetApplication()->GetAppLoader()->GetBinaryFileLoad() ) ) )
        {
            APP_LOAD_TODO_GetArchive() & *question_text_manager;
        }


        // close the error lister
        m_compilerErrorLister.reset();

        // resources
        try
        {
            if( pApp->GetApplicationLoader() != nullptr )
                pApp->GetApplicationLoader()->ProcessResources();
        }

        catch( const ApplicationLoadException& exception ) // APP_LOAD_TODO centralize exception handling
        {
#ifdef WIN_DESKTOP
            ErrorMessage::Display(exception);
#else
            throw;
#endif
        }

    }// SAVY added this For single call of compile in entry

    return true;
}


void CEngineArea::get_acum() {          // assign Crosstab' acumareas
    for( CTAB* ct : m_engineData->crosstabs )
        get_cumarea( ct );              // get m_pAcumArea for this Crosstab
}

void CEngineArea::get_cumarea( CTAB* ct )
{
#ifdef WIN_DESKTOP

    int xtab_level = ct->GetTableLevel() / 10;
    int decl_level = ct->GetTableLevel() % 10;

    ct->SetTableLevel(std::max( xtab_level, decl_level ) );

    // setup ct_BREAK depending on real environment
    // RHF INIC Oct 23, 2001
    if( ct->GetTableType() == CTableDef::Ctab_Freq ) {
        ct->m_uEnviron[0] &= ~ct_BREAK;
        ct->SetTableLevel( 0 );
    }
    // RHF END Oct 23, 2001
    else {
      //Savy added this line "ct->GetNumBreaks()== 0" to make table with no break work when lowest level on
      // some table is totals ,when you have area processing in other tables Jan 23 2008
        if( Breaknvars <= 0 ||ct->GetNumBreaks()== 0 ) //savy modfied with ||
            ct->m_uEnviron[0] &= ~ct_BREAK;
        else if( ct->GetTableLevel() < Breaklevel && ct->m_uEnviron[0] & ct_BREAK ) {
            // RHF/VC INIC 11/10/96
            // Tabla declarada, con variables de WORKDICT, y sin XTAB que le hay subido el nivel
            if( ct->GetTableLevel() == 0 )
                ct->SetTableLevel( Breaklevel );
            else
                // RHF/VC END 11/10/96
                ct->m_uEnviron[0] &= ~ct_BREAK;
        }
    }

    UINT   uSize = ct->GetAcumSize();

    // RHF INIC Aug 12, 2002
    double      dDefaultValue;
    double*     pDefaultValue;

    if( ct->GetTableType() == CTableDef::Ctab_MinTable ) {
        //dDefaultValue = INVALIDINDEX;
        dDefaultValue = NOTAPPL;
        pDefaultValue = &dDefaultValue;
    }
    else if( ct->GetTableType() == CTableDef::Ctab_MaxTable ) {
        //dDefaultValue = -INVALIDINDEX;
        dDefaultValue = -NOTAPPL;
        pDefaultValue = &dDefaultValue;
    }
    else if( ct->GetTableType() == CTableDef::Ctab_Mode
            || ct->GetTableType() == CTableDef::Ctab_Percentil
            || ct->GetTableType() == CTableDef::Ctab_Median
            ) {
        dDefaultValue = -NOTAPPL;
        pDefaultValue = &dDefaultValue;
    }
    else
        pDefaultValue = NULL;
    // RHF END Aug 12, 2002

    byte*   pAcumArea = ct->m_pAcum.Alloc( ct->GetCellSize(), ct->GetTotDim(0), ct->GetTotDim(1), ct->GetTotDim(2), (byte*) pDefaultValue ); // RHF Jul 31, 2001
    ct->SetAcumArea( (csprochar*) pAcumArea );

    if( ct->GetAcumArea() == NULL )
        issaerror( MessageType::Warning, 1009, ct->GetName().c_str() );
    else if( pDefaultValue == NULL ) // RHF Aug 12, 2002 Add pDefaultValue == NULL
        memset( ct->GetAcumArea(), 0, uSize );

#if defined(USE_BINARY) // IGNORE_CTAB
#else
    // RHF INIC Jan 30, 2003
    if( ct->m_pBorder != NULL )
        ct->AllocBorder();
    // RHF END Jan 30, 2003

    // RHF INIC Aug 13, 2002
    // Remake coordnumber & bitmaps
    for( int iSubTable = 0; iSubTable < ct->GetNumSubTables(); iSubTable++ ) {
        CSubTable& cSubTable=ct->GetSubTable(iSubTable);
        //cSubTable.MakeCoordNumberMap( ct, pNodeBase );

        // GenRemapCoord only create maps when ct->GetNumCells() <= MAXCELL_REMAP.
        // If greater than MAXCELL_REMAP use GetTableCoordOnLine & GetSubTableCoordOnLine methods
        cSubTable.GenRemapCoord( m_pEngineArea );
    }
    // RHF END Aug 13, 2002

    // RHF INIC Oct 09, 2002
    // Some percent subtable
    ct->CalcHasSomePercent();
#endif

    //bool    bSomePercent=ct->GetHasSomePercent();
    // RHF END Oct 09, 2002
#else
    // crosstabs don't exist in the portable environments
    ASSERT(false);
#endif
}

////////////////////// ISSAW/IMSA new functions //////////////////////////////
int CEngineArea::LookForUsedSubItems()
{
    // victor Aug 13, 99
    int iNumChanges = 0;

    // loops thru each section of each dict...
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetSubType() == SymbolSubType::NoType )
            continue;                   // empty dict slot

        // RHF INIC Sep 12, 2001
        if( Issamod == ModuleType::Entry ) {
            int     iSymSec = pDicT->SYMTfsec;

            while( iSymSec > 0 ) {
                SECT*   pSecT = SPT(iSymSec);
                int     iSymVar = pSecT->SYMTfvar;

                while( iSymVar > 0 ) {
                    VART*   pVarT = VPT(iSymVar);
                    if( pVarT->IsInAForm() )
                        pVarT->SetUsed( true );
                    iSymVar = pVarT->SYMTfwd;
                }
                iSymSec = pSecT->SYMTfwd;
            }
        }
        // RHF END Sep 12, 2001


        int     iSymSec = pDicT->SYMTfsec;

        while( iSymSec > 0 ) {
            SECT*   pSecT = SPT(iSymSec);
            int     iSymVar = pSecT->SYMTfvar;

            // ... looking for "used" subitems...
            while( iSymVar > 0 ) {
                VART*   pVarT = VPT(iSymVar);

                if( pVarT->IsUsed() ) {
                    // ... in order to mark its owner as "used" also
                    VART*   pOwnerVarT = pVarT->GetOwnerVarT();

                    if( pOwnerVarT != NULL ) {
                        pOwnerVarT->SetUsed( true );
                        iNumChanges++;
                    }

                    // RHF INIC Nov 02, 2000
                    else { // Item is used --> mark all sub-items in forms like used.
                        if( Issamod == ModuleType::Entry ) {
                            VART*   pSubItem=pVarT->GetNextSubItem();

                            while( pSubItem != NULL ) {
                                if( pSubItem->IsInAForm() )
                                    pSubItem->SetUsed( true );

                                pSubItem = pSubItem->GetNextSubItem();
                            }
                        }
                    }
                    // RHF END Nov 02, 2000
                }
                // RHF INIC May 2, 2001
                bool    bItem = ( !pVarT->GetOwnerSymItem() );
                if( bItem )
                    pVarT->DoNeedConvertSomeSubItem();
                // RHF END May 2, 2001



                iSymVar = pVarT->SYMTfwd;
            }

            iSymSec = pSecT->SYMTfwd;
        }
    }

    return iNumChanges;
}
