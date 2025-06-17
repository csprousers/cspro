#pragma once

//---------------------------------------------------------------------------
//  File name: TbdSlice.h
//
//  Description:
//          Header for CTbdSlice class
//          This class is intended define a tbd slice structure
//
//  History:    Date       Author   Comment
//              ---------------------------
//              6 Jul 01   DVB      Created
//
//---------------------------------------------------------------------------
#include <ZTBDO/zTbdO.h>
#include <ZTBDO/TbdTable.h>
#include <ZTBDO/TableAc.h>

// ---------------------------
// Slice file header structure
// ---------------------------
#pragma pack( push, TbdSlice_include ) // rcl, Jun 18 2004
#pragma pack(1)
typedef struct {
    long    iTotalLenght;
    short   iTableNumber;   // 0 based
    csprochar    cSliceStatus;
    csprochar    cOtherInfo;
} TBD_SLICE_HDR;
#pragma pack( pop, TbdSlice_include ) // rcl, Jun 18 2004

#define  TBD_SLICE_CELLNUM 100

class CLASS_DECL_ZTBDO CTbdSlice {  // Manipulate I/O of a slice
private:
    bool                    m_bClone;
    bool                    m_bMustFreeAcum;
    bool                    m_bMustFreeTable;
    CTbdTable*              m_pTable;
    CTableAcum*             m_pAcum; // m_pAcum can be free by a table only.
                                     // Assume that every pointer is unique (if not FreeAcum could have problems)

    // File pos in tbd TBD. -1 if the CSlice is in memory only.
    long                    m_lFilePos;
    TBD_SLICE_HDR           m_sHdr;

    // Slice Break Key value
    CString                 m_csBreakKey;
    int                     m_iBreakKeyLen;

    csprochar                   m_cOtherInfo;

public:
    CTbdSlice( CTbdTable* pTable, bool bMustFreeTable=false );
    CTbdSlice( CTbdSlice& rSlice ); // Copy Constructor
    virtual ~CTbdSlice();

    // I/O from the TBD file
    bool Load( int iFileHandler, long lFilePos );
    bool Save( int iFileHandler, long lFilePos ); // Must refresh the trailer




    CString         GetBreakKey( bool bFullKey=true );
    int             GetBreakKeyLen();
    CTbdTable*      GetTable();
    void            SetTable( CTbdTable* pTable ); // Call only if constructor was called with NULL

    int             GetNumDims();
    int             GetDimSize( int iDim );
    int             GetDimSize( int* aIndex );
    int             GetDimSize( CArray<int,int>& rDim );
    long            GetTotalLenght();
    bool            InDisk();
    bool            SetFilePos(long lFilePos);
    long            GetFilePos();
    TBD_SLICE_HDR*  GetSliceHdr() { return &m_sHdr; };

    CTableAcum*     GetAcum() { return m_pAcum; };

    // When we create a slice from scrash
    void            SetSliceHdr(TBD_SLICE_HDR* tSHdr);
    void            SetAcum(CTableAcum* pAcum, bool bMustFreeAcum=false );
    void            SetBreakKey( CString csBreakKey, int iBreakKeyLen);

    bool            IsClone() { return m_bClone; }

    bool            ReadBaseInfo(int iFileHandler);
    bool            ReadBreakInfo(int iFileHandler);
private:

    byte*           ReadInfo(int iFileHandler );

    bool            WriteBaseInfo(int iFileHandler);
    bool            WriteInfo(int iFileHandler );
};
