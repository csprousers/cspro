#pragma once

class CBorder
{
public:
    CBorder();
    ~CBorder();

    void    SetDims( int iNumRows, int iNumCols, int iNumLayers );

    bool    Alloc( double* adValues );
    void    CleanArrays( bool bCleanValues );
    bool    AddValue( double dValue, int iRow, int iCol, int iLayer ); // Add to the border
    int     GetCellNum( int iRow, int iCol, int iLayer );
    double  GetValueAt( int iCellNum );

    int     GetNumRows() const { return m_iNumRows; }
    int     GetNumCols() const { return m_iNumCols; }
    int     GetNumLayers() const { return m_iNumLayers; }

    int     GetNumCells() const { return m_iNumCells; }; // Is not the same as Numrows*NumCols*NumLayers. It's the border cells

private:
    bool*       m_abUsed;
    double*     m_adValue;
    int         m_iNumRows;
    int         m_iNumCols;
    int         m_iNumLayers;
    int         m_iNumCells;

    bool        m_bExternalValues; // True when m_adValue is an external area
};



inline CBorder::CBorder()
{
    m_abUsed = NULL;
    m_adValue = NULL;
    m_iNumRows = 0;
    m_iNumCols = 0;
    m_iNumLayers = 0;
    m_iNumCells = 0;
    m_bExternalValues = false;
}


inline CBorder::~CBorder()
{
    if( m_abUsed != NULL )
        free( m_abUsed );
    m_abUsed = NULL;

    if( !m_bExternalValues && m_adValue != NULL )
        free( m_adValue );
    m_adValue = NULL;
}


inline void CBorder::SetDims( int iNumRows, int iNumCols, int iNumLayers )
{
    ASSERT( iNumRows >= 1 );
    ASSERT( iNumCols >= 1 );
    ASSERT( iNumLayers >= 1 );
    m_iNumRows = iNumRows;
    m_iNumCols = iNumCols;
    m_iNumLayers = iNumLayers;

    ASSERT( m_adValue == NULL  );
    ASSERT( m_abUsed == NULL );

    // Rows+Columns+Border * NumLayer + Total Layer Cells
    m_iNumCells=(iNumRows+iNumCols+1)*iNumLayers + (iNumRows+1)*(iNumCols+1);
}


// iNumRows,iNumCols, iNumLayers are the dimension of the "normal" cube (not the expanded)
inline bool CBorder::Alloc(double* adValues)
{
    ASSERT( m_iNumCells >= 1 );

    if( adValues == NULL ) {
        m_adValue = (double*) calloc( m_iNumCells, sizeof(double) );
        m_bExternalValues = false;
    }
    else {
        m_adValue = adValues;
        m_bExternalValues = true;
    }

    m_abUsed = (bool*) calloc( m_iNumCells, sizeof(bool) );

    if( m_adValue == NULL || m_abUsed == NULL )
        return false;

    CleanArrays(true);

    return true;
}


inline void CBorder::CleanArrays(bool bCleanValues)
{
    ASSERT( m_adValue != NULL );
    ASSERT( m_abUsed != NULL );

    double*     pValue;
    bool*       pUsed;
    for(int i=0; i < m_iNumCells; i++ ) {
        if( bCleanValues ) {
            pValue = ( m_adValue + i );
            *pValue = (double) 0;
        }

        pUsed = ( m_abUsed + i );
        *pUsed = false;
    }
}


inline int CBorder::GetCellNum( int iRow, int iCol, int iLayer )
{
    int     iCellNum=-1;

    // Check dimensions
    if( iRow < 0 || iRow > m_iNumRows || iCol < 0 || iCol > m_iNumCols ||
        iLayer < 0 || iLayer > m_iNumLayers )
        ASSERT(0);
    // At least one dimension must be in the border
    else if( iRow < m_iNumRows && iCol < m_iNumCols && iLayer < m_iNumLayers )
        ASSERT(0);
    else { // Everything OK
        // The total layer
        if( iLayer == m_iNumLayers ) { // Last layer has (Columns+1)*(Rows+1) cells
            iCellNum = (m_iNumRows+m_iNumCols+1)*iLayer;
            iCellNum += iCol*(m_iNumRows+1)+iRow;
        }
        else if( iCol == m_iNumCols ) {
            iCellNum = (m_iNumRows+m_iNumCols+1)*iLayer;
            iCellNum += iRow;
        }
        else if( iRow == m_iNumRows ) {
            iCellNum = (m_iNumRows+m_iNumCols+1)*iLayer;
            iCellNum += m_iNumRows;
            iCellNum += (m_iNumCols - iCol);
        }
        else
            ASSERT(0);


        ASSERT( iCellNum >= 0 && iCellNum < m_iNumCells );
    }

    return iCellNum;
}


// zero base indexes
inline bool CBorder::AddValue( double dValue, int iRow, int iCol, int iLayer )
{
    bool    bDone=false;

    int iCellNum = GetCellNum( iRow, iCol, iLayer );

    if( iCellNum >= 0 && iCellNum < m_iNumCells ) {
        ASSERT( m_adValue != NULL );
        ASSERT( m_abUsed != NULL );

        bool*       pUsed = ( m_abUsed + iCellNum );
        double*     pValue = ( m_adValue + iCellNum );

        if( !(*pUsed) ) {
            *pUsed = true;
            (*pValue) += dValue;
            bDone = true;
        }
    }

    return bDone;
}


inline double CBorder::GetValueAt( int iCellNum )
{
    ASSERT( iCellNum >= 0 && iCellNum < m_iNumCells );
    ASSERT( m_adValue );
    double*       pValue = ( m_adValue + iCellNum );
    return *pValue;
}
