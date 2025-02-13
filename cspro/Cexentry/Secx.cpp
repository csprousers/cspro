#include "STDAFX.H"
#include <engine/Exappl.h>
#include <zToolsO/Tools.h>


//////////////////////////////////////////////////////////////////////////////
//
// --- Initialization
//
//////////////////////////////////////////////////////////////////////////////

void SECX::Init( void ) {
    m_pDicX      = NULL;                // DICX  entry address
    m_pSecT      = NULL;                // SECT  entry address

    // --- basic data
    SetOccurrences( 0 );                // Sect  actual number of occurences
    SetPartialOccurrences( 0 );                // Sect  actual partial number of occurences // RHF May 07, 2004
// RHF COM Jun 05, 2001    SetProgAddress( NULL );             // Addr.of object procedure text in Opro area

    // --- data management by occurrence
    SetSecAsciiSize( 0 );               // the size of one record (full, plus 2)
    m_iFloatSize = 0;                   // the number of floats required
    m_iFlagsSize = 0;                   // the number of flags required
    m_apSecOcc   = nullptr;             // the SECX' occurrences array

    // --- legacy data - OBSOLETE
    seclen       = 0;                   //       data length in section record
}


//////////////////////////////////////////////////////////////////////////////
//
// --- Data areas for one SECX' occurrence
//
//////////////////////////////////////////////////////////////////////////////

void CSecOcc::BuildAreas( int iAsciiSize, int iFloatSize, int iFlagsSize ) {
    BuildAsciiArea( iAsciiSize );
    BuildFloatArea( iFloatSize );
    BuildFlagsArea( iFlagsSize );
}

void CSecOcc::BuildAsciiArea( int iAsciiSize ) {
    if( iAsciiSize > 0 ) {
        EraseAsciiArea();
        m_pAsciiArea = (csprochar*) calloc( iAsciiSize, sizeof(csprochar) );
    }
}

void CSecOcc::BuildFloatArea( int iFloatSize ) {
    if( iFloatSize > 0 ) {
        EraseFloatArea();
        m_pFloatArea = (double*) calloc( iFloatSize, sizeof(double) );
    }
}

void CSecOcc::BuildFlagsArea( int iFlagsSize ) {
    if( iFlagsSize > 0 ) {
        EraseFlagsArea();
        m_pFlagsArea = (csprochar*) calloc( iFlagsSize, sizeof(csprochar) );
    }
}

void CSecOcc::EraseAreas( void ) {
    EraseAsciiArea();
    EraseFloatArea();
    EraseFlagsArea();
}

void CSecOcc::EraseAsciiArea( void ) {
    if( m_pAsciiArea != NULL )
        free( m_pAsciiArea );
    m_pAsciiArea = NULL;
}

void CSecOcc::EraseFloatArea( void ) {
    if( m_pFloatArea != NULL )
        free( m_pFloatArea );
    m_pFloatArea = NULL;
}

void CSecOcc::EraseFlagsArea( void ) {
    if( m_pFlagsArea != NULL )
        free( m_pFlagsArea );
    m_pFlagsArea = NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
// --- Data management by occurrence                    // victor Jul 04, 00
//
//////////////////////////////////////////////////////////////////////////////

csprochar* SECX::GetAsciiAreaAtOccur( int iOccur ) {
        ASSERT( iOccur >= 1 );
    CSecOcc*    pSecOcc = GetSecOccAt( iOccur );
    csprochar*       pAsciiArea = ( pSecOcc != NULL ) ? pSecOcc->GetAsciiArea() : NULL;

    return pAsciiArea;
}

double* SECX::GetFloatAreaAtOccur( int iOccur ) {
        ASSERT( iOccur >= 1 );
    CSecOcc*    pSecOcc = GetSecOccAt( iOccur );
    double*     pFloatArea = ( pSecOcc != NULL ) ? pSecOcc->GetFloatArea() : NULL;

    return pFloatArea;
}

csprochar* SECX::GetFlagsAreaAtOccur( int iOccur ) {
        ASSERT( iOccur >= 1 );
    CSecOcc*    pSecOcc = GetSecOccAt( iOccur );
    csprochar*       pFlagsArea = ( pSecOcc != NULL ) ? pSecOcc->GetFlagsArea() : NULL;

    return pFlagsArea;
}

CSecOcc* SECX::GetSecOccAt( int iOccur ) {
        ASSERT( iOccur >= 1 );
    CSecOcc*    pSecOcc = ( iOccur > 0 && iOccur <= m_pSecT->GetMaxOccs() ) ?
                          m_apSecOcc->at(iOccur - 1) : NULL;
    return pSecOcc;
}

void SECX::BuildSecOccArray( void ) {
    // BuildSecOccArray: build 'max-occs' entries for the SECX' occurrences array
    ASSERT( m_apSecOcc == nullptr );
    int     iMaxOccs = m_pSecT->GetMaxOccs();

    m_apSecOcc = new std::vector<CSecOcc*>;

    for( int iOccur = 1; iOccur <= iMaxOccs; iOccur++ ) {
        CSecOcc*    pSecOcc = new CSecOcc;

        pSecOcc->BuildAreas( m_iAsciiSize, m_iFloatSize, m_iFlagsSize );

        m_apSecOcc->emplace_back( pSecOcc );
    }
}

void SECX::EraseSecOccArray( void ) {
    if( m_apSecOcc == nullptr ) {
        //ASSERT( 0 ); // Why is NULL?
        return;
    }

    int     iMaxOccs = m_pSecT->GetMaxOccs();
    int     iNumSlots = (int)m_apSecOcc->size();
    ASSERT( iNumSlots == iMaxOccs );

    for( int iOccur = 1; iOccur <= iMaxOccs; iOccur++ ) {
        CSecOcc*    pSecOcc = m_apSecOcc->at(iOccur - 1);

        pSecOcc->EraseAreas();

        delete pSecOcc;
    }

    delete m_apSecOcc;
    m_apSecOcc = nullptr;
}

void SECX::InitSecOccArray( double dInitValue, csprochar cInitFlags, int iMaxOccs ) { // RHF May 14, 2003 Add iMaxOccs
    // InitSecOccArray: initializes data areas of the Section
    // ... dInitValue: initial double value to set (zeros for workDict' Sections, NA otherwise)
    // ... cInitFlags: initial "color" flags
    //                 - color: HIGHLIHT for batch or non-input dicts, NOLIGHT for input dict in ENTRY
    // RHF COM May 14, 2003int     iMaxOccs = m_pSecT->GetMaxOccs();

    for( int iOccur = 1; iOccur <= iMaxOccs; iOccur++ ) {
        CSecOcc*    pSecOcc = m_apSecOcc->at(iOccur - 1);

        InitOneOccAreas( pSecOcc, dInitValue, cInitFlags );
    }
}

namespace
{
    void repmem( char *to, char *vt, int nv, int nt )
    {
        while( nt-- > 0 )
        {
            memcpy( to, vt, nv );
            to += nv;
        }
    }
}

void SECX::InitOneOccAreas( CSecOcc* pSecOcc, double dInitValue, csprochar cInitFlags ) {
    // InitOneOccAreas: initializes data areas of one occurrence of the Section
    ASSERT( pSecOcc != NULL );
    if( pSecOcc == NULL )
        return;

    csprochar*       pAsciiArea = pSecOcc->GetAsciiArea();
    double*     pFloatArea = pSecOcc->GetFloatArea();
    csprochar*       pFlagsArea = pSecOcc->GetFlagsArea();

    // set ascii area to blank
    if( pAsciiArea != NULL )
        _tmemset( pAsciiArea, BLANK, m_iAsciiSize );

    // set initial double values
    if( pFloatArea != NULL )
        repmem( (char*) pFloatArea, (char*) (&dInitValue), sizeof(double), m_iFloatSize );

    // set "color" and "valid marks" flags
    if( pFlagsArea != NULL )
        _tmemset( pFlagsArea, cInitFlags, m_iFlagsSize );
}


//////////////////////////////////////////////////////////////////////////////
//
// --- CEngineArea::methods
//
//////////////////////////////////////////////////////////////////////////////

int CEngineArea::SecxStart() {
    return( 0 );
}

void CEngineArea::SecxEnd()
{
    for( SECT* pSecT : m_engineData->sections )
    {
        SECX* pSecX = pSecT->GetSecX();
        pSecX->EraseSecOccArray(); // victor Jul 04, 00
    }
}
