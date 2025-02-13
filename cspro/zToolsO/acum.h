#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO CAcum
{
public:
    CAcum();
    CAcum( CAcum& rAcum );
    ~CAcum();

    void    SetDefaultValue( byte* pDefaultValue );
    byte*   GetDefaultValue();

    // Alloc and UseAre are excluyents.
    bool    ShareArea( CAcum& rAcum );
    byte*   Alloc( int uCellSize, int uNumRows, int uNumCols=1, int uNumLayers=1, byte* pDefaultValue=NULL );

    // Only free if Alloc was used
    void    Free();

    bool    Fill( byte* pFillValue );

    double  GetDoubleValue( int iRow, int iCol=0, int iLayer=0 );

    bool    PutDoubleValue( double dValue, int iRow, int iCol=0, int iLayer=0 );

    byte*   GetValue( int iRow, int iCol=0, int iLayer=0 );

    bool    PutValue( byte* pValue, int iRow, int iCol=0, int iLayer=0 );

    byte*   GetAcumArea();
    int     GetCellSize();
    int     GetNumDim();
    int     GetNumRows();
    int     GetNumCols();
    int     GetNumLayers();
    int     GetNumCells();

    bool    AreSimilar( CAcum* pAcum );

    int     GetOffSetPos( int iRow, int  iCol/*=0*/, int iLayer/*=0*/ ); // RHF Jul 29, 2002


    void    Dump( CString csFileName, CString csTitle, int iNumDec, bool bUseRowColNumbers ); // RHF Sep 25, 2002
    void    Dump( CStringArray& aLines, CString csTitle, int iNumDec, bool bUseRowColNumbers );

private:
    void Init();

    // uRow,iCol, iLayer are 0 based
    byte *GetOffSet( int iRow, int  iCol=0, int iLayer=0 );

private:
    byte*   m_pAcumArea;

    int     m_iCellSize;
    int     m_iNumDim;
    int     m_iNumRows;
    int     m_iNumCols;
    int     m_iNumLayers;
    int     m_iNumCells;
    byte*   m_pDefaultValue;
    bool    m_bMustFree;
};
