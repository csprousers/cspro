#include "StdAfx.h"

#include "TbdSlice.h"
#include "BreakIt.h"
#include "defslot.h"


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

//SAVY 05/05/2004 no more old C libs .use C++ standard library
#include <iostream>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

// ----------------------------
// Method: Constructor
// ----------------------------
CTbdSlice::CTbdSlice( CTbdTable* pTable, bool bMustFreeTable ) {
    m_bMustFreeAcum = false;
    m_bMustFreeTable = bMustFreeTable;
    m_bClone = false;
    m_pTable = pTable;
    m_pAcum = NULL;
    m_lFilePos = -1;
    memset((csprochar*)&m_sHdr, 0, sizeof(m_sHdr));

    m_iBreakKeyLen = 0;
    m_csBreakKey = _T("");

    if( pTable != NULL )
        SetTable( pTable );

    m_cOtherInfo = 0;
    //BreakKeyLen  length is in number of characters. total length needs the number of bytes -Savy 01/31/2012 for unicode changes
    m_sHdr.iTotalLenght = sizeof(m_sHdr) + m_iBreakKeyLen*sizeof(TCHAR);

    if ( pTable != NULL ) {
        int iNumCells = 1;
        for ( int i = 0; i < m_pTable->GetNumDims(); i++ )
            iNumCells *= m_pTable->GetDimSize(i);

        m_sHdr.iTotalLenght += (iNumCells*m_pTable->GetCellSize());
    }
}

// ----------------------------
// Method: Constructor
// ----------------------------
CTbdSlice::CTbdSlice( CTbdSlice& rSlice ) {
    m_bClone = true;
    m_pTable = rSlice.m_pTable;

    m_bMustFreeAcum = false;
    m_bMustFreeTable = false;
    m_pAcum = rSlice.m_pAcum;                                               // REFERENCE TO THE SAME DATA
    m_lFilePos = -1;                                                            // The copy is load in memory no saved in disk
    m_lFilePos = rSlice.GetFilePos();                           // The copy is load in memory no saved in disk // RHF Aug 08, 2001

    memcpy((csprochar *)&m_sHdr, (csprochar *)&(rSlice.m_sHdr), sizeof(TBD_SLICE_HDR ));

    m_csBreakKey = rSlice.m_csBreakKey;
    m_iBreakKeyLen = rSlice.m_iBreakKeyLen;

    m_cOtherInfo = rSlice.m_cOtherInfo;
}

// ----------------------------
// Method: Destructor
// ----------------------------
CTbdSlice::~CTbdSlice() {
    if( !m_bClone ) {
        if( m_bMustFreeAcum && m_pAcum != NULL )
            delete m_pAcum;

        if( m_bMustFreeTable && m_pTable != NULL )
            delete m_pTable;
    }

}


void CTbdSlice::SetTable( CTbdTable* pTable ) {
    ASSERT(pTable);
    m_pTable = pTable;

    CBreakItem* pbItem;
    // The key will be:  "table num + break-values". Table num will be a short
    //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
    m_iBreakKeyLen = sizeof(short)/sizeof(TCHAR);
    for ( int i = 0; i < pTable->GetNumBreak(); i++ ) {
        pbItem = pTable->GetBreak(i);
        ASSERT(pbItem);
        m_iBreakKeyLen += pbItem->GetLen();
    }
    //SAVY 10/04/2005
    if( pTable->GetNumBreak() > 0){
        ASSERT(pTable->GetTbdFileBreakKeyLen() > 0);
        //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
        m_iBreakKeyLen = sizeof(short)/sizeof(TCHAR) + pTable->GetTbdFileBreakKeyLen(); //always full key length
    }


    //  DVB COM 20/08  m_iBreakKeyLen += 1;

    m_csBreakKey = _T("");

    //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
    m_sHdr.iTotalLenght = sizeof(TBD_SLICE_HDR) + m_iBreakKeyLen*sizeof(TCHAR);
    int iNumCells = 1;
    for ( int i = 0; i < m_pTable->GetNumDims(); i++ )
        iNumCells *= m_pTable->GetDimSize(i);

    m_sHdr.iTotalLenght += (iNumCells*m_pTable->GetCellSize());
}


// ----------------------------
// Method: Load ( iFH )
// ----------------------------
bool CTbdSlice::Load( int iFileHandler, long lFilePos ) {
    // ------------------------
    // If lFilePos == -1 then
    //  use the m_lFilePos like
    //  offset.
    // ------------------------
    if ( lFilePos != -1 )
        m_lFilePos = lFilePos;

    // ---------------------------------
    // Read the slice header information
    // ---------------------------------
    _lseek(iFileHandler, m_lFilePos, SEEK_SET);
    if ( !ReadBaseInfo(iFileHandler) )
        return false;

    if ( !ReadBreakInfo(iFileHandler) )
        return false;

    if( ReadInfo( iFileHandler ) == NULL )
        return false;

    return true;
}

// ----------------------------
// Method: Save
// ----------------------------
bool CTbdSlice::Save( int iFileHandler, long lFilePos ) {

    // Save at end
    if ( lFilePos == -1 )
        lFilePos = _lseek(iFileHandler, 0, SEEK_END);
    else
        _lseek(iFileHandler, lFilePos, SEEK_SET);

    if( lFilePos < 0 )
        return false;

    m_lFilePos = lFilePos;

    // Save the slice header information
    if ( !WriteBaseInfo(iFileHandler) )
        return false;

    if( !WriteInfo(iFileHandler) )
        return false;

    return true;
}

CString CTbdSlice::GetBreakKey( bool bFullKey ) {
    if( bFullKey ) {
        ASSERT( GetTable() );
        //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
        int     iKeyLen= sizeof(short)/sizeof(TCHAR)+ GetTable()->GetTbdFileBreakKeyLen();

        ASSERT( iKeyLen >= GetBreakKeyLen() );
        CString csBreakKey;

        csBreakKey.Format( _T("%-*ls"), iKeyLen, (LPCTSTR)m_csBreakKey );

        return csBreakKey;
    }
    else
        return m_csBreakKey;
}

int            CTbdSlice::GetBreakKeyLen() { return m_iBreakKeyLen; }
CTbdTable*     CTbdSlice::GetTable() { return m_pTable; }


//---------------------------------
// Method: GetNumDims
//---------------------------------
int CTbdSlice::GetNumDims() {
    return m_pTable->GetNumDims();
}

//---------------------------------
// Method: GetDimSize
//---------------------------------
int CTbdSlice::GetDimSize( int iDim ) {
    return m_pTable->GetDimSize(iDim);
}

//---------------------------------
// Method: GetDimSize
//---------------------------------
int CTbdSlice::GetDimSize( int* aIndex ) {
    return m_pTable->GetDimSize(aIndex);
}

//---------------------------------
// Method: GetDim
//---------------------------------
int CTbdSlice::GetDimSize( CArray<int,int>& rDim ) {
    return m_pTable->GetDimSize( rDim );
}

//---------------------------------
// Method: GetDim
//---------------------------------
long CTbdSlice::GetTotalLenght() {
    return m_sHdr.iTotalLenght;
}

//---------------------------------
// Method: SetSliceHdr
//---------------------------------
void CTbdSlice::SetSliceHdr(TBD_SLICE_HDR* tSHdr) {
    memcpy((csprochar *)&m_sHdr, (csprochar*)tSHdr, sizeof(TBD_SLICE_HDR));
}

//---------------------------------
// Method: SetAcum
//---------------------------------
void CTbdSlice::SetAcum(CTableAcum*     pAcum, bool bMustFreeAcum ) {
    m_pAcum = pAcum;
    m_bMustFreeAcum = bMustFreeAcum;
}

//---------------------------------
// Method: SetBreakKey
// SAVY modified 10/05/2005
// BreakKey length is the total length of the breakvars involved irrespective
// of whether the tbdtable is using all the breaksvars
//---------------------------------
void CTbdSlice::SetBreakKey( CString csBreakKey, int iBreakKeyLen) {
    // csprochar *pcBp;
    // csprochar *pcBm;

    //SAVY&&& why is the break key coming with junk characters during the calc
    //uncomment this assert to observe. uncomment the line once the problem is
    //fixed
    //ASSERT(csBreakKey.GetLength() == m_iBreakKeyLen);

    //Make sure that the break key length has not changed.
    ASSERT(iBreakKeyLen == m_iBreakKeyLen);

    m_csBreakKey = csBreakKey;//Set the new key.
    //SAVY commented the following code . It increases the breakkey length .
    // the length of the break key is set for the slice by the table.
    // this is to prevent modification of the key length .
    /*pcBp = csBreakKey.GetBufferSetLength(iBreakKeyLen + 1);
    pcBm = m_csBreakKey.GetBufferSetLength(iBreakKeyLen + 1);
    memcpy(pcBm, pcBp, iBreakKeyLen + 1);
    m_csBreakKey.ReleaseBuffer(iBreakKeyLen + 1);
    csBreakKey.ReleaseBuffer(iBreakKeyLen + 1);*/

    //This code is left alone from the old stuff . ideally we do not want
    //to change the key length . it has to come from tbdfile->tbdtable->tbdslice.
   //When the lowest level is totals i.e when the table has no breaks leave the  break key length as is Jan 23 2008
   if(this->GetTable()->GetNumBreak() != 0){
      m_iBreakKeyLen = iBreakKeyLen;
   }

   //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
    m_sHdr.iTotalLenght = sizeof(TBD_SLICE_HDR) + m_iBreakKeyLen*sizeof(TCHAR);

    int iNumCells = 1;
    for ( int i = 0; i < m_pTable->GetNumDims(); i++ )
        iNumCells *= m_pTable->GetDimSize(i);

    m_sHdr.iTotalLenght += (iNumCells*m_pTable->GetCellSize());
}

//---------------------------------
// Method: InDisk
//---------------------------------
bool CTbdSlice::InDisk() {
    if ( m_lFilePos != -1 )
        return true;

    return false;
}

//---------------------------------
// Method: GetFilePos
//---------------------------------
long CTbdSlice::GetFilePos() {
    return m_lFilePos;
}

//---------------------------------
// Method: SetFilePos
//---------------------------------
bool CTbdSlice::SetFilePos(long lFilePos) {
    m_lFilePos = lFilePos;
    return true;
}

//---------------------------------
// Method: ReadBaseInfo
//---------------------------------
bool CTbdSlice::ReadBaseInfo(int iFileHandler) {

    // Read the Slice Header
    ASSERT(iFileHandler>=0);

    int     iSizeHdr=sizeof(TBD_SLICE_HDR);
    int     iBytesRead = _read(iFileHandler, (csprochar *)&m_sHdr, iSizeHdr );

    if( iBytesRead < iSizeHdr )
        return false;

    return true;
}

//---------------------------------
// Method: ReadBaseInfo
//---------------------------------
bool CTbdSlice::ReadBreakInfo(int iFileHandler) {
    bool    bRet = true;

    // Read the break key value
    if ( m_iBreakKeyLen > 0 ) {
        //SAVY 10/05/2005 using a temp sKey so that m_csBreakKey length is not modified
        CString sKey = m_csBreakKey;
        csprochar*   pszKey=sKey.GetBufferSetLength( m_iBreakKeyLen + 1 );
        _tmemset( pszKey, 0,  m_iBreakKeyLen + 1 );

        //BreakKeyLen  length is in number of characters(TCHARs).here we need bytes.  -Savy 01/31/2012 for unicode changes
        if ( _read(iFileHandler, (csprochar *)pszKey, m_iBreakKeyLen*sizeof(TCHAR)) < m_iBreakKeyLen )
            bRet = false;

        sKey.ReleaseBuffer();
        m_csBreakKey = sKey;//Set the m_csBreakKey to the newly read breakkey
    }

    return bRet;
}

//---------------------------------------
// Method: ReadInfo
// Observation: Assume that m_cBufferRead
//                              was allocated by Load
//---------------------------------------
byte* CTbdSlice::ReadInfo(int iFileHandler ) {

    ASSERT(m_pAcum->GetAcumArea() != NULL );

    int     iSize=m_pAcum->GetNumCells() * m_pAcum->GetCellSize();
    int     iBytesRead = _read(iFileHandler, m_pAcum->GetAcumArea(), iSize );
    if( iBytesRead < iSize ) {
        ASSERT(0);
        return NULL;
    }

    return m_pAcum->GetAcumArea();
}

//---------------------------------
// Method: WriteBaseInfo
//---------------------------------
bool CTbdSlice::WriteBaseInfo(int iFileHandler) {

    ASSERT(iFileHandler>=0);

    int     iSizeHdr=sizeof(TBD_SLICE_HDR);

    int iBytesWrite=_write(iFileHandler, (csprochar *)&m_sHdr, iSizeHdr );

    if( iBytesWrite < iSizeHdr )
        return false;

    // Write the break key value
    if ( m_iBreakKeyLen > 0 ) {
        CString   csKey;

        csprochar*   pszKey=csKey.GetBufferSetLength( m_iBreakKeyLen + 1);
        _tmemset( pszKey, 0, m_iBreakKeyLen + 1 );

        int iLengthFromBreakKeyString = m_csBreakKey.GetLength();
        _tmemcpy( pszKey, (csprochar *)(LPCTSTR)m_csBreakKey,
            std::min( m_iBreakKeyLen, m_csBreakKey.GetLength() ) );

        iLengthFromBreakKeyString = m_csBreakKey.GetLength();
        //Remove once tested SAVY ...10/05/2005 make sure that the m_csBreakKey and the breakkeylen are same.TRACE for debug
        TRACE(_T("Writing Slice. BreakKeyLength is %d . m_iBreakKeyLen is %d\n") ,iLengthFromBreakKeyString ,m_iBreakKeyLen);
        //BreakKeyLen  length is in number of characters(TCHARs).here we need bytes.  -Savy 01/31/2012 for unicode changes
        iBytesWrite = _write(iFileHandler, pszKey, m_iBreakKeyLen*sizeof(TCHAR)); //Savy for unicode
        csKey.ReleaseBuffer();

        if( iBytesWrite < m_iBreakKeyLen )
            return false;
    }

    return true;
}

//---------------------------------------
// Method: WriteInfo
// Observation: Assume that m_cBufferRead
//                              was allocated by Load
//---------------------------------------
bool CTbdSlice::WriteInfo(int iFileHandler ) {
    ASSERT(m_pAcum->GetAcumArea());
    int     iSize;
    int     iBytesWrite;

    iSize = m_pAcum->GetNumCells() * m_pAcum->GetCellSize();
    iBytesWrite = _write(iFileHandler, m_pAcum->GetAcumArea(), iSize );

    if( iBytesWrite < iSize ) {
        ASSERT(0);
        return false;
    }

    // Recalculate iTotalLenght
    //BreakKeyLen  length is in number of characters(TCHARs).  -Savy 01/31/2012 for unicode changes
    m_sHdr.iTotalLenght = sizeof(TBD_SLICE_HDR) + m_iBreakKeyLen*sizeof(TCHAR);

    m_sHdr.iTotalLenght += (m_pAcum->GetNumCells()*(m_pAcum->GetCellSize()));

    return true;
}
