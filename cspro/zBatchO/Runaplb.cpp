// RunAplB.cpp: implementation of the CRunAplB class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "Runaplb.h"
#include <engine/BATIFAZ.H>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRunAplBatch::CRunAplBatch(CNPifFile* pPifFile, bool bAsCtab )
{
    m_pPifFile = pPifFile;
    m_pBatchIFaz = new CBatchIFaz;

    if( bAsCtab ) {
        m_bAsBatch = false;
    }
    else {
        m_bAsBatch = true;
    }
}

CRunAplBatch::~CRunAplBatch()
{
    Stop();
    End( false );

     if( m_pBatchIFaz != NULL )
        delete m_pBatchIFaz;
    m_pBatchIFaz = NULL;

}

bool CRunAplBatch::LoadCompile() {
    bool bRet = CRunApl::LoadCompile();
    if( bRet ) {
        // Call to CSPro Engine exapplinit (applload, attrload, compall..)
        // Errors are displayed trough issaerror function
        if(m_pPifFile->GetApplication()->IsCompiled() ) {

            if( !m_pBatchIFaz->C_BatchInit1( m_pPifFile, m_bAsBatch ? CRUNAPL_CSBATCH : CRUNAPL_CSTAB ) )
                bRet = false;
        }
        else  if( !m_pBatchIFaz->C_BatchInit( m_pPifFile, m_bAsBatch ?  CRUNAPL_CSBATCH : CRUNAPL_CSTAB ) )
            bRet = false;
    }

    return bRet;
}

// Inform to engine that application was finished.
bool CRunAplBatch::End( const bool bCanExit ) {
    bool  bRet = CRunApl::End();

    if( bRet )
        m_pBatchIFaz->C_BatchEnd( (int) bCanExit );

    return( bRet );
}

void CRunAplBatch::SetBatchMode( const int iBatchMode ) {
    ASSERT( iBatchMode == CRUNAPL_NONE || iBatchMode == CRUNAPL_CSBATCH || iBatchMode == CRUNAPL_CSTAB );

    m_pBatchIFaz->C_SetBatchMode( iBatchMode );
}

int CRunAplBatch::GetBatchMode() {
    return m_pBatchIFaz ? m_pBatchIFaz->C_GetBatchMode() : CRUNAPL_NONE;
}


// Open Dat/IDX. Let's ready to run.
bool CRunAplBatch::Start() {
    bool bRet=CRunApl::Start();

    if( bRet )
        bRet = m_pBatchIFaz->C_BatchStart(); // Call to the Engine

    SetAppMode( bRet ? m_pBatchIFaz->C_GetBatchMode() : CRUNAPL_NONE );

    return( bRet );
}

// Closes DAT/IDX and LST.
bool CRunAplBatch::Stop() {
    bool    bRet=CRunApl::Stop();

    if( bRet )
        m_pBatchIFaz->C_BatchStop();

    return( true );
}

bool CRunAplBatch::HasExport()
{
    return m_pBatchIFaz->HasExport();
}

const CEngineArea* CRunAplBatch::GetEngineArea() const
{
    return m_pBatchIFaz->GetEngineArea();
}
