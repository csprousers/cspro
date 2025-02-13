//---------------------------------------------------------------------------
//  File name: acum.cpp
//
//  Description:
//          Implementation for 3-d acumulator class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Aug 01   RHF     Creation
//
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "acum.h"
#include "Special.h"


#define SIZE_OF_SHORT       2
#define SIZE_OF_LONG        4
#define SIZE_OF_DOUBLE      8


CAcum::CAcum() {
        Init();
}

CAcum::CAcum( CAcum& rAcum ) {
        //  Free();

        m_iCellSize = rAcum.GetCellSize();
        m_iNumDim = rAcum.GetNumDim();
        m_iNumRows = rAcum.GetNumRows();
        m_iNumCols = rAcum.GetNumCols();
        m_iNumLayers = rAcum.GetNumLayers();
        m_iNumCells = rAcum.GetNumCells();

        m_pDefaultValue = (byte*) malloc( m_iCellSize );
        memcpy( m_pDefaultValue, rAcum.GetDefaultValue(), m_iCellSize );

        m_bMustFree = true;

        m_pAcumArea = (byte*) malloc( m_iNumCells * m_iCellSize * sizeof(byte) );
        memcpy( m_pAcumArea, rAcum.GetAcumArea(), m_iNumCells * m_iCellSize * sizeof(byte) );

}


CAcum::~CAcum() {
        Free();

}


void    CAcum::SetDefaultValue( byte* pDefaultValue ) {
        if( m_pDefaultValue == NULL )
                return;
        memcpy( m_pDefaultValue, pDefaultValue, m_iCellSize);
}

byte*   CAcum::GetDefaultValue() {
        return m_pDefaultValue;
}

void CAcum::Init() {
        m_pAcumArea = NULL;

        m_iCellSize = 0;
        m_iNumDim = 0;
        m_iNumRows = 0;
        m_iNumCols = 0;
        m_iNumLayers = 0;
        m_iNumCells = 0;
        m_pDefaultValue = NULL;
        m_bMustFree = true;
}

// RHF INIC Jul 29, 2002
// iRow,iCol, iLayer are 0 based. -1 if error
int CAcum::GetOffSetPos( int iRow, int  iCol/*=0*/, int iLayer/*=0*/ ) {
        int     uCellIndex;

        if( m_pAcumArea == NULL )
                return( -1 );

        if( iRow >= m_iNumRows || iCol >= m_iNumCols || iLayer >= m_iNumLayers )
                return( -1 );

        if( m_iNumDim < 2 )
                iCol = iLayer = 0;
        else if( m_iNumDim < 3 )
                iLayer = 0;

        if( iRow < 0 || iRow >= m_iNumRows ||
                iCol < 0 || iCol >= m_iNumCols ||
                iLayer < 0 || iLayer >= m_iNumLayers )
                return( -1 );

        uCellIndex = ( iLayer * m_iNumCols + iCol ) * m_iNumRows + iRow;

        return uCellIndex*m_iCellSize;
}
// RHF END Jul 29, 2002


// iRow,iCol, iLayer are 0 based
byte *CAcum::GetOffSet( int iRow, int  iCol/*=0*/, int iLayer/*=0*/ ) {
        int     uCellIndex;

        if( m_pAcumArea == NULL )
                return( NULL );

        if( iRow >= m_iNumRows || iCol >= m_iNumCols || iLayer >= m_iNumLayers )
                return( NULL );

        if( m_iNumDim < 2 )
                iCol = iLayer = 0;
        else if( m_iNumDim < 3 )
                iLayer = 0;

        if( iRow < 0 || iRow >= m_iNumRows ||
                iCol < 0 || iCol >= m_iNumCols ||
                iLayer < 0 || iLayer >= m_iNumLayers )
                return( NULL );


        uCellIndex = ( iLayer * m_iNumCols + iCol ) * m_iNumRows + iRow;

        return( m_pAcumArea + uCellIndex * m_iCellSize );
}


byte* CAcum::Alloc( int iCellSize, int iNumRows, int iNumCols/*=0*/, int iNumLayers/*=0*/, byte* pDefaultValue/*=NULL*/ ) {
        if( m_pAcumArea != NULL ) // Already allocated or shared
                return NULL;

        if( m_pDefaultValue != NULL )// Already allocated or shared
                return NULL;

        if( iNumRows <= 0 || (iNumCols <= 0 && iNumLayers > 0 ) )
                return NULL;

        ASSERT( iCellSize > 0 );

        m_iCellSize = iCellSize;
        m_iNumRows = iNumRows;
        m_iNumCols = iNumCols;
        m_iNumLayers = iNumLayers;

        m_iNumDim = 0;
        if( m_iNumRows > 0 ) {
                m_iNumDim++;
                m_iNumCells = m_iNumRows;
        }
        else
                m_iNumRows = 1;

        if( m_iNumCols > 0 ) {
                m_iNumDim++;
                m_iNumCells *= m_iNumCols;
        }
        else
                m_iNumCols = 1;

        if( m_iNumLayers > 0 ) {
                m_iNumDim++;
                m_iNumCells *= m_iNumLayers;
        }
        else
                m_iNumLayers = 1;

        m_pAcumArea = (byte*) malloc( m_iNumCells * m_iCellSize * sizeof(byte) );
        m_pDefaultValue = (byte*) malloc( m_iCellSize );

        if( pDefaultValue != NULL )
                memcpy( m_pDefaultValue, pDefaultValue, m_iCellSize );
        else
                memset( m_pDefaultValue, 0, m_iCellSize );

        Fill( m_pDefaultValue );

        m_bMustFree = true;

        return m_pAcumArea;
}

bool CAcum::ShareArea( CAcum& rAcum ) {
        if( m_pAcumArea != NULL ) // Already allocated or shared
                return false;

        m_iCellSize = rAcum.GetCellSize();
        m_iNumDim = rAcum.GetNumDim();
        m_iNumRows = rAcum.GetNumRows();
        m_iNumCols = rAcum.GetNumCols();
        m_iNumLayers = rAcum.GetNumLayers();
        m_iNumCells = rAcum.GetNumCells();
        m_pDefaultValue = rAcum.GetDefaultValue();
        m_pAcumArea = rAcum.GetAcumArea();
        m_bMustFree = false;

        return true;
}

void CAcum::Free() {
        if( m_bMustFree ) {
                if( m_pDefaultValue != NULL )
                        free( m_pDefaultValue );

                if( m_pAcumArea != NULL )
                        free( m_pAcumArea );
        }

        Init();
}

double CAcum::GetDoubleValue( int iRow, int iCol/*=0*/, int iLayer/*=0*/ ) {
        ASSERT( m_iCellSize == SIZE_OF_SHORT || m_iCellSize == SIZE_OF_LONG || m_iCellSize == SIZE_OF_DOUBLE );

        byte*   pOffSet = GetOffSet( iRow, iCol, iLayer );

        if( pOffSet != NULL ) {
                switch( m_iCellSize ) {
                case SIZE_OF_SHORT:
                        return( ( double ) *( (short *) pOffSet ) );
                case SIZE_OF_LONG:
                        return( ( double ) *( (long *) pOffSet ) );
                case SIZE_OF_DOUBLE:
                        return( *( (double *) pOffSet ) );
                }
        }

        return( * ((double*) m_pDefaultValue)  );
}

bool CAcum::PutDoubleValue( double dValue, int iRow, int iCol/*=0*/, int iLayer/*=0*/ ) {
        bool    bRet=true;
        ASSERT( m_iCellSize == SIZE_OF_SHORT || m_iCellSize == SIZE_OF_LONG || m_iCellSize == SIZE_OF_DOUBLE );

        byte*   pOffSet = GetOffSet( iRow, iCol, iLayer );

        if( pOffSet != NULL ) {
                switch( m_iCellSize ) {
                case SIZE_OF_SHORT:
                        if( dValue == (short) dValue )
                                *( (short *) pOffSet ) = (short) dValue;
                        else
                                bRet = false;
                        break;
                case SIZE_OF_LONG:
                        if( dValue == (long) dValue )
                                *( (long *) pOffSet ) = (long) dValue;
                        else
                                bRet = false;
                        break;
                case SIZE_OF_DOUBLE:
                        *( (double *) pOffSet ) = dValue;
                        break;
                }
        }

        return bRet;
}

byte* CAcum::GetValue( int iRow, int iCol/*=0*/, int iLayer/*=0*/ ) {
        return GetOffSet( iRow, iCol, iLayer );
}

bool CAcum::PutValue( byte* pValue, int iRow, int iCol/*=0*/, int iLayer/*=0*/ ) {
        bool     bRet=true;

        byte*   pOffSet = GetOffSet( iRow, iCol, iLayer );
        if( pOffSet != NULL )
                memcpy( pOffSet, pValue, m_iCellSize );
        else
                bRet =false;

        return bRet;
}


byte*   CAcum::GetAcumArea()   { return m_pAcumArea; }
int     CAcum::GetCellSize()   { return m_iCellSize; }
int     CAcum::GetNumDim()     { return m_iNumDim; }
int     CAcum::GetNumRows()    { return m_iNumRows; }
int     CAcum::GetNumCols()    { return m_iNumCols; }
int     CAcum::GetNumLayers()  { return m_iNumLayers; }
int     CAcum::GetNumCells()   { return m_iNumCells; }


bool CAcum::AreSimilar( CAcum* pAcum ) {
        // Validate dim number
        if( GetNumDim() != pAcum->GetNumDim() )
                return false;

        if( GetNumRows() != pAcum->GetNumRows() )
                return false;

        if( GetNumCols() != pAcum->GetNumCols() )
                return false;

        if( GetNumLayers() != pAcum->GetNumLayers() )
                return false;

        // Valid Cell Size
        if( GetCellSize() != pAcum->GetCellSize() )
                return false;

        return true;
}

bool CAcum::Fill( byte* pFillValue ) {
        for( int iCell=0; iCell < m_iNumCells; iCell++ )
                memcpy( m_pAcumArea + iCell * m_iCellSize, pFillValue, m_iCellSize );

        return true;
}

// RHF INIC Jan 31, 2003
void CAcum::Dump( CString csFileName, CString csTitle, int iNumDec, bool bUseRowColNumbers ) {
#ifdef _MFC_VER
        CStringArray     aLines;

        Dump( aLines, csTitle, iNumDec, bUseRowColNumbers );

        CStdioFile  cFile;

        bool    bRemove=false;
        if( bRemove ) {
                try {
                        CFile::Remove( csFileName );
                }
                catch( ... ) {
                }
        }


        if (cFile.Open(csFileName, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate )) {
                cFile.SeekToEnd();

                for( int i=0; i < aLines.GetSize(); i++ )
                        cFile.Write( aLines.ElementAt(i), aLines.ElementAt(i).GetLength() );

                cFile.Close();
        }
#endif
}


void CAcum::Dump( CStringArray& aLines, CString csTitle, int iNumDec, bool bUseRowColNumbers ) {
        int         i, j, k;
        CString     csLine;

        // Calculates max code len for all layers
        int    iMaxColLen=0;
        for( k=0; k < GetNumLayers(); k++ ) {
                for( i=0; i < GetNumRows(); i++ ) {
                        csLine = IntToString( i + 1 );
                        iMaxColLen = std::max( iMaxColLen, csLine.GetLength() );
                        for( j=0; j < GetNumCols(); j++ ) {
                                double dValue=GetDoubleValue( i, j, k );
                                if( dValue == MISSING ) // MISSING
                                        csLine = _T("M");
                                else if( dValue == REFUSED ) // REFUSED
                                        csLine = _T("R");
                                else if( dValue == DEFAULT ) // DEFAULT
                                        csLine = _T("D");
                                else if( dValue == NOTAPPL ) // NOTAPPL
                                        csLine = _T("N");
                                else if( dValue > -MAXVALUE && !IsSpecial(dValue) ) // MAXVALUE
                                        csLine.Format( _T("%.*f"), iNumDec, dValue );
                                else
                                        csLine = _T("-");
                                iMaxColLen = std::max( iMaxColLen, csLine.GetLength() );

                                csLine = IntToString( j + 1 );
                                iMaxColLen = std::max( iMaxColLen, csLine.GetLength() );
                        }
                }
        }


        for( k=0; k < GetNumLayers(); k++ ) {
                csLine.Format( _T("%sLayer %d of %d\n"), (LPCTSTR)csTitle, k+1, GetNumLayers() );
                aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );

                if( bUseRowColNumbers ) {
                        int     iLineLen=0;
                        for( j=-1; j < GetNumCols(); j++ ) {

                                if( j == -1 )
                                        csLine.Format( _T("%*ls "), iMaxColLen, _T(" ") );
                                else
                                        csLine.Format( _T("%*d "), iMaxColLen, j + 1 );

                                if( j < GetNumCols() - 1 )
                                        csLine += _T("|");
                                aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );
                                iLineLen += csLine.GetLength();
                        }
                        csLine.Format( _T("\n") );
                        aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );

                        {
                                CString csLineAux(_T('-'), iLineLen );
                                //CString csLineAux2;

                                //csLineAux2.Format( _T("%*ls %ls"), iMaxColLen, _T(" "), csLineAux );

                                aLines.Add( csLineAux ); // cFile.Write( csLineAux, csLineAux.GetLength() );
                        }

                        csLine.Format( _T("\n") );
                        aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );
                }

                for(  i=0; i < GetNumRows(); i++ ) {
                        for( j=0; j < GetNumCols(); j++ ) {
                                double dValue=GetDoubleValue( i, j, k );
                                if( bUseRowColNumbers && j == 0 ) {
                                        if( dValue == MISSING ) // MISSING
                                                csLine.Format( _T("%*d |%*ls "), iMaxColLen, i + 1, iMaxColLen, _T("M") );
                                        else if( dValue == REFUSED ) // REFUSED
                                                csLine.Format( _T("%*d |%*ls "), iMaxColLen, i + 1, iMaxColLen, _T("R") );
                                        else if( dValue == DEFAULT ) // DEFAULT
                                                csLine.Format( _T("%*d |%*ls "), iMaxColLen, i + 1, iMaxColLen, _T("D") );
                                        else if( dValue == NOTAPPL ) // NOTAPPL
                                                csLine.Format( _T("%*d |%*ls "), iMaxColLen, i + 1, iMaxColLen, _T("N") );
                                        else if( dValue > -MAXVALUE && !IsSpecial(dValue) ) // MAXVALUE
                                                csLine.Format( _T("%*d |%*.*f "), iMaxColLen, i+1, iMaxColLen, iNumDec, dValue );
                                        else
                                                csLine.Format( _T("%*d |%*ls "), iMaxColLen, i + 1, iMaxColLen, _T("-") );
                                }
                                else {
                                        if( dValue == MISSING ) // MISSING
                                                csLine.Format( _T("%*ls "), iMaxColLen, _T("M") );
                                        else if( dValue == REFUSED ) // REFUSED
                                                csLine.Format( _T("%*ls "), iMaxColLen, _T("R") );
                                        else if( dValue == DEFAULT ) // DEFAULT
                                                csLine.Format( _T("%*ls "), iMaxColLen, _T("D") );
                                        else if( dValue == NOTAPPL ) // NOTAPPL
                                                csLine.Format( _T("%*ls "), iMaxColLen, _T("N") );
                                        else if( dValue > -MAXVALUE && !IsSpecial(dValue) ) // MAXVALUE
                                                csLine.Format( _T("%*.*f "), iMaxColLen, iNumDec, dValue );
                                        else
                                                csLine.Format( _T("%*ls"), iMaxColLen, _T("-") );

                                }
                                if( j < GetNumCols() - 1 )
                                        csLine += _T("|");

                                aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );
                        }

                        csLine.Format( _T("\n") );
                        aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );
                }

                csLine.Format( _T("\n") );
        aLines.Add( csLine ); // cFile.Write( csLine, csLine.GetLength() );
    }
}
// RHF END Jan 31, 2003
