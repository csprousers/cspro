#include "StdAfx.h"
#include "Shlwapi.h"
#include <engine/Engine.h>
#include <engine/Batdrv.h>
#include <engine/Ctab.h>
#include <engine/ProgramControl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//---------------------------------------------------------------------------
//  File name: calcdrv.cpp
//
//  Description:
//          Post Calculation Driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Dec 02   RHF     Creation
//---------------------------------------------------------------------------


// CsCalc add in
CCalcDriver::CCalcDriver(Application* pApplication)
    :   CBatchDriverBase(pApplication)
{
    m_Tbd.SetBatchDriver( this );
    m_Tbd.SetNewTbd( CSettings::m_bNewTbd );
    m_iCurrentBreakKeyNum = -1;
}

bool CCalcDriver::OpenInputTbd( CString csInputTbdName ) {
    m_InputTbd.SetFileName( csInputTbdName );

    bool    bRet = m_InputTbd.Open( false );

    if( !bRet ) {
        m_InputTbd.Close();
    }
    else {
        CString csInputTbiName(csInputTbdName);
        PathRemoveExtension(csInputTbiName.GetBuffer(_MAX_PATH));
        csInputTbiName.ReleaseBuffer();
        csInputTbiName += FileExtensions::BinaryTable::WithDot::TabIndex;

        int     iKeyLen = m_InputTbd.GetBreakKeyLen() + sizeof(short);
        m_InputTbi.SetFileName( csInputTbiName, iKeyLen );
        bRet = m_InputTbi.Open( false );
        if( !bRet ) {
            m_InputTbd.Close();
            m_InputTbi.Close();
        }
    }

    return bRet;
}

void CCalcDriver::CloseInputTbd() {
    m_InputTbd.Close();
    m_InputTbi.Close();
    m_InputTbd.Init();
}

void CCalcDriver::CreateProgLevel( int iLevel ) {
    if( iLevel == 0 ) // There is no level 0 in CsCalc
        return;

    // CreateProgLevel: build the program-strip partition for a given level
    // ... formerly 'bdicprog', now 'CreateProgLevel'
    int     iSymLevel = GetGroupTRootInProcess()->GetLevelSymbol( iLevel );
    ASSERT( iSymLevel > 0 );

    // adding Level' PROCTYPE_POSTCALC-block
    CreateProgSlot( iLevel, CPrSlot::LEVELslot, ProcType::ExplicitCalc, iSymLevel );

    // RHF INIC May 07, 2003
    for( int iTabSlot = 0; iTabSlot < m_aCtabExecOrder.GetSize(); iTabSlot++ ) {
        int     iCtab=m_aCtabExecOrder.GetAt(iTabSlot);
        CTAB*   pCtab = XPT(iCtab);

        if( pCtab->GetTableLevel() == iLevel ) {
            CreateProgTable( iLevel, iCtab );
        }
    }
    // RHF END May 07, 2003

    // marking "no more progs at this level"
    CreateProgSlot( iLevel, CPrSlot::EMPTYslot );
}

int CCalcDriver::CreateProgGroup( int iLevel, int iSymGroup, bool bDoCreate ) {
    ASSERT(0);
    return -1;
}

int CCalcDriver::CreateProgItem( int iLevel, int iSymVar, bool bDoCreate ) {
    ASSERT(0);
    return -1;
}

int CCalcDriver::CreateProgTable( int iLevel, int iCtab, bool bDoCreate ) {
    // CreateProgTable: build the program-strip for a Table
    ASSERT( iCtab > 0 );
    CTAB*   pCtab = XPT(iCtab);
    int     iNumProgSlots = 0;

    // Explicit Calc
    // adding Var' PROCTYPE_CALC-block only if PostCalc present
    if( pCtab->HasProcIndex(ProcType::ExplicitCalc) ) {
        iNumProgSlots++;
        if( bDoCreate )
            CreateProgSlot( iLevel, CPrSlot::CTslot, ProcType::ExplicitCalc, iCtab );
    }

    // Implicit
    iNumProgSlots++;
    if( bDoCreate )
        CreateProgSlot( iLevel, CPrSlot::CTslot, ProcType::ImplicitCalc, iCtab );

    return iNumProgSlots;
}

CString sLoadedBreakKey;

void CCalcDriver::RunDriver( void ) {
    // ... formerly 'rundriver', now 'RunDriver'
    DICX*       pDicX = DIX(0);

#ifdef  _DEBUG
    DumpBatchProg();
#endif//_DEBUG

    pDicX->level = 1;// RHF Oct 31, 2002

    // For each break
    int     iTotalBreaks = m_aBreakKeys.GetSize();

    ASSERT( m_aBreakKeys.GetSize() == m_aBreakNumKeys.GetSize() );

    ProcessSummaryReporter* process_summary_reporter = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_PROCESS_SUMMARY_REPORTER, (WPARAM)&process_summary_reporter);
    ASSERT(process_summary_reporter != nullptr);

    CString dialog_title = FormatText(_T("Running %s application %s. Press ESC to interrupt..."),
                                      m_pEngineDriver->m_lpszExecutorLabel,
                                      PortableFunctions::PathGetFilename(Appl.GetAppFileName()));

    process_summary_reporter->Initialize(dialog_title, m_pEngineDriver->GetProcessSummary(), &m_pIntDriver->m_bStopProc);

    process_summary_reporter->SetSource(m_InputTbd.GetFileName());

    for( int i=0; i < iTotalBreaks; i++ ) {

        // end the process if the user clicks cancel
        if( m_pIntDriver->m_bStopProc )
            break;

        CString& csBreakKey=m_aBreakKeys.ElementAt(i);
        int      iBreakKeyNum=m_aBreakNumKeys.ElementAt(i);// RHF Apr 17, 2003

        // Load Break for used Crosstabs
        sLoadedBreakKey = csBreakKey;
        LoadBreak( csBreakKey, iBreakKeyNum, &m_aUsedCtabs, i, iTotalBreaks );

        process_summary_reporter->SetKey(sLoadedBreakKey);

        try
        {
            // For each Level
            for( int iLevel=1; iLevel < (int)MaxNumberLevels; iLevel++ ) {
                int     iCurLevel  = iLevel;
                int     iProgIndex = ( iCurLevel ) ? m_iLevProg[iCurLevel] : -1;

                if( iProgIndex >= 0 ) {
                    // set the starting proc for this level
                    SetCurBatchProgIndex( iProgIndex );

                    CPrSlot*    pPrSlot = GetCurBatchProg();

                    while( true ) {
                        ASSERT( pPrSlot != NULL );

                        if( pPrSlot->IsEmptySlot() )
                            break;          // no more progs at this level

                        int iSymbol = pPrSlot->GetSlotSymbol();

                        switch( pPrSlot->GetSlotType() )
                        {
                            case CPrSlot::LEVELslot:
                            {
                                m_pEngineDriver->GetLister()->SetMessageSource(FormatTextCS2WS(_T("LEVEL %s, Break '%s'"),
                                                                                               GPT(iSymbol)->GetName().c_str(),
                                                                                               GetCurrentBreakKey().GetString()));

                                m_pIntDriver->ExecuteProcGroup(iSymbol, pPrSlot->GetProcType(), false);
                                break;
                            }

                            case CPrSlot::CTslot:
                            {
                                m_pEngineDriver->GetLister()->SetMessageSource(FormatTextCS2WS(_T("TABLE %s, Break '%s'"),
                                                                                               NPT(iSymbol)->GetName().c_str(),
                                                                                               GetCurrentBreakKey().GetString()));

                                m_pIntDriver->ExecuteProcTable(iSymbol, pPrSlot->GetProcType());
                                break;
                            }

                            default:
                            {
                                ASSERT( 0 );// shouldn't happen (no other types)
                                break;
                            }
                        }

                        // advance the moving index
                        pPrSlot = GetNextBatchProg();
                    }
                }
            }
        }

        // endcase and skip case will stop executing any additional procs
        catch( const EndCaseProgramControlException& )  { }
        catch( const SkipCaseProgramControlException& ) { }

        // RHF INIC Apr 17, 2003
        if( GetNumCtabToWrite() > 0 ) {
            m_Tbd.breakend();
        }
        // RHF END Apr 17, 2003
    }
}


bool CCalcDriver::LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks )
{
    try
    {
        if( GetLister() == nullptr )
            OpenListerAndWriteFiles();
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Abort, exception);
        return false;
    }

    //Now we are sending the full break key in the new idx format
    /*int iTableNumForCurrentBreakKey = */CTbdFile::GetTableNum(csCurrentBreakKey, sizeof(short));
    m_pEngineDriver->GetLister()->SetMessageSource(FormatTextCS2WS(_T("Loading Break '%s'"), csCurrentBreakKey.GetString()));

    CString csBreakKeyNoTableNum;
    //break key is blank when the area breaks are not present.
    if(!csCurrentBreakKey.IsEmpty())
        csBreakKeyNoTableNum = csCurrentBreakKey.GetString() + sizeof(short) / sizeof(TCHAR);

    //set the current break key without tablenum - the breaksave adds the tablenum again
    SetCurrentBreakKey(csBreakKeyNoTableNum);
    SetCurrentBreakKeyNum( iBreakKeyNum );

    CString      csFullKey;
    CString      csLoadedBreak;
    //Since the storage now is in TCHAR's we need to have the length in terms of number of TCHARS - Savy for Unicode -01/31/2012
    TCHAR        pszTableNum[sizeof(short)/sizeof(TCHAR)+1];
    const TCHAR* pKey;
    const TCHAR* pszKey;
    long         lFilePos;

    CTbdFile*   pTbdFile=GetInputTbd();
    CTbiFile*   pTbiFile=GetInputTbi();

    int         iCurrentSlice=(iCurrentBreak*aUsedCtabs->GetSize());
    int         iTotalSlices=(iNumBreaks*aUsedCtabs->GetSize());

    auto process_summary = m_pEngineDriver->GetProcessSummary();

    for( int iCtab=0; iCtab < aUsedCtabs->GetSize(); iCtab++ ) {

        CTAB* pCtab = m_engineData->crosstabs[iCtab];

        CTbdSlice*  pTbdSlice =  pCtab->GetTbdSlice();
        //ASSERT( pTbdSlice != NULL );
        //ASSERT( pTbdSlice->GetAcum() != NULL );

        // 20110625 declaring an array in a table application was crashing the system because the tabulation
        // system was trying to process it here; so I'm turned the above ASSERTS into conditions on whether to proceed
        if( pTbdSlice == NULL || pTbdSlice->GetAcum() == NULL )
            continue;


        bool        bHasBreak=(pCtab->m_uEnviron[0] & ct_BREAK );

        CTbdFile::SetTableNum(pszTableNum, (short)(iCtab + 1), sizeof(short));
        //Since the storage now is in TCHAR's we need to have the length in terms of number of TCHARS - Savy for Unicode -01/31/2012
        pszTableNum[sizeof(short)/sizeof(TCHAR)]=0;


        // RHF INIC Apr 17, 2003. Only Load the prefixs break.
        if( pCtab->GetNumBreaks() > iBreakKeyNum )
            continue;

        if( !bHasBreak )
            ASSERT( pCtab->GetNumBreaks() == 0 );

        int iBreakLen = m_Tbd.breaklen( pCtab->GetNumBreaks() );

        //get the break for display
        csLoadedBreak = bHasBreak ? csBreakKeyNoTableNum.Mid(0,iBreakLen) : _T("");
        if(!csLoadedBreak.IsEmpty()){//Set the correct break key for display
            sLoadedBreakKey = csLoadedBreak;
        }
        // RHF END Apr 17, 2003


        // If table doesn't have break, load the unique slice
        // RHF COM Apr 17, 2003 csLoadedBreak = bHasBreak ? csCurrentBreakKey : "";


        //the full key should be generated for each table with the current table number and the breakkey with no table number combined
        csFullKey.Format(_T("%s%s"), pszTableNum, csLoadedBreak.GetString());


        int     iFullKeyLenAux=csFullKey.GetLength();// RHF Apr 17, 2003

        bool    bRet = pTbiFile->Locate(CTbiFile::Exact, &csFullKey, &iFullKeyLenAux );

        if( bRet ) {
            // GetInfo
            pKey = pTbiFile->GetCurrentReg();

            ASSERT( CTbdFile::GetTableNum(pKey, sizeof(short)) == iCtab + 1 );

            lFilePos = pTbiFile->GetCurrentOffset()-1;
            pszKey = pKey + sizeof(short)/sizeof(TCHAR);

            // Load Slice in Ctab
            ASSERT(pTbdFile != NULL );

            int     iTbdFile=pTbdFile->GetFd();


            TBD_SLICE_HDR   oldSlice;

            memcpy( &oldSlice, pTbdSlice->GetSliceHdr(), sizeof(TBD_SLICE_HDR) );

            ASSERT(iTbdFile >= 0 );
            if( !pTbdSlice->Load( iTbdFile, lFilePos ) ) {
                bRet = false;
                issaerror( MessageType::Error, 2300, csLoadedBreak.GetString(), pCtab->GetName().c_str(), pTbdFile->GetFileName().GetString() ); // Can't load
                process_summary->IncrementAttributesUnknown();
            }
            else if( memcmp( &oldSlice, pTbdSlice->GetSliceHdr(), sizeof(TBD_SLICE_HDR) ) != 0 ) {
                bRet = false;
                issaerror( MessageType::Error, 2302, csLoadedBreak.GetString(), pCtab->GetName().c_str(), pTbdFile->GetFileName().GetString() ); // Invalid Slice
                process_summary->IncrementAttributesUnknown();
            }
        }
        else {
            issaerror( MessageType::Warning, 2304, csLoadedBreak.GetString(), pCtab->GetName().c_str(), pTbdFile->GetFileName().GetString() ); // Key not found // RHF Apr 17, 2003 Now thd TBD contains tables with Break Prefixs!
            process_summary->IncrementAttributesErased();
        }

        if( bRet ) {
            process_summary->IncrementAttributesRead();
        }
        else {
            double  dZero=0;
            pTbdSlice->GetAcum()->Fill( (unsigned char*) &dZero );
        }


        iCurrentSlice++;
        int iPct = (int) ( (double) iCurrentSlice * 100 / iTotalSlices );
        process_summary->SetPercentSourceRead(iPct);
    }

    return true;
}

void CCalcDriver::SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray* aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs ) {
    m_aBreakKeys.RemoveAll();
    m_aBreakNumKeys.RemoveAll();
    m_aUsedCtabs.RemoveAll();

    if( aBreakKeys != NULL  && aBreakKeys->GetSize() > 0 ) {
        for( int i=0; i < aBreakKeys->GetSize(); i++ )
            m_aBreakKeys.Add( aBreakKeys->ElementAt(i) );
    }

    if( aBreakNumKeys != NULL  && aBreakNumKeys->GetSize() > 0 ) {
        for( int i=0; i < aBreakNumKeys->GetSize(); i++ )
            m_aBreakNumKeys.Add( aBreakNumKeys->ElementAt(i) );
    }

    if( aUsedCtabs != NULL && aUsedCtabs->GetSize() > 0 ) {
        for( int i=0; i < aUsedCtabs->GetSize(); i++ )
            m_aUsedCtabs.Add( aUsedCtabs->ElementAt(i) );
    }

}

