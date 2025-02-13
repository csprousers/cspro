//---------------------------------------------------------------------------
//
//  BatIFaz: Interfaz for Batch Applications
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/3dException.h>
#include <engine/BATIFAZ.H>
#include <engine/Ctab.h>
#include <engine/IntDrive.h>
#include <engine/ParadataDriver.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zToolsO/WinSettings.h>
#include <ZBRIDGEO/PifDlg.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CBatchIFaz::CBatchIFaz()
{
    m_pBatchDriverBase  = NULL;
    m_pEngineArea       = NULL;
    m_engineData        = nullptr;
    m_pEngineDriver     = NULL;
    m_pEngineSettings   = NULL;
    m_pIntDriver        = NULL;

    m_bBatchStarted      = false;
    m_bBatchInited       = false;
    m_bTabProcessSpecial = false;
}

CBatchIFaz::~CBatchIFaz()
{
    DeleteBatchDriver();
}

void CBatchIFaz::DeleteBatchDriver()
{
    delete m_pBatchDriverBase;

    m_pBatchDriverBase  = NULL;
    m_pEngineArea       = NULL;
    m_engineData        = nullptr;
    m_pEngineDriver     = NULL;
    m_pEngineSettings   = NULL;
    m_pIntDriver        = NULL;
}

void CBatchIFaz::SetBatchDriver( CBatchDriverBase* pBatchDriverBase )
{
    m_pBatchDriverBase  = pBatchDriverBase;
    m_pEngineArea       = m_pBatchDriverBase->m_pEngineArea;
    m_engineData        = &m_pEngineArea->GetEngineData();
    m_pEngineDriver     = m_pBatchDriverBase;
    m_pEngineSettings   = m_pEngineDriver->m_pEngineSettings;
    m_pIntDriver        = m_pEngineDriver->m_pIntDriver.get();
    m_pEngineDriver->m_bTabProcessSpecial = m_bTabProcessSpecial;
}


const Logic::SymbolTable& CBatchIFaz::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


bool CBatchIFaz::C_BatchInit( CNPifFile* pPifFile, int iRunMode ) {
    bool    bCsBatch=( iRunMode == CRUNAPL_CSBATCH );
    bool    bCsCalc=( iRunMode == CRUNAPL_CSCALC );
    bool    bCsTab=( iRunMode == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );
    bool    bDone = false;

    if( m_bBatchInited )
        return bDone;

    CBatchDriverBase*   pBatchDriverBase;

    if( pPifFile->GetApplication()->GetApplicationLoader() == nullptr )
    {
        // APP_LOAD_TODO remove once tabs are working
        pPifFile->GetApplication()->SetApplicationLoader(std::make_unique<FileApplicationLoader>(pPifFile->GetApplication()));
    }

    // initializing batch driver
    if( bCsBatch || bCsTab ) {
        pBatchDriverBase = new CBatchDriver(pPifFile->GetApplication());

        SetBatchDriver( pBatchDriverBase ); // Set m_pEngineDriver also!
        pBatchDriverBase->SetBatchMode( bCsBatch ? CRUNAPL_CSBATCH : CRUNAPL_CSTAB );

        if( bCsBatch )
            m_pEngineDriver->m_lpszExecutorLabel =_T("BATCH");
        else
            m_pEngineDriver->m_lpszExecutorLabel = _T("CSTAB");

        if(pPifFile->GetSkipStructFlag()){
            GetSettings()->SetHasSkipStruc(true);
            GetSettings()->SetAutoSkipStruc(true);
        }
        if(pPifFile->GetChkRangesFlag()){
            GetSettings()->SetHasCheckRanges(true);
            GetSettings()->SetAutoCheckRanges( true );
        }
    }
    else {
        ASSERT( bCsCalc);

        pBatchDriverBase = new CCalcDriver(pPifFile->GetApplication());
        pBatchDriverBase->SetBatchMode( CRUNAPL_CSCALC );

        SetBatchDriver( pBatchDriverBase ); // Set m_pEngineDriver also!
        m_pEngineDriver->m_lpszExecutorLabel = _T("CALC");
    }

    CTbd*   pTbd = pBatchDriverBase->GetTbd();

    m_bBatchInited = true;

    Issamod = ModuleType::Batch;


    // initializing application
    m_pEngineDriver->SetPifFile( pPifFile );            // see Attr.cpp

    bDone = m_pEngineDriver->exapplinit();

    if( bDone ) {
        // RHF INIC May 07, 2003
        pBatchDriverBase->m_csOutputTbdName = pPifFile->GetTabOutputFName();

        if( pBatchDriverBase->m_csOutputTbdName.IsEmpty() )
            pBatchDriverBase->m_csOutputTbdName = PortableFunctions::PathRemoveFileExtension<CString>(pPifFile->GetAppFName()) + _T(".xxx");
        // RHF END May 07, 2003


        bool bHasAnyTable = ( pBatchDriverBase->GetNumCtabToWrite() > 0 );

        pBatchDriverBase->SetHasAnyTable( bHasAnyTable );

        pBatchDriverBase->BatchCreateProg();

        if( pBatchDriverBase->GetNumCtabToWrite() > 0 ) {
            // RHF INIC May 07, 2003
            if( !pTbd->breakinit( pBatchDriverBase->m_csOutputTbdName ) )
                issaerror( MessageType::Abort, 590, pBatchDriverBase->m_csOutputTbdName.GetString() );    // cannot open TBI
            // RHF END May 07, 2003
        }
    }

    return bDone;
}

//SAVY ADDED THIS FOR SINGLE COMPILE
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CBatchIFaz::C_BatchInit1( CArray<CString, CString>& m_aParams, CNPifFile* pPifFile, int iRunMode )
//
/////////////////////////////////////////////////////////////////////////////////
bool CBatchIFaz::C_BatchInit1( CNPifFile* pPifFile, int iRunMode )
{
    bool    bCsBatch=( iRunMode == CRUNAPL_CSBATCH );
    bool    bCsCalc=( iRunMode == CRUNAPL_CSCALC );
    bool    bCsTab=( iRunMode == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );
    bool    bDone = false;

    // initializing batch driver
    // RHF COM Oct 23, 2002 CTbd*   pTbd = m_pBatchDriverBase->GetTbd();

    m_bBatchInited = true;

    Issamod = ModuleType::Batch;

    if( bCsBatch )
        m_pEngineDriver->m_lpszExecutorLabel = _T("BATCH");
    else if( bCsTab )
        m_pEngineDriver->m_lpszExecutorLabel = _T("CSTAB");
    else {
        ASSERT(bCsCalc);
        m_pEngineDriver->m_lpszExecutorLabel = _T("CALC");
    }

    // initializing application
    m_pEngineDriver->SetPifFile( pPifFile );            // see Attr.cpp

    bDone = m_pEngineDriver->exapplinit();

    if( bDone ) {
        // RHF INIC May 07, 2003
        m_pBatchDriverBase->m_csOutputTbdName = pPifFile->GetTabOutputFName();

        if( m_pBatchDriverBase->m_csOutputTbdName.IsEmpty() )
            m_pBatchDriverBase->m_csOutputTbdName = PortableFunctions::PathRemoveFileExtension<CString>(pPifFile->GetAppFName()) + _T(".xxx");
        // RHF END May 07, 2003
    }

    return bDone;
}


void CBatchIFaz::C_SetBatchMode( int iBatchMode ) {
    ASSERT( iBatchMode == CRUNAPL_NONE || iBatchMode == CRUNAPL_CSBATCH || iBatchMode == CRUNAPL_CSCALC );

    m_pBatchDriverBase->SetBatchMode( iBatchMode );
}

int CBatchIFaz::C_GetBatchMode() {
    return m_pBatchDriverBase->GetBatchMode();
}


bool CBatchIFaz::C_BatchStart() {
    bool    bCsBatch=( C_GetBatchMode() == CRUNAPL_CSBATCH );
    bool    bCsCalc=( C_GetBatchMode() == CRUNAPL_CSCALC );
    bool    bCsTab=( C_GetBatchMode() == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );

    if( m_bBatchStarted )
        return false;

    m_bBatchStarted = true;

    if( bCsBatch || bCsTab ) {
        // set primary flow as current
        m_pEngineDriver->SetFlowInProcess( Appl.GetFlowAt( 0 ) );
        m_pEngineSettings->SetPathOff();
    }

    if( m_pBatchDriverBase->RunInit() )
    {
        try
        {
            m_pBatchDriverBase->RunDriver();
        }

        catch( Fatal3DException& fe )
        {
            issaerror( MessageType::Error, 36000, fe.getErrorCode() );
        }
    }

    return true;
}



#define COLON _T(':')

// RHF 20/8/97 Se permite pasar el path vacio : ""
void strmfp( TCHAR *name, const TCHAR *path, const TCHAR *node )
{
    csprochar     c;
    int      last;

    _tcscpy( name, path );
    last = _tcslen(name)-1;
    if( last >= 0 && (c = name[last]) != 0 && c != PATH_CHAR && c != COLON )
        _tcscat( name, _T("\\") );
    _tcscat( name, node );
}


void CBatchIFaz::C_BatchStop() {
    bool    bCsBatch=( C_GetBatchMode() == CRUNAPL_CSBATCH );
    bool    bCsCalc=( C_GetBatchMode() == CRUNAPL_CSCALC );
    bool    bCsTab=( C_GetBatchMode() == CRUNAPL_CSTAB );
    ASSERT( bCsBatch || bCsCalc || bCsTab );

    if( !m_bBatchStarted )
        return;

    m_bBatchStarted = false;

    m_pBatchDriverBase->RunEnd();

    if( bCsCalc )
        return;

    // reset flow in process
    m_pEngineDriver->ResetFlowInProcess();

    if( !m_pBatchDriverBase->GetHasAnyTable() ) {
        return;
    }

    ASSERT( bCsBatch || bCsTab );
    TCHAR    pszProgName[_MAX_PATH];
    TCHAR    mark[2];

    // some table in TBD file should be printed by exprtab
    CString Mod_path;
    GetModuleFileName(AfxGetApp()->m_hInstance, Mod_path.GetBuffer(MAX_PATH), MAX_PATH);
    Mod_path.ReleaseBuffer();

    PathRemoveFileSpec(Mod_path.GetBuffer(MAX_PATH));
    Mod_path.ReleaseBuffer();

    strmfp( pszProgName, Mod_path, ((CBatchDriver*)m_pBatchDriverBase)->m_csExprtab );
    *mark = 0;

    if( !CSettings::m_bNewTbd ) {
        if( _tspawnl( _P_WAIT, pszProgName, ((CBatchDriver*)m_pBatchDriverBase)->m_csExprtab.GetString(), m_pBatchDriverBase->m_csOutputTbdName.GetString(), mark, NULL ) != 0 ) {
            CString csMsg;

            csMsg.Format( _T("Error invoking '%s'"), pszProgName );
            issaerror( MessageType::Error, MGF::OpenMessage, csMsg.GetString() );
        }
    }
}


void CBatchIFaz::C_BatchEnd( int bCanExit ) {
    bool    bCsBatch=( C_GetBatchMode() == CRUNAPL_CSBATCH );
    bool    bCsCalc=( C_GetBatchMode() == CRUNAPL_CSCALC );
    bool    bCsTab=( C_GetBatchMode() == CRUNAPL_CSTAB);
    ASSERT( bCsBatch || bCsCalc || bCsTab );

    UNREFERENCED_PARAMETER( bCanExit );
    if( !m_bBatchInited )
        return;

    m_bBatchInited = false;

    m_pBatchDriverBase->SetHasAnyTable(false);

    // RHF INIC Oct 22, 2002
    if( bCsCalc ) {
        CTbd*   pTbd = m_pBatchDriverBase->GetTbd();

        if( pTbd != NULL )
            pTbd->breakclose();
    }
    // RHF END Oct 22, 2002


    m_pEngineArea->tablesend();

    C_SetBatchMode( CRUNAPL_NONE );

    DeleteBatchDriver();
}

bool CBatchIFaz::HasExport()
{
    ASSERT( m_pEngineArea != NULL );
    return m_pEngineArea->ExportGetSize() > 0;
}
