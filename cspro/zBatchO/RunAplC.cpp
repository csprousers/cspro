// RunAplC.cpp: implementation of the CRunAplCsCalc class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "RunAplC.h"
#include <engine/calcifaz.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRunAplCsCalc::CRunAplCsCalc(CNPifFile* pPifFile)
{
    m_pPifFile = pPifFile;
    m_pCalcIFaz = new CCalcIFaz;
}

CRunAplCsCalc::~CRunAplCsCalc()
{
    Stop();
    End( false );

     if( m_pCalcIFaz != NULL )
        delete m_pCalcIFaz;
    m_pCalcIFaz = NULL;

}

bool CRunAplCsCalc::LoadCompile() {
    bool bRet = CRunApl::LoadCompile();

    if( bRet ) {
        // Call to CSPro Engine exapplinit (applload, attrload, compall..)
        // Errors are displayed trough issaerror function
        if(m_pPifFile->GetApplication()->IsCompiled() ) {
            if( !m_pCalcIFaz->C_BatchInit1( m_pPifFile, CRUNAPL_CSCALC ) )
                bRet = false;
        }
        else  if( !m_pCalcIFaz->C_BatchInit( m_pPifFile, CRUNAPL_CSCALC ) )
            bRet = false;
    }

    return( bRet );
}

// Inform to engine that application was finished.
bool CRunAplCsCalc::End( const bool bCanExit ) {
    bool  bRet = CRunApl::End();

    if( bRet )
        m_pCalcIFaz->C_BatchEnd( (int) bCanExit );

    return( bRet );
}

void CRunAplCsCalc::SetBatchMode( const int iBatchMode ) {
    ASSERT( iBatchMode == CRUNAPL_NONE || iBatchMode == CRUNAPL_CSCALC );

    m_pCalcIFaz->C_SetBatchMode( iBatchMode );
}

// Open Dat/IDX. Let's ready to run.
bool CRunAplCsCalc::Start() {
    bool bRet=CRunApl::Start();

    if( bRet )
        bRet = m_pCalcIFaz->C_BatchStart(); // Call to the Engine

    SetAppMode( bRet ? m_pCalcIFaz->C_GetBatchMode() : CRUNAPL_NONE );

    return( bRet );
}

// Closes DAT/IDX and LST.
bool CRunAplCsCalc::Stop() {
    bool    bRet=CRunApl::Stop();

    if( bRet )
        m_pCalcIFaz->C_BatchStop();

    return( true );
}


// For the application
int  CRunAplCsCalc::GetAppCtabs( CArray <CTAB*, CTAB*>& aCtabs ) {
    return  m_pCalcIFaz->C_GetAppCtabs( aCtabs );
}

// For Each TBD
bool CRunAplCsCalc::OpenInputTbd( CString csInputTbdName ) {
    return  m_pCalcIFaz->C_OpenInputTbd( csInputTbdName );
}

void CRunAplCsCalc::CloseInputTbd() {
    m_pCalcIFaz->C_CloseInputTbd();
}


int CRunAplCsCalc::GetTbdCtabs( CArray<CTAB*,CTAB*>& aUsedCtabs,
                                CArray<CTAB*,CTAB*>& aUnUsedCtabs,
                                CArray<CTbdTable*,CTbdTable*>& aTables
                                ) {

    return m_pCalcIFaz->C_GetTbdCtabs( aUsedCtabs,  aUnUsedCtabs, aTables );
}

int CRunAplCsCalc::GetTbdBreakKeys( CStringArray& aBreakKey, CUIntArray& aBreakNumKeys ) {
   return m_pCalcIFaz->C_GetTbdBreakKeys( aBreakKey, aBreakNumKeys );
}

// Between TBD, XTS & APP. IF Tbd has less tables than the APP tables
// show a warning (show error if it not a subset).
bool CRunAplCsCalc::CheckIntegrity( CStringArray& csErrorMsg ) {
    return m_pCalcIFaz->C_CheckIntegrity( csErrorMsg );
}

//For each break in the current TBD
bool CRunAplCsCalc::LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks ) {
    return m_pCalcIFaz->C_LoadBreak( csCurrentBreakKey, iBreakKeyNum, aUsedCtabs, iCurrentBreak, iNumBreaks);
}

void CRunAplCsCalc::SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray*  aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs ) {
    m_pCalcIFaz->C_SetRunTimeBreakKeys( aBreakKeys, aBreakNumKeys, aUsedCtabs );
}

