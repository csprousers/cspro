// RunApl.cpp: implementation of the CRunApl class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "runapl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif
#include <ZBRIDGEO/npff.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRunApl::CRunApl()
{
    m_pPifFile = NULL;

    m_bFlagAppLoaded = false;
    m_bFlagStart = false;
    SetAppMode( CRUNAPL_NONE );
}

CRunApl::~CRunApl()
{
    Stop();
    End();
}

// Compile. true if OK
bool CRunApl::LoadCompile()
{
    if(m_pPifFile->GetApplication()->IsCompiled()){ //SAVY 03 May 2002
        m_bFlagAppLoaded  = false;
    }

    if( m_bFlagAppLoaded )
        return false;

    else
        m_bFlagAppLoaded = true;

    return true;
}

// Inform to engine that application was finished.
bool CRunApl::End() {
    if( !m_bFlagAppLoaded )
        return( false );
    m_bFlagAppLoaded = false;

    return( true );
}


// Open Dat/IDX. Let's ready to run.
bool CRunApl::Start()
{
    if( !m_bFlagAppLoaded )
        return( false );

    if( m_bFlagStart )
        return( false );
    else
        m_bFlagStart = true;

    return( true );
}

// Closes DAT/IDX and LST.
bool CRunApl::Stop() {
    if( !m_bFlagStart )
        return( false );
    m_bFlagStart = false;

    return( true );
}
