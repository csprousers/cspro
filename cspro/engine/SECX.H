#pragma once

//---------------------------------------------------------------------------
//  File name: SecX.h
//
//  Description:
//          Header for executor-Sec parallel table
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Jul 99   RHF     Basic conversion
//              10 Jul 00   vc      Data management by occurrence, adding CSecOcc
//
//---------------------------------------------------------------------------
#include <engine/Tables.h>

//---------------------------------------------------------------------------
//
//  class CSecOcc : public CObject
//
//  Description:
//      Data areas for one SECX' occurrence
//
//  Construction/Destruction
//      CSecOcc                     Constructor
//      ~CSecOcc                    Destructor
//
//  Managing memory for areas
//      BuildAreas                  Create memory areas
//      BuildAsciiArea              Create memory area for ascii record'images
//      BuildFloatArea              Create memory area for float images of used numeric variables
//      BuildFlagsArea              Create memory area for flags of every variable in the record
//      EraseAreas                  Destroy memory areas
//      EraseAsciiArea              Destroy memory area for ascii record'images
//      EraseFloatArea              Destroy memory area for float images of used numeric variables
//      EraseFlagsArea              Destroy memory area for flags of every variable in the record
//
//---------------------------------------------------------------------------
class CSecOcc : public CObject {
private:
    csprochar*   m_pAsciiArea;
    double* m_pFloatArea;
    csprochar*   m_pFlagsArea;

    // --- constructor/destructor
public:
    CSecOcc() {
            m_pAsciiArea = NULL;
            m_pFloatArea = NULL;
            m_pFlagsArea = NULL;
    }
    ~CSecOcc() { EraseAreas(); }

    // --- managing memory for areas
public:
    void    BuildAreas( int iAsciiSize, int iFloatSize, int iFlagsSize );
    void    BuildAsciiArea( int iAsciiSize );
    void    BuildFloatArea( int iFloatSize );
    void    BuildFlagsArea( int iFlagsSize );
    void    EraseAreas( void );
    void    EraseAsciiArea( void );
    void    EraseFloatArea( void );
    void    EraseFlagsArea( void );
    csprochar*   GetAsciiArea( void )        { return m_pAsciiArea; }
    double* GetFloatArea( void )        { return m_pFloatArea; }
    csprochar*   GetFlagsArea( void )        { return m_pFlagsArea; }
};



//---------------------------------------------------------------------------
//
//  struct SECX
//
//  Description:
//      Parallel Sections' table management
//
//  Construction/Destruction/Initialization
//      Init                        Initialization
//
//  Related objects
//      GetSecT                     Return the pointer to the SECT parallel to this SECX
//      SetSecT                     Set the pointer to the SECT parallel to this SECX
//
//  Basic data
//      GetOccurrences
//      SetOccurrences
//
//  Data management by occurrence
//      GetAsciiAreaAtOccur
//      GetFloatAreaAtOccur
//      GetFlagsAreaAtOccur
//      GetSecOccAt
//      BuildSecOccArray
//      EraseSecOccArray
//      InitSecOccArray             Initializes data areas of the Section (Floats and Flags are set to given values)
//      InitOneOccAreas             Initializes data areas of one occurrence of the Section
//
//---------------------------------------------------------------------------

struct SECX {
    // SECX: executor parallel Section  table

// --- Data members --------------------------------------------------------
    // --- related objects
private:
    DICX*   m_pDicX;                    // DICX* owner  // victor Jul 04, 00
    SECT*   m_pSecT;                    // SECT* brother// victor Jul 04, 00

    // --- basic data
public:
private:
    int     secnum;             // Sect  actual number of occurences
    int     m_iPartialOccurs;   // Sect  actual number of partial occurences RHF May 07, 2004
public:
// RHF COM Jun 05, 2001    int*    oproaddr;                   // Addr.of object procedure text in Opro area

    // --- data management by occurrence                // victor Jul 04, 00
public:
    int     m_iAsciiSize;               // the size of one record (full, plus 2)
    int     m_iFloatSize;               // the number of floats required
    int     m_iFlagsSize;               // the number of flags required
    std::vector<CSecOcc*>* m_apSecOcc;  // the SECX' occurrences array

    // --- legacy data - OBSOLETE
//private:
    int     seclen;                     //       data length in section record


// --- Methods -------------------------------------------------------------
    // --- initialization
public:
    void    Init( void );

    // --- related objects
public:
    DICX*   GetDicX( void )                     { return m_pDicX; }     // victor May 24, 00
    void    SetDicX( DICX* pDicX )              { m_pDicX = pDicX; }    // victor May 24, 00
    SECT*   GetSecT( void )                     { return m_pSecT; }     // victor May 24, 00
    void    SetSecT( SECT* pSecT )              { m_pSecT = pSecT; }    // victor May 24, 00

    // --- basic data
public:
    int     GetOccurrences( void )              { return secnum; }
    void    SetOccurrences( int iOccurs )       { secnum = iOccurs; }

    // RHF INIC May 07, 2004
    int     GetPartialOccurrences( void )              { return m_iPartialOccurs; }
    void    SetPartialOccurrences( int iPartialOccurs ){ m_iPartialOccurs = iPartialOccurs; }
    // RHF END May 07, 2004

    // --- data management by occurrence                // victor Jul 04, 00
public:
    void    SetSecAsciiSize( int iSize )        { m_iAsciiSize = ( iSize > 0 ? iSize + 2 : 0 ); }
    int     GetSecAsciiSize( void )             { return m_iAsciiSize; }
    int     GetSecDataSize( void )              { return( m_iAsciiSize > 2 ? m_iAsciiSize - 2 : 0 ); }
    csprochar*   GetAsciiAreaAtOccur( int iOccur );
    double* GetFloatAreaAtOccur( int iOccur );
    csprochar*   GetFlagsAreaAtOccur( int iOccur );
private:
    CSecOcc* GetSecOccAt( int iOccur );
public:
    void    BuildSecOccArray( void );
    void    EraseSecOccArray( void );
    // RHF COM May 14, 2003 void    InitSecOccArray( double dInitValue = NOTAPPL, csprochar cInitFlags = FLAG_NOLIGHT, int iMaxOccs ); // RHF May 14, 2003 Add iMaxOccs
    void    InitSecOccArray( double dInitValue, csprochar cInitFlags, int iMaxOccs );// RHF May 14, 2003
    void    InitOneOccAreas( CSecOcc* pSecOcc, double dInitValue, csprochar cInitFlags );
};
