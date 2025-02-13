#include "StdAfx.h"
#include <engine/Engine.h>
#include <engine/Batdrv.h>
#include <engine/Ctab.h>
#include <engine/ProgramControl.h>
#include <zMessageO/Messages.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseConstructionReporter.h>
#include <zDataO/DataRepository.h>
#include <zParadataO/Logger.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CString m_csSkipStructMsg;


bool CBatchDriverBase::RunInit() {
    bool    bCsBatch=( GetBatchMode() == CRUNAPL_CSBATCH );
    bool    bCsCalc=( GetBatchMode() == CRUNAPL_CSCALC );
    bool    bCsTab=( GetBatchMode() == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );

    try
    {
        OpenListerAndWriteFiles();

        m_pEngineDriver->GetLister()->SetMessageSource(_T("LEVEL 0 PREPROC"));

        m_pIntDriver->StartApplication();        
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Abort, exception);
        return false;
    }

    if( !bCsCalc ) //SAVY && for run modules when input dat files are not present
    {
        if( !OpenRepositories(false) )
            return false;
    }

    initwsect();

    // starting copy-cases file
    if( m_pPifFile->UsingOutputData() )
    {
        if( !m_pEngineDriver->OpenBatchOutputRepositories(m_pPifFile->GetOutputDataConnectionStrings(), false) )
            return false;
    }

    // checking every tables have memory assigned
    for( CTAB* ct : m_engineData->crosstabs )
    {
        if( ct->GetAcumArea() == NULL )
            issaerror( MessageType::Warning, 670, ct->GetName().c_str() );
    }

    // initializing sections for each dictionary
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetSubType() == SymbolSubType::Input || pDicT->GetSubType() == SymbolSubType::Work )
            continue;

        for( int iSymSec = pDicT->SYMTfsec; iSymSec > 0; iSymSec = SPT(iSymSec)->SYMTfwd )
            initsect(iSymSec);
    }

    // initializing input dict info
    m_pIntDriver->m_bStopProc = false;

    if( bCsBatch || bCsTab ) {
    }

    else if( bCsCalc ) {
        CString csCurrentBreakKey = _T("[") + ((CCalcDriver*)this)->GetCurrentBreakKey() + _T("]");
        m_pEngineDriver->GetLister()->SetMessageSource(CS2WS(csCurrentBreakKey));
    }

    return true;
}


void CBatchDriver::RunDriver()
{
    size_t process_update_frequency = 10;
    size_t cases_until_progress_update = process_update_frequency;
    size_t last_percent_read = SIZE_MAX;

    m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStart);

    DICX* pDicX = DIX(0);

    // generate a list of the output repositories (the batch output and the special output dictionaries), as well as cases for each repository
    std::vector<DICX*> batch_outputs;

    batch_outputs.emplace_back(pDicX); // the main batch output

    for( DICT* pOutputDicT : m_engineData->dictionaries_pre80 )
    {
        if( pOutputDicT->GetSubType() == SymbolSubType::Output ) // special output dictionaries
            batch_outputs.emplace_back(pOutputDicT->GetDicX());
    }


    ProcessSummaryReporter* process_summary_reporter = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_PROCESS_SUMMARY_REPORTER, (WPARAM)&process_summary_reporter);
    ASSERT(process_summary_reporter != nullptr);

    auto process_summary = m_pEngineDriver->GetProcessSummary();

    CString dialog_title = FormatText(_T("Running %s application %s. Press ESC to interrupt..."),
                                      m_pEngineDriver->m_lpszExecutorLabel, PortableFunctions::PathGetFilename(Appl.GetAppFileName()));

    process_summary_reporter->Initialize(dialog_title, m_pEngineDriver->GetProcessSummary(), &m_pIntDriver->m_bStopProc);

    // cycle through all of the input data
    bool continue_processing = true;

    try
    {
        for( int iFile = 0; continue_processing && iFile < (int)m_pPifFile->GetInputDataConnectionStrings().size(); iFile++ )
        {
            // update the progress reporter
            process_summary->SetPercentSourceRead(0);

            CString source_text = _T("File");

            if( m_pPifFile->GetInputDataConnectionStrings().size() > 1 )
                source_text.AppendFormat(_T(" %d of %d"), iFile + 1, (int)m_pPifFile->GetInputDataConnectionStrings().size());

            source_text.AppendFormat(_T(": %s"), PortableFunctions::PathGetFilename(m_pPifFile->GetInputDataConnectionString(iFile).GetFilename()));
            process_summary_reporter->SetSource(source_text);            


            // open the input data
            m_pEngineDriver->OpenRepository(pDicX, m_pPifFile->GetInputDataConnectionString(iFile), DataRepositoryOpenFlag::OpenMustExist, false);

            if( iFile == 0 && GetBatchMode() != CRUNAPL_CSTAB ) // run the application preproc
            {
                m_pIntDriver->ExecuteProcLevel(0, ProcType::PreProc);
                continue_processing = !m_pIntDriver->m_bStopProc;
            }

            while( continue_processing && pDicX->StepCaseIterator() )
            {
                m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStart);

                bool bWriteCase = false;

                // reset the casetainers for any special output dictionaries
                for( size_t i = 1; i < batch_outputs.size(); ++i )
                    batch_outputs[i]->ResetCaseObjects();

                continue_processing = BatchProcessCasetainer(&pDicX->GetCase(), pDicX->GetCase().GetPre74_Case(), batch_outputs, &bWriteCase);

                if( continue_processing && bWriteCase )
                {
                    // write out the cases
                    for( size_t i = 0; i < batch_outputs.size(); ++i )
                    {
                        DICX* pOutputDicX = batch_outputs[i];
                        bool update_notes = true;

                        auto write_case = [&](DataRepository& output_repository)
                        {
                            // update the notes
                            if( update_notes )
                            {
                                m_pEngineDriver->UpdateCaseNotesLevelKeys_pre80(pOutputDicX);
                                update_notes = false;
                            }

                            output_repository.WriteCasetainer(&pOutputDicX->GetCase());
                        };

                        // for the output data, write the case to each repository
                        if( i == 0 )
                        {
                            for( const std::shared_ptr<DataRepository>& output_data_repository : m_batchOutputRepositories )
                                write_case(*output_data_repository);
                        }

                        // for special outputs, there is only one repository
                        else if( pOutputDicX->IsDataRepositoryOpen() )
                        {
                            write_case(pOutputDicX->GetDataRepository());
                        }
                    }
                }

                // update the progress bar
                if( --cases_until_progress_update == 0 )
                {
                    size_t percent_read = pDicX->GetCaseIteratorPercentRead();

                    process_summary->SetPercentSourceRead(percent_read);

                    // if processing a massive file, update the progress bar less frequently
                    if( percent_read == last_percent_read )
                    {
                        process_update_frequency = (size_t)( process_update_frequency * 1.2 );
                    }

                    else
                    {
                        last_percent_read = percent_read;
                    }

                    cases_until_progress_update = process_update_frequency;

                    process_summary_reporter->SetKey(pDicX->GetCase().GetKey());
                }

                m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStop);
            }

            if( m_pIntDriver->m_bStopProc )
                continue_processing = false;

            // set the final percent read to 100% if the user didn't cancel, or the percent read if the user did cancel
            process_summary->SetPercentSourceRead(continue_processing ? 100 : pDicX->GetCaseIteratorPercentRead());

            if( continue_processing && ( iFile + 1 ) < (int)m_pPifFile->GetInputDataConnectionStrings().size() )
                CloseCurrentInputFile();
        }
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }

    m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStop);
}


void CBatchDriverBase::CloseCurrentInputFile()
{
    DICX* pDicX = DIX(0);
    pDicX->CloseDataRepository();
}


void CBatchDriverBase::RunEnd() {
    bool    bCsBatch=( GetBatchMode() == CRUNAPL_CSBATCH );
    bool    bCsCalc=( GetBatchMode() == CRUNAPL_CSCALC );
    bool    bCsTab=( GetBatchMode() == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );

    // if processing was stopped using either stop without (1) or by hitting cancel on the
    // batch meter dialog, turn off the stop flag so that the application postproc runs
    if( m_pIntDriver->m_bStopProc && m_pEngineDriver->GetStopCode() != 1 )
        m_pIntDriver->m_bStopProc = false;

    // generate descriptions & delete export support    // victor Dec 18, 00
    m_pEngineArea->ExportFinish();                      // victor Dec 18, 00

    // completing other tasks
    if( bCsBatch || bCsTab ) {
        m_pEngineDriver->GetLister()->SetMessageSource(_T("LEVEL 0 POSTPROC"));

        if( Breaknvars > 0 && *m_Tbd.GetCurrId() ) {
            //Save tables with break
            for( CTAB* pCtab : m_engineData->crosstabs ) {
                if( pCtab->m_uEnviron[0] & ct_BREAK )
                    m_Tbd.breaksave( m_Tbd.GetCurrId(), pCtab );
            }
        }

        m_pIntDriver->ExecuteProcLevel(0, ProcType::PostProc);

        CloseCurrentInputFile();
    }

    if( GetNumCtabToWrite() > 0 ) {
        if( !m_Tbd.IsNewTbd() )
            m_Tbd.tbd_WriteTrailer();

        if( !bCsCalc ) // RHF May 06, 2003
            m_Tbd.breakend();
    }

    CloseRepositories(false);

    CloseBatchOutputRepositories();

    m_pIntDriver->StopApplication();

    m_pEngineDriver->CloseListerAndWriteFiles();
}


void CBatchDriver::RunGroupIterator( int iSymGroup ) {  // victor Jun 29, 00
    ASSERT( GetBatchMode() == CRUNAPL_CSBATCH );// RHF Jan 22, 2003

    // RunGroupIterator: drives the execution of every occurrences of a given group
    CPrSlot* pPrSlot = GetCurBatchProg();
    ASSERT( pPrSlot->GetSlotType() == CPrSlot::GIslot );
    ASSERT( pPrSlot->GetProcType() == ProcType::PreProc );
    int     iHeadIndex = GetCurBatchProgIndex();
    int     iTailIndex = pPrSlot->GetSlotIndex();
    ASSERT( GetBatchProgSlotAt( iTailIndex )->GetSlotIndex() == iHeadIndex );
    GROUPT* pGroupT = GPT(iSymGroup);
    int     iOccur = pGroupT->GetFirstExOccurrence();

    if( iOccur == 0 )     // RHF  Aug 24, 2000
        SetCurBatchProgIndex( iTailIndex );
    else {
        while( iOccur ) {
            m_pIntDriver->ExecuteProcGroup(iSymGroup, ProcType::OnOccChange);

            RunGroupItems( iHeadIndex, iTailIndex );

            iOccur = pGroupT->GetNextExOccurrence();
        }
    }
}


void CBatchDriver::RunGroupItems( int iHeadIndex, int iTailIndex ) { // victor Jun 29, 00
    // RunGroupItems: traverse the procs of group' members up to the tail of group' iterator
    // ... for the implicit occurrence given by 'GROUPT::m_iExOccur'
    // TODO: tailor both compiler & interpreter to fit this new behavior
    // to use when working with long prog-strip <begin> // victor Mar 14, 01
    bool        bCheckSkipStruc = m_pEngineSettings->IsCheckingSkipStruc();
    GROUPT*     pGroupT;
    VART*       pVarT;
    double*     dValAddr;
    int         tIndex[DIM_MAXDIM];
    int         iAtOccur;
    // to use when working with long prog-strip <end>   // victor Mar 14, 01

    ASSERT( GetBatchMode() == CRUNAPL_CSBATCH );// RHF Jan 22, 2003

    int         iIndex = iHeadIndex;

    SetCurBatchProgIndex( iIndex );

    CPrSlot*    pPrSlot = GetCurBatchProg();
    ASSERT( pPrSlot->GetSlotType() == CPrSlot::GIslot );
    ASSERT( pPrSlot->GetProcType() == ProcType::PreProc );

    // advance the moving index to program-strip past the head of group' iterator
    pPrSlot = GetNextBatchProg( &iIndex );

    int iLastSymbolWithMessage=0;

    // run the procs of group' members up to the tail of group' iterator
    while( iIndex >= 0 && iIndex < iTailIndex ) {
        ProcType proc_type = pPrSlot->GetProcType();
        int iSymbol = pPrSlot->GetSlotSymbol();

        switch( pPrSlot->GetSlotType() ) {
            case CPrSlot::LEVELslot:
                ASSERT( 0 );            // shouldn't happen - only VA/GR encapsulated here
                break;

            case CPrSlot::GRslot:
                // working with long prog-strip <begin> // victor Mar 14, 01
                tIndex[0] = tIndex[1] = tIndex[2] = 0;

                if( m_pEngineDriver->IsSkipping() ) {
                    // ... check if the skip-target was reached
                    CNDIndexes theCurrentIndex( ZERO_BASED );

                    m_pIntDriver->GetCurrentGroupSubIndexes( iSymbol, theCurrentIndex );

                    bool    bTargetReached = m_pEngineDriver->IsSkippingTargetReached( iSymbol, theCurrentIndex );

                    if( bTargetReached )
                        m_pEngineDriver->ResetSkipping();
                }

                if( !m_pEngineDriver->IsSkipping() ) {
                    // ... check contents if requested (only at PostProc)
                    if( bCheckSkipStruc && proc_type == ProcType::PostProc ) {
                    }

                    if( proc_type == ProcType::PostProc )
                        m_pIntDriver->ExecuteProcGroup(iSymbol, ProcType::KillFocus);

                    // ... execute this proc
                    m_pIntDriver->ExecuteProcGroup(iSymbol, proc_type);

                    if( proc_type == ProcType::PreProc )
                        m_pIntDriver->ExecuteProcGroup(iSymbol, ProcType::OnFocus);
                }
                // working with long prog-strip <end>   // victor Mar 14, 01
                break;

            case CPrSlot::GIslot:
                ASSERT( proc_type == ProcType::PreProc );
                RunGroupIterator( iSymbol );
                break;

            case CPrSlot::Blockslot:
            {
                // check if the block was the current skip target
                if( m_pEngineDriver->IsSkipping() )
                {
                    const EngineBlock& engine_block = GetSymbolEngineBlock(iSymbol);

                    CNDIndexes theCurrentIndex( ZERO_BASED );
                    m_pIntDriver->GetCurrentGroupSubIndexes(engine_block.GetGroupT()->GetSymbol(), theCurrentIndex);

                    bool bTargetReached = m_pEngineDriver->IsSkippingTargetReached(iSymbol, theCurrentIndex);

                    if( bTargetReached )
                        m_pEngineDriver->ResetSkipping();
                }

                if( !m_pEngineDriver->IsSkipping() )
                    m_pIntDriver->ExecuteProcBlock(iSymbol, proc_type);

                break;
            }

            case CPrSlot::VAslot:
            {
                // working with long prog-strip <begin> // victor Mar 14, 01
                pVarT    = VPT(iSymbol);
                pGroupT  = pVarT->GetOwnerGPT();
                iAtOccur = pGroupT->GetCurrentExOccurrence();

                CNDIndexes theCurrentIndex( ZERO_BASED );

                m_pIntDriver->GetCurrentVarSubIndexes( iSymbol, theCurrentIndex );

                if( m_pEngineDriver->IsSkipping() ) {

                    // ... check if the skip-target was reached
                    bool    bTargetReached = m_pEngineDriver->IsSkippingTargetReached( iSymbol, theCurrentIndex );

                    if( bTargetReached )
                        m_pEngineDriver->ResetSkipping();
                }

                if( m_pEngineDriver->IsSkipping() ) {
                    if( iLastSymbolWithMessage != iSymbol && !m_pEngineDriver->IsBlankField( pVarT, theCurrentIndex ) ) {
                        iLastSymbolWithMessage = iSymbol;

                        CString csFieldMsg;
                        CString csFinalMsg;

                        CString csDirtyTxt;
                        csprochar* pVarAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarT, theCurrentIndex );
                        _tmemcpy(csDirtyTxt.GetBufferSetLength(pVarT->GetLength()), pVarAsciiAddr, pVarT->GetLength());
                        csDirtyTxt.ReleaseBuffer();

                        if( !pVarT->IsArray() ) {
                            csFieldMsg.Format(MGF::GetMessageText(88212).c_str(), pVarT->GetName().c_str(), csDirtyTxt.GetString());
                        }
                        else {
                            CString csVarNameOcc = FormatText(_T("%s%s"), pVarT->GetName().c_str(), theCurrentIndex.toString(pVarT->GetNumDim()).c_str());
                            csFieldMsg.Format(MGF::GetMessageText(88212).c_str(), csVarNameOcc.GetString(), csDirtyTxt.GetString());
                        }

                        csFinalMsg = m_csSkipStructMsg + csFieldMsg;

                        issaerror( MessageType::Error, 88180, csFinalMsg.GetString() );
                    }

                }

                if( !m_pEngineDriver->IsSkipping() && proc_type == ProcType::PreProc )
                {
                    // ... execute the PreProc + OnFocus
                    m_pIntDriver->ExecuteProcVar(iSymbol, ProcType::PreProc);
                    m_pIntDriver->ExecuteProcVar( iSymbol, ProcType::OnFocus);

                    // ... maybe the PreProc has just issued another skip
                    if( !m_pEngineDriver->IsSkipping() && pVarT->IsSkipStrucImpute() ) { // RHF Nov 09, 2001 Add && pVarT->IsSkipStrucImpute()
                        VARX*   pVarX=pVarT->GetVarX();

                        // RHF INIC Nov 09, 2001
                        // Alpha vars filled with "?"
                        if( bCheckSkipStruc && !pVarT->IsNumeric() && m_pEngineSettings->IsCheckingSkipStrucImpute()  ) {
                            CNDIndexes  theIndex( ZERO_BASED );
                            TCHAR*       pVarAsciiAddr;

                            pVarX->PassTheOnlyIndex( theIndex, iAtOccur ); // ***TRANSITION***
                            pVarAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarT, theIndex );

                            // stops at first non-blank occurrence found
                            ASSERT( pVarT->GetLength()  <= ENG_BLANKSIZE ); // re blank-area for comparisons
                            if( memcmp( pVarAsciiAddr, pEngBlank, pVarT->GetLength() ) == 0 ) {
                                CString csAssignText;
                                csAssignText = _T("?");
                                *pVarAsciiAddr = _T('?');

                                if( !pVarT->IsArray() )
                                    issaerror( MessageType::Error, 88223, pVarT->GetName().c_str(), csAssignText.GetString() );
                                else
                                    issaerror( MessageType::Error, 88224, pVarT->GetName().c_str(), iAtOccur, csAssignText.GetString() );
                            }
                        }
                        // RHF END Nov 09, 2001


                        // ... check contents if requested
                        if( bCheckSkipStruc && pVarT->IsNumeric() ) {
                            CNDIndexes theIndex( ZERO_BASED );

                            pVarX->PassTheOnlyIndex( theIndex, iAtOccur ); // ***TRANSITION***
                            dValAddr = m_pIntDriver->GetVarFloatAddr( pVarT, theIndex );

                            if( *dValAddr == NOTAPPL ) {

                                // RHF INIC Oct 18, 2003
                                bool    bCanEnterNotAppl    = pVarT->m_iBehavior & CANENTER_NOTAPPL;
                                bool    bAskWhenNotappl     = !( pVarT->m_iBehavior & CANENTER_NOTAPPL_NOCONFIRM );
                                bool    bShowMsg=true;

                                if( bCanEnterNotAppl && !bAskWhenNotappl )
                                    bShowMsg = false;
                                // RHF END Oct 18, 2003


                                // RHF INIC Nov 09, 2001
                                //88221 ... %s is not a skipped field, however is NotAppl
                                //88222 ... %s(%d) is not a skipped field, however is NotAppl
                                //88223 ... %s is not a skipped field, however is NotAppl. %s will be assigned.
                                //88224 ... %s(%d) is not a skipped field, however is NotAppl. %s will be assigned.

                                if( m_pEngineSettings->IsCheckingSkipStrucImpute() ) {
                                    CString csAssignText;

                                    double      dValue;
                                    // MISSING CAN't be BLANK!!!
                                    // set to true as part of CSPro 7.2 refactoring as part of the introduction of the ValueProcessor;
                                    // if BEHAVIOR_SKIPSTRUCIMPUTE is ever fully fleshed out, this decision can be revisited
                                    //if( *(Flotbase+pVarT->mis ) != MISSING ) { // Has mask defined
                                    if( true ) {
                                        csAssignText = _T("MISSING");
                                        dValue = MISSING;
                                    }
                                    else {
                                        csAssignText = _T("DEFAULT (MISSING mask undefined)");
                                        dValue = DEFAULT;
                                    }

                                    //  ---- Assign Value
                                    bool    bOk;

                                    bOk = m_pIntDriver->SetVarFloatValue( dValue, pVarX, theIndex );
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
                                    if( !pVarT->IsArray() )
                                        issaerror( MessageType::Error, 88223, pVarT->GetName().c_str(), csAssignText.GetString() );
                                    else
                                        issaerror( MessageType::Error, 88224, pVarT->GetName().c_str(), iAtOccur, csAssignText.GetString() );

                                }
                                // RHF END Nov 09, 2001
                                else {
                                    if( bShowMsg ) {// RHF Oct 18, 2003
                                        CString csFieldMsg;
                                        CString csFinalMsg;

                                        m_csSkipStructMsg = WS2CS(MGF::GetMessageText(88184));

                                        if( !pVarT->IsArray() ) {
                                            csFieldMsg.Format(MGF::GetMessageText(88221).c_str(), pVarT->GetName().c_str());
                                        }
                                        else {
                                            CString csVarNameOcc = FormatText( _T("%s%s"), pVarT->GetName().c_str(), theCurrentIndex.toString(pVarT->GetNumDim()).c_str() );
                                            csFieldMsg.Format( MGF::GetMessageText(88221).c_str(), csVarNameOcc.GetString() );
                                        }

                                        csFinalMsg = m_csSkipStructMsg + csFieldMsg;

                                        issaerror( MessageType::Error, 88182, csFinalMsg.GetString() );
                                    } // RHF Oct 18, 2003
                                }
                            }
                        }
                    }
                }

                if( !m_pEngineDriver->IsSkipping() && proc_type == ProcType::PostProc )
                {
                    // ... execute the KillFocus + PostProc
                    m_pIntDriver->ExecuteProcVar(iSymbol, ProcType::KillFocus);
                    m_pIntDriver->ExecuteProcVar(iSymbol, ProcType::PostProc);
                }

                // working with long prog-strip <end>   // victor Mar 14, 01
                break;
            }

            default:
                ASSERT( 0 );            // shouldn't happen (no other types)
                break;
        }

        // advance the moving index to program-strip
        pPrSlot = GetNextBatchProg( &iIndex );
    }
    ASSERT( pPrSlot->GetProcType() == ProcType::PostProc );

    // advance the moving index to program-strip past the tail of group' iterator
// RHF COM Aug 07, 2000    GetNextBatchProg( &iIndex );
}

bool CBatchDriver::BatchProcessCasetainer(Case* pCasetainer, Pre74_Case* pInputCase, const std::vector<DICX*>& batch_outputs, bool* pbWriteCase)
{
    CArray<Pre74_CaseLevel*> apParentLevels;
    *pbWriteCase = false;

    return BatchProcessCaseLevel(pCasetainer, pInputCase->GetRootLevel(), pCasetainer->GetRootCaseLevel(), batch_outputs, apParentLevels, pbWriteCase);
}


bool CBatchDriver::BatchProcessCaseLevel(Case* pCasetainer, Pre74_CaseLevel* pInputLevel, const CaseLevel& case_level,
                                         const std::vector<DICX*>& batch_outputs, CArray<Pre74_CaseLevel*>& apParentLevels, bool* pbWriteCase)
{
    if( m_pIntDriver->m_bStopProc )
        return false;

    DICT* pDicT = DIP(0);
    DICX* pDicX = DIX(0);

    CString level_key = ( pInputLevel->GetLevelNum() > 1 ) ? pInputLevel->GetKey().Mid(pDicT->qlen[0]) : CString();
    m_pEngineDriver->GetLister()->SetMessageSource(pDicX->GetCase(), CS2WS(level_key));

    ParseCaseLevel(pCasetainer, pInputLevel, &case_level, pDicT);

    int iSharedLevel = 0;
    bool level_was_skipped = false;

    if( pInputLevel->GetLevelNum() > 1 )
        iSharedLevel = commonlvl(pDicT, pDicX->last_key, pDicX->lastlevel, pDicX->current_key, pDicX->level);

    try
    {
        for( int iLevel = iSharedLevel + 1; iLevel <= pDicX->level; iLevel++ )
        {
            if( !EvaluateNode(pDicT, iLevel) )
                throw SkipCaseProgramControlException();

            // open the level
            m_pIntDriver->ExecuteProcLevel(iLevel, ProcType::PreProc);
        }

        // get the level and the starting proc for this level
        int iCurLevel = pDicX->level;
        int iProgIndex = ( iCurLevel ) ? m_iLevProg[iCurLevel] : -1;

        // working with "implicit iterations"       // victor Jun 29, 00
        if( iProgIndex >= 0 )
        {
            // set the starting proc for this level
            SetCurBatchProgIndex( iProgIndex );
            ResetSkipping();        // batch-skip   // victor Mar 14, 01

            CPrSlot* pPrSlot = GetCurBatchProg();

            while( !pPrSlot->IsEmptySlot() )
            {
                ProcType proc_type = pPrSlot->GetProcType();
                int iSymbol = pPrSlot->GetSlotSymbol();

                switch( pPrSlot->GetSlotType() )
                {
                    case CPrSlot::LEVELslot:
                        // ignore Level' procs (are executed outside this cycle)
                        break;

                    case CPrSlot::GRslot:
                        // working with long prog-strip <begin> // victor Mar 14, 01
                        int tIndex[DIM_MAXDIM];
                        tIndex[0] = tIndex[1] = tIndex[2] = 0;

                        if( m_pEngineDriver->IsSkipping() )
                        {
                            // ... check if the skip-target was reached
                            bool bTargetReached = m_pEngineDriver->IsSkippingTargetReached( iSymbol, tIndex, (int)proc_type );

                            if( bTargetReached )
                                m_pEngineDriver->ResetSkipping();
                        }

                        if( !m_pEngineDriver->IsSkipping() )
                        {
                            // ... check contents if requested (only at PostProc)
                            if( m_pEngineSettings->IsCheckingSkipStruc() && proc_type == ProcType::PostProc )
                            {
                            }

                            if( proc_type == ProcType::PostProc )
                                m_pIntDriver->ExecuteProcGroup(iSymbol, ProcType::KillFocus);

                            // ... execute this proc
                            m_pIntDriver->ExecuteProcGroup(iSymbol, proc_type);

                            if( proc_type == ProcType::PreProc )
                                m_pIntDriver->ExecuteProcGroup(iSymbol, ProcType::OnFocus);
                        }

                        // working with long prog-strip <end>   // victor Mar 14, 01
                        break;

                    case CPrSlot::GIslot:
                        ASSERT(proc_type == ProcType::PreProc);
                        RunGroupIterator( iSymbol );
                        break;

                    case CPrSlot::Blockslot:
                    case CPrSlot::VAslot:
                        ASSERT( 0 );// shouldn't happen (done by RunGroupIterator)
                        break;

                    case CPrSlot::CTslot:
                        m_pIntDriver->ExecuteProcTable(iSymbol, proc_type);
                        break;

                    default:
                        ASSERT( 0 );// shouldn't happen (no other types)
                        break;
                }

                // advance the moving index
                pPrSlot = GetNextBatchProg();
            }
        }
    }

    // endcase will stop executing any additional procs for the case
    catch( const EndCaseProgramControlException& )  { }

    // as will skip case except then the level will not be written out
    catch( const SkipCaseProgramControlException& ) { level_was_skipped = true; }


    CArray<Pre74_CaseLevel*> apOutputLevels;

    // are there children levels to process?
    if( !level_was_skipped )
    {
        for( size_t i = 0; i < batch_outputs.size(); ++i )
        {
            Pre74_CaseLevel* pOutputLevel = NULL;

            if( i == 0 )
                pOutputLevel = pInputLevel;

            else
            {
                if( pInputLevel->GetLevelNum() == 1 )
                {
                    pOutputLevel = batch_outputs[i]->GetCase().GetPre74_Case()->GetRootLevel();
                }

                else
                {
                    // only add a 2+ level if the special output dictionary supports that many levels
                    const CDataDict* pOutputDict = batch_outputs[i]->GetDicT()->GetDataDict();
                    Pre74_CaseLevel* pParentLevel = apParentLevels[i];

                    if( pInputLevel->GetLevelNum() <= (int)pOutputDict->GetNumLevels() )
                        pOutputLevel = pParentLevel->AddChildLevel(pOutputDict->GetLevel(pInputLevel->GetLevelNum() - 1));
                }
            }

            apOutputLevels.Add(pOutputLevel);
        }

        if( pInputLevel->GetNumChildLevels() > 0 )
        {
            // create a copy of the levels because skipped levels will be removed so any counter will be invalid
            std::vector<std::tuple<Pre74_CaseLevel*, const CaseLevel*>> case_levels;

            ASSERT(case_level.GetNumberChildCaseLevels() == static_cast<size_t>(pInputLevel->GetNumChildLevels()));

            for( int i = 0; i < pInputLevel->GetNumChildLevels(); ++i )
                case_levels.emplace_back(pInputLevel->GetChildLevel(i), &case_level.GetChildCaseLevel(i));

            for( auto& [pre74_case_level, child_case_level] : case_levels )
            {
                if( !BatchProcessCaseLevel(pCasetainer, pre74_case_level, *child_case_level, batch_outputs, apOutputLevels, pbWriteCase) )
                    return false;
            }

            // restore the message source to this case level's
            m_pEngineDriver->GetLister()->SetMessageSource(pDicX->GetCase(), CS2WS(level_key));
        }
    }

    // at this point, any children levels have been processed

    // if the level wasn't initially skipped, close it
    if( !level_was_skipped && !m_pIntDriver->m_bStopProc )
    {
        m_pEngineDriver->GetProcessSummary()->IncrementPostProcsExecuted(pInputLevel->GetLevelNum() - 1);

        try
        {
            m_pIntDriver->ExecuteProcLevel(pInputLevel->GetLevelNum(), ProcType::PostProc);
        }

        catch( const EndCaseProgramControlException& )  { }
        catch( const SkipCaseProgramControlException& ) { level_was_skipped = true; }
    }

    // if the level was skipped, remove it from the case
    if( level_was_skipped )
    {
        if( pInputLevel->GetLevelNum() > 1 )
        {
            for( size_t i = 0; i < batch_outputs.size(); ++i )
            {
                Pre74_CaseLevel* pParentLevel = apParentLevels[i];

                if( pParentLevel != nullptr )
                    pParentLevel->RemoveChildLevel(pInputLevel);
            }
        }
    }

    else if( !m_pIntDriver->m_bStopProc ) // we're keeping the case
    {
        for( size_t i = 0; i < batch_outputs.size(); ++i )
        {
            Pre74_CaseLevel* pOutputLevel = apOutputLevels[i];

            if( pOutputLevel != NULL )
            {
                Pre74_Case* pCase = batch_outputs[i]->GetCase().GetPre74_Case();

                if( i == 0 ) // reset the contents of the main dictionary (but don't modify the children levels)
                    pOutputLevel->Reset(false);

                CopyLevelToRepository(batch_outputs[i]->GetDicT(),&batch_outputs[i]->GetCase(),pOutputLevel);

                // suppress the case construction reporter when this case won't be output
                bool use_case_construction_reporter = false;

                if( i == 0 )
                {
                    for( const std::shared_ptr<DataRepository>& output_data_repository : m_batchOutputRepositories )
                    {
                        if( output_data_repository->GetRepositoryType() != DataRepositoryType::Null )
                        {
                            use_case_construction_reporter = true;
                            break;
                        }
                    }
                }

                else
                {
                    use_case_construction_reporter = ( batch_outputs[i]->IsDataRepositoryOpen() &&
                                                       batch_outputs[i]->GetDataRepository().GetRepositoryType() != DataRepositoryType::Null );
                }


                pCase->FinalizeLevel(pOutputLevel, true, false,
                    use_case_construction_reporter ? batch_outputs[i]->GetCase().GetCaseConstructionReporter() : nullptr);

                if( i > 0 ) // special output dictionary
                    pCase->ApplySpecialOutputKey(pDicX->GetCase().GetPre74_Case(), pOutputLevel, pInputLevel->GetKey());
            }
        }

        if( pInputLevel->GetLevelNum() == 1 )
            *pbWriteCase = true; // the case should be written
    }

    return !m_pIntDriver->m_bStopProc;
}
