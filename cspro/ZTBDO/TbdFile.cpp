#include "StdAfx.h"
#include "TbdFile.h"
#include <zToolsO/Special.h>
#include <zToolsO/Tools.h>
#include <zUtilO/Interapp.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//SAVY 05/05/2004 no more old C libs .use C++ standard library
#include <iostream>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


// --------------------------------
// Method: Constructor
// --------------------------------
CTbdFile::CTbdFile(CString csFileName, char cVersion) {
    Init();

    m_csFileName = csFileName;
    m_cVersion = cVersion;
}

void CTbdFile::Init() {
    m_csFileName.Empty();
    m_iFd = -1;

    for( int i=0; i < m_aBreakItem.GetSize(); i++ )
        delete m_aBreakItem.ElementAt(i);
    m_aBreakItem.RemoveAll();

    for( int i=0; i < m_aTbdTable.GetSize(); i++ )
        delete m_aTbdTable.ElementAt(i);
    m_aTbdTable.RemoveAll();

    for( int i=0; i < m_aTbdSlice.GetSize(); i++ )
        delete m_aTbdSlice.ElementAt(i);
    m_aTbdSlice.RemoveAll();

    m_iBreakLen  = 0;
    m_lTrlOffset = 0;
    m_iMaxNameLen = 0;
    m_cVersion = 0x01;
    m_iFileSize = -1;
}



// --------------------------------
// Method: Destructor
// --------------------------------
CTbdFile::~CTbdFile() {
    Close();// RHF Aug 08, 2001
    Init();
}

//---------------------------
// Method: GetNumTables
//---------------------------
int CTbdFile::GetNumTables() {
    return m_aTbdTable.GetSize();
}

//---------------------------
// Method: GetTables
//---------------------------
CTbdTable* CTbdFile::GetTable(int iTable) {

    // RHF COM Aug 07, 2001         return new CTbdTable(*m_aTbdTable.GetAt(iTable));
    return m_aTbdTable.ElementAt(iTable);
}

//---------------------------
// Method: GetTables
//---------------------------
CTbdTable* CTbdFile::GetTable(csprochar *pszTableName) {
    CTbdTable* pTable;

    for (int i = 0; i < m_aTbdTable.GetSize(); i++) {
        pTable = m_aTbdTable.GetAt(i);
        if ( !pTable->GetTableName().Compare(pszTableName) ) {
            // RHF COM Aug 07, 2001 return new CTbdTable(*pTable);
            return pTable;
        }
    }

    return NULL;
}


//---------------------------
// Method: DoOpen
//---------------------------
// RHF INIC Oct 21, 2002
void CTbdFile::SetFileName( CString csFileName ) {
    m_csFileName = csFileName;
}
// RHF END Oct 21, 2002


bool CTbdFile::DoOpen(bool bCreate ) {
    int   iFlag;
    int   iMode;

    m_lTrlOffset = 0;
    if ( bCreate ) {
        if( PortableFunctions::FileExists(m_csFileName) ) {
            try {
                CFile::Remove(m_csFileName);
            } catch ( ...) {
            }
        }
        iFlag = _O_BINARY | _O_CREAT | _O_RDWR;
        iMode = _S_IREAD | _S_IWRITE;
    }
    else {
        iFlag = _O_BINARY | _O_RDWR;
        iMode = 0x00;
    }

    m_iFd = _topen(m_csFileName, iFlag, iMode);
    if ( m_iFd == -1 )
        return false;

    if (!bCreate) {
        m_iFileSize = _lseek(m_iFd, 0, SEEK_END);
        _lseek(m_iFd, 0, SEEK_SET);
    }

    return true;
}
//SAVY 10/05/2005 for setting full breakkey in slices
void CTbdFile::SetBreakKeyLen(int iBreakKeylen){
    m_iBreakLen = iBreakKeylen;
}

// --------------------------------
// Method: Open
// --------------------------------
bool CTbdFile::Open(bool bCreate) {
    long  lBOffset;
    long  lEOffset;
    long  lRealEOffset;
    short sNumTables;
    char  cNumTbdBreaks;
    byte  cBuffer[1024];
    csprochar  csNameBI[TBD_BI_NAMELEN + 1];
    csprochar  csNameTT[TBD_TT_NAMELEN + 1];
    int   iBackFd;
    int   iRc;
    int   iBreakLen;
    int       iValue;
    TBD_BI      tBI;
    TBD_TT      tTT;
    CString                 csFile;
    CBreakItem      *pbItem;
    CArray<int,int> aDim;


    // --------------------------------------
    // First i create or open the file
    // --------------------------------------
    if( !DoOpen( bCreate ) )
        return false;


    // --------------------------------------
    // if we create a new tbd, we must build
    // and empty Tbd file
    // --------------------------------------
    if ( bCreate ) {
        EmptyTbd();
    }

    _lseek(m_iFd, -5L, SEEK_END);                           // EndOffset + Version (5 bytes)
    lRealEOffset = _tell(m_iFd);
    _read(m_iFd, (char *)&lBOffset, sizeof(long) );
    _read(m_iFd, (char *)&m_cVersion, sizeof(char) );
    _lseek(m_iFd, lBOffset, SEEK_SET);

    // --------------------------------------
    // Build a trailer backup over <file>.bak
    // --------------------------------------
    csFile = m_csFileName.Left(_tcslen(m_csFileName)) + _T(".bak");
    iBackFd = _topen(csFile, _O_BINARY | _O_CREAT | _O_WRONLY, _S_IREAD| _S_IWRITE);
    if ( iBackFd == -1 )
        return false;
    while ( (iRc = _read(m_iFd, cBuffer, 1024)) > 0 )
        write( iBackFd, cBuffer, iRc );
    _close(iBackFd);


    // --------------------------------------
    // Check the trailer integrity
    // --------------------------------------
    _lseek(m_iFd, lBOffset, SEEK_SET);
    _read(m_iFd, (char *)&lEOffset,      sizeof(long));
    _read(m_iFd, (char *)&sNumTables,    sizeof(short));
    _read(m_iFd, (char *)&cNumTbdBreaks, sizeof(char));

    // --------------------------------------
    // If the real end offset defire from the
    // read offset then ckeck consistence is
    // false
    // --------------------------------------
    if (lEOffset != lRealEOffset) {
        Close();
        return false;
    }

    // ------------------------
    // Set the trailer offset
    // ------------------------
    m_lTrlOffset = lBOffset;

    // --------------------------------------
    // Read the breaks info
    // --------------------------------------
    iBreakLen = 0;
    for ( char i = 0; i < cNumTbdBreaks; i++ ) {
        _read(m_iFd, (void *)&tBI, sizeof(TBD_BI));
        _tmemcpy(csNameBI, tBI.cName, TBD_BI_NAMELEN);
        //csNameBI[TBD_BI_NAMELEN] = 0x00;
        csNameBI[TBD_BI_NAMELEN] = _T('\0'); //Savy for Unicode
        CString cName(csNameBI);

        pbItem = new CBreakItem(cName, (int)tBI.cItemLen, (csprochar)tBI.cOtherInfo);

        iBreakLen += pbItem->GetLen();
        m_aBreakItem.Add(pbItem);
    }
    m_iBreakLen = iBreakLen;

    // --------------------------------------
    // Read the tables info
    // --------------------------------------
    m_iMaxNameLen = 0;
    for ( int k = 0; k < sNumTables; k++ ) {
        _read(m_iFd, (void *)&tTT, sizeof(TBD_TT));

        // Table Name
        _tmemcpy(csNameTT, tTT.cTableName, TBD_TT_NAMELEN);
        //csNameTT[TBD_TT_NAMELEN] = 0x00;
        csNameTT[TBD_TT_NAMELEN] = _T('\0'); //Savy for unicode
        CString cName(csNameTT);

        // Next Table Name
        _tmemcpy(csNameTT, tTT.cNextTableName, TBD_TT_NAMELEN);
        //csNameTT[TBD_TT_NAMELEN] = 0x00;
        csNameTT[TBD_TT_NAMELEN] = _T('\0'); //Savy for unicode
        CString cNextName(csNameTT);


        // Get the mayor table name size
        cName.TrimRight(' ');
        if ( m_iMaxNameLen < (int)_tcslen(cName) )
            m_iMaxNameLen = _tcslen(cName);

        aDim.RemoveAll();
        for ( int j = 0; j < tTT.cNumDim; j++ ) {
            _read(m_iFd, (void *)&iValue, sizeof(int));
            aDim.Add(iValue);
        }

        CTbdTable   *ptTable;

        ptTable = new CTbdTable(cName, cNextName, aDim, (CTableDef::ETableType) tTT.cType, (int)tTT.lCellSize, (csprochar)tTT.cOtherInfo, m_iBreakLen );

        int     iNumBreaks=m_aBreakItem.GetSize();// RHF Apr 17, 2003

        iNumBreaks = tTT.cNumTableBreaks; // RHF Apr 17, 2003

        // RHF COM Apr 17, 2003 for ( j = 0; j < m_aBreakItem.GetSize()/*&& j < tTT.cNumTableBreaks*/; j++ )
        for ( int j = 0; j < iNumBreaks; j++ ) // RHF Apr 17, 2003
            ptTable->AddBreak(m_aBreakItem.GetAt(j));

        m_aTbdTable.Add(ptTable);


        ptTable->SetDimSize(aDim);
    }

    return true;
}

int CTbdFile::GetFd() {
    return m_iFd;
}

// --------------------------------
// Method: Close
// --------------------------------
bool CTbdFile::Close() {
    CString csFile;

    csFile = m_csFileName + _T(".bak");

    if( PortableFunctions::FileExists(csFile) ) {
        try {
            CFile::Remove(csFile);
        }
        catch(...) {
        }
    }

    // RHF INIC Aug 08, 2001
    if( m_iFd >= 0 )
        _close( m_iFd );
    m_iFd = -1;
    // RHF END Aug 08, 2001


    return true;
}

// --------------------------------
// Method: Flush
// --------------------------------
bool CTbdFile::Flush() {
    return true;
}


// --------------------------------
// Method: EmptyTbd
// --------------------------------
void CTbdFile::EmptyTbd() {
    long    lValue;
    short   sValue;
    char    cValue;


    _lseek(m_iFd, 0L, SEEK_SET);
    lValue = 7;
    write(m_iFd, (char *)&lValue, sizeof(long));
    sValue = 0;
    write(m_iFd, (char *)&sValue, sizeof(short));
    cValue = 0;
    write(m_iFd, (char *)&cValue, sizeof(char));
    lValue = 0;
    write(m_iFd, (char *)&lValue, sizeof(long));
    write(m_iFd, &m_cVersion, sizeof(char));
}

//-----------------------------
// Method: GetFileName
//-----------------------------
CString CTbdFile::GetFileName() {
    return m_csFileName;
}

//-----------------------------
// Method: GetBreak
//-----------------------------
CBreakItem* CTbdFile::GetBreak(int iIndex) {
    return m_aBreakItem.GetAt(iIndex);
}

// --------------------------------
// Method: GetNextSlice()
// --------------------------------
CTbdSlice* CTbdFile::GetNextSlice( long& lFilePos, bool bForUpdate ) {
    CTbdSlice*      ptSlice;
    CTbdTable*  ptTable;

    // RHF INIC Aug 08, 2001
    if( lFilePos >= m_lTrlOffset )
        return NULL;

    ptSlice = new CTbdSlice((CTbdTable*) NULL); // Still I don't know the table
    ASSERT(ptSlice);

    _lseek(m_iFd, lFilePos, SEEK_SET);
    if( !ptSlice->ReadBaseInfo(m_iFd) ){
        SAFE_DELETE(ptSlice);
        return NULL;
    }

    int  iTableNumber= ptSlice->GetSliceHdr()->iTableNumber;


    ptTable = GetTable( iTableNumber );
    ptSlice->SetTable( ptTable ); // Now I know the table

    if( !ptSlice->ReadBreakInfo(m_iFd) ){
        SAFE_DELETE(ptSlice);
        return NULL;
    }

    // Alloc acum
    CTableAcum* pAcum=new CTableAcum( ptTable );
    pAcum->Alloc();

    bool    bMustFree=true;
    ptSlice->SetAcum( pAcum, bMustFree );

    // Load
    if (ptSlice->Load(m_iFd, lFilePos)) {
        lFilePos += ptSlice->GetTotalLenght(); // Leave ready for the next slice
        if ( bForUpdate )
            AddSlice(ptSlice, true );
        return ptSlice;
    }
    else{
        SAFE_DELETE(ptSlice);
        return NULL;
    }
    // RHF END Aug 08, 2001

    return NULL;
}


// --------------------------------
// Method: AddSlice
// --------------------------------
bool CTbdFile::AddSlice(CTbdSlice* ptSlice, bool bCreateTable ) {
    bool        bFound;
    CTbdTable  *ptTable;

    // ----------------------------------
    // The m_pAcum is the same,
    // but m_lFilePos is -1 for the copy
    // ----------------------------------
    ASSERT(ptSlice);
    m_aTbdSlice.Add(ptSlice);

    // ----------------------------------
    // Is new the table definition
    // ----------------------------------
    bFound = false;
    ptTable = ptSlice->GetTable();

    for ( int i = 0; i < m_aTbdTable.GetSize(); i++ ) {
        if ( m_aTbdTable.GetAt(i)->GetTableName() == ptTable->GetTableName() ) {
            bFound = true;
            break;
        }
    }
    if (!bFound) {
        if( bCreateTable )
            m_aTbdTable.Add(new CTbdTable(*ptTable)); //  Build a copy of table object to be delete in tbdfile destructor
        else
            m_aTbdTable.Add(ptTable);

    }

    // -----------------------------------
    // Breaks
    // 1) Verify that have the same breaks
    // 2) Add at the end some new break
    // -----------------------------------
    for ( int i = 0; i < ptTable->GetNumBreak(); i++ ) {
        if ( i < m_aBreakItem.GetSize() ) {
            if ( m_aBreakItem.GetAt(i)->GetName() != ptTable->GetBreak(i)->GetName() ) {
                return false;
            }
        }
        else {
            if( bCreateTable )
                m_aBreakItem.Add(new CBreakItem(*ptTable->GetBreak(i))); //  Build a copy of table object to be delete in tbdfile destructor
            else
                m_aBreakItem.Add(ptTable->GetBreak(i));

        }
    }

    return true;
}

// --------------------------------
// Method: AddBreak
//                 The ~TbdFile method
//         delete the bItem instance
// --------------------------------
bool CTbdFile::AddBreak(CBreakItem* bItem) {
    for ( int i = 0; i < m_aBreakItem.GetSize(); i++) {
        if ( m_aBreakItem.GetAt(i)->GetName() == bItem->GetName() )
            return true;
    }

    m_aBreakItem.Add(bItem);

    return true;
}

// --------------------------------
// Method: AddBreak
//                 The ~TbdFile method
//         delete the pTable instance
// --------------------------------
bool CTbdFile::AddTable(CTbdTable* pTable) {
    for ( int i = 0; i < m_aTbdTable.GetSize(); i++) {
        if ( m_aTbdTable.GetAt(i)->GetTableName() == pTable->GetTableName() )
            return false; // RHF Oct 25, 2001 change from true to false
    }

    m_aTbdTable.Add(pTable);

    return true;
}

// --------------------------------
// Method: GetTablePos
// --------------------------------
int CTbdFile::GetTablePos(CString csTableName) {
        for ( int i = 0; i < m_aTbdTable.GetSize(); i ++ ) {
                if ( !csTableName.Compare(m_aTbdTable.GetAt(i)->GetTableName()) )
                        return i;
        }

        return -1; // Table name not found
}


// --------------------------------
// Method: WriteAllSlices
// --------------------------------
bool CTbdFile::WriteAllSlices(bool bFreeData) {
    CTbdSlice*      ptSlice;
    bool            bRet=true;

    // First write the slice, inside the save method is refreshed the index info
    for ( int i = 0; i < m_aTbdSlice.GetSize(); i++) {
        ptSlice = m_aTbdSlice.GetAt(i);

        if ( ptSlice == NULL ) {
            ASSERT(ptSlice);
            continue;
        }

        // If the slice is allocate in disk
        if (ptSlice->InDisk()) {
            // RHF COM Aug 09, 2001 if ( !ptSlice->Save(m_iFd, -1) )
            if ( !ptSlice->Save(m_iFd, ptSlice->GetFilePos() ) ) { // RHF Aug 09, 2001
                bRet = false;
                break;
            }
        }
        else {
            // Found the new position of the table information
            int j;
            for ( j = 0; j < m_aTbdTable.GetSize(); j++ ) {
                if ( m_aTbdTable.GetAt(j)->GetTableName() == ptSlice->GetTable()->GetTableName() )
                    break;
            }

            ptSlice->GetSliceHdr()->iTableNumber = j;

            // RHF COM Aug 09, 2001 if( !ptSlice->Save(m_iFd, m_lTrlOffset) )
            if ( !ptSlice->Save(m_iFd, -1 ) ) { // Save at end
                bRet = false;
                break;
            }

            m_lTrlOffset += ptSlice->GetTotalLenght();
        }
    }

    if ( bFreeData ) {
        // RHF INIC Mar 12, 2003
        for ( int i = 0; i < m_aTbdSlice.GetSize(); i++) {
            ptSlice = m_aTbdSlice.GetAt(i);
            if( ptSlice != NULL ) delete ptSlice;
        }
        // RHF END Mar 12, 2003

        m_aTbdSlice.RemoveAll();
    }

    return bRet;
}

// --------------------------------
// Method: WriteTrailer
// --------------------------------
bool CTbdFile::WriteTrailer() {
    long    lEOffset;
    long    lValue;
    short   sValue;
    char    cValue;
    TBD_TT  tTT;
    TBD_BI  tBI;
    int             iBackFd;
    int     iRc;
    CString csFile;
    byte    cBuffer[1024];

    // RHF INIC Aug 09, 2001 Always the trailer is at the end
    m_lTrlOffset = _lseek(m_iFd, 0, SEEK_END);
    if( m_lTrlOffset < 0 )
        return false;
    // RHF END Aug 09, 2001

    // -------------------------
    // Go to the offset trailer
    // -------------------------
    // RHF COM Aug 09, 2001 _lseek(m_iFd, m_lTrlOffset, SEEK_SET);

    // -------------------------
    // Trailer Info
    // -------------------------
    lEOffset  = m_lTrlOffset;                                                               // All slice data
    lEOffset += sizeof(long);                                                               // End offset it self
    lEOffset += sizeof(short);                                                              // Number of tables
    lEOffset += sizeof(char);                                                               // Number of breaks
    lEOffset += (sizeof(TBD_BI)*m_aBreakItem.GetSize());    // Number the break * TBD_BI size
    lEOffset += (sizeof(TBD_TT)*m_aTbdTable.GetSize());             // Number the table * TBD_TT size
    for ( int i = 0; i < m_aTbdTable.GetSize(); i++ )
        lEOffset += (m_aTbdTable.GetAt(i)->GetNumDims() * sizeof(long));

    write(m_iFd, (char *)&lEOffset,  sizeof(long));                         // End offset;

    sValue = (short)m_aTbdTable.GetSize();
    write(m_iFd,  (char *)&sValue, sizeof(short));                  // Tables number

    cValue = (char)m_aBreakItem.GetSize();
    write(m_iFd,  (char *)&cValue, sizeof(char));                   // Breaks number

    CString     csBreakName;
    for (int i = 0; i < m_aBreakItem.GetSize(); i++ ) {                 // Break info
        memset( &tBI, 0, sizeof(TBD_BI) );

        tBI.cItemLen = m_aBreakItem.GetAt(i)->GetLen();
        tBI.cOtherInfo = 0;

        csBreakName = m_aBreakItem.GetAt(i)->GetName();

        memcpy(tBI.cName, csBreakName, std::min( (unsigned int) TBD_BI_NAMELEN, csBreakName.GetLength() * sizeof(TCHAR) ) );
        write(m_iFd, (void *)&tBI, sizeof(TBD_BI));
    }

    CString     csTableName;
    CString     csNextTableName;
    for (int i = 0; i < m_aTbdTable.GetSize(); i++ ) {                  // Table info
        memset( &tTT, 0, sizeof(TBD_TT) );

        tTT.cNumDim = m_aTbdTable.GetAt(i)->GetNumDims();
        tTT.cNumTableBreaks = m_aTbdTable.GetAt(i)->GetNumBreak();
        tTT.cOtherInfo = 0;

        csTableName = m_aTbdTable.GetAt(i)->GetTableName();
        csNextTableName = m_aTbdTable.GetAt(i)->GetNextTableName();


        _tmemcpy(tTT.cTableName, csTableName, std::min( TBD_TT_NAMELEN, csTableName.GetLength() ) );
        _tmemcpy(tTT.cNextTableName, csNextTableName, std::min( TBD_TT_NAMELEN, csNextTableName.GetLength() ) );

        tTT.cType = m_aTbdTable.GetAt(i)->GetTableType();
        tTT.lCellSize = m_aTbdTable.GetAt(i)->GetCellSize();
        write(m_iFd, (void *)&tTT, sizeof(TBD_TT));
        for (int j = 0; j < m_aTbdTable.GetAt(i)->GetNumDims(); j++ ) {
            lValue = m_aTbdTable.GetAt(i)->GetDimSize(j);
            write(m_iFd, (char*)&lValue, sizeof(long));
        }
    }

#ifdef _DEBUG
    long    lFilePosInic=_tell(m_iFd);
#endif
    write(m_iFd, (char *)&m_lTrlOffset, sizeof(long));              // Begin trailer offset
    write(m_iFd, &m_cVersion, sizeof(char) );                                                       // Version

    // --------------------------------------
    // Build a trailer backup over <file>.bak
    // --------------------------------------
    _lseek(m_iFd, m_lTrlOffset, SEEK_SET);
    csFile = m_csFileName.Left(_tcslen(m_csFileName)) + _T(".bak");
    iBackFd = _topen(csFile, _O_BINARY | _O_CREAT | _O_WRONLY, _S_IREAD| _S_IWRITE);
    if ( iBackFd == -1 )
        return false;
    while ( (iRc = _read(m_iFd, cBuffer, 1024)) > 0 )
        write( iBackFd, cBuffer, iRc );
    _close(iBackFd);

    return true;
}

// RHF INIC Oct 24, 2001
static CString alpha_value( double dValue ) {
    CString csValue;
    bool    bNeg=false;

    if( dValue <= -MAXVALUE ) {
        bNeg = true;
        dValue = -dValue;
    }

   if( IsSpecial(dValue) ) {
        if( dValue == MISSING )
            csValue.Format( _T("%lcMIS"), bNeg ? _T('-') : ' ' );
        else if( dValue == REFUSED )
            csValue.Format( _T("%lcREF"), bNeg ? _T('-') : ' ' );
        else if( dValue == DEFAULT )
            csValue.Format( _T("%lcDEF"), bNeg ? _T('-') : ' ' );
        else if( dValue == NOTAPPL )
            csValue.Format( _T("%lcNAP"), bNeg ? _T('-') : ' ' );
        else
            csValue.Format( _T("%lc***"), bNeg ? _T('-') : ' ' );
    }
    else
        csValue.Format( _T("%8.2f"), dValue );

    return csValue;
}
// RHF END Oct 24, 2001

// To make below code VC++ 6.0 and 7.0 fully compatible
#ifndef _CSTR_
 #define _CSTR_ (csprochar*) (LPCTSTR)
#endif

// --------------------------------
// Method: ShowInfo
// --------------------------------
void CTbdFile::ShowInfo() {
    CBreakItem cbItem;
    CTableAcum* pAcum;
    CTbdSlice* pSlice;
    CTbdTable* pTable;
    double     dValue;
    int                iValue1;
    int                iValue2;
    int                iValue3;

    std::wcout << _T("BEGIN TBD FILE INFO\n------------\n");
    std::wcout << _T("File Name ") << _CSTR_ m_csFileName << _T("\n\n");

    std::wcout << _T("Breaks Info\n");
    std::wcout << _T("Num breaks: ") << m_aBreakItem.GetSize() <<_T("\n\n");
    for ( int i = 0; i < m_aBreakItem.GetSize(); i++ ) {
        cbItem = *m_aBreakItem.GetAt(i);
        std::wcout << _T("Break Name: ") << _CSTR_ cbItem.GetName() << _T(" ");
        std::wcout << _T("Break Len : ") << cbItem.GetLen() << _T("\n");
    }

    std::wcout << _T("\n\nTable Info\n");
    std::wcout << _T("Num tables: ") << m_aTbdTable.GetSize() << _T("\n\n");

    CString csOut;
    for ( int i = 0; i < m_aTbdTable.GetSize(); i++ ) {
        pTable = m_aTbdTable.GetAt(i);

        csOut.Format( _T("Name=%-25ls, Type=%1d, CellSize=%3d, NumDim=%1d, Dim0=%3d, Dim1=%3d, Dim2=%3d, NumBreak=%d"),
            pTable->GetTableName().GetBuffer(0),
            pTable->GetTableType(),
            pTable->GetCellSize(),
            pTable->GetNumDims(),
            pTable->GetDimSize(0),
            pTable->GetNumDims() >= 2 ? pTable->GetDimSize(1) : 0,
            pTable->GetNumDims() >= 3 ? pTable->GetDimSize(2) : 0,
            pTable->GetNumBreak() );

        std::wcout << _CSTR_ csOut << _T("\n");
    }

    std::wcout << _T("       BEGIN SHOW SLICE\n");

    CString     csMsg;
    CString     csValue;
    CString     csFilePos;
    byte*       pValue;
    long        lSliceFilePos=0L;
    while ( (pSlice = GetNextSlice(lSliceFilePos)) != NULL ) {
        pTable = pSlice->GetTable();

        csMsg = pTable->GetTableName();

        csFilePos.Format( _T(" File Pos : %d (0x%X)"), pSlice->GetFilePos(), pSlice->GetFilePos() );

        csMsg = csMsg + csFilePos;
        if( pTable->GetNumBreak() >= 1 ) {
            CString  csKey;

            csKey = _CSTR_ pSlice->GetBreakKey()+ sizeof(short);

            csMsg = csMsg + _T(" Break = ") + _T("(") + csKey + _T(")");
        }

        std::wcout << _CSTR_ csMsg << _T("\n");

        pAcum = pSlice->GetAcum();
        byte*   pDefaultValue = pAcum->GetDefaultValue();
        double  dDefaultValue;

// RHF INIC Jan 31, 2003
        CString         csTitle;
        CStringArray    aLines;
        int             iNumDec=2;
        pAcum->Dump( aLines, csTitle, iNumDec, true );

        for( int i=0; i < aLines.GetSize(); i++ ) {
            std::wcout << _CSTR_ aLines.ElementAt(i);
        }
        if( false )
// RHF END Jan 31, 2003

        for( int iLayer=0; iLayer < pAcum->GetNumLayers(); iLayer++ ) {
            if( pAcum->GetNumLayers() > 1 ) {
                csMsg.Format( _T("---> Layer %d"), iLayer );
                std::wcout << _CSTR_ csMsg << _T("\n");
            }

            for( int iRow=0; iRow < pAcum->GetNumRows(); iRow++ ) {
                csMsg = _T("");
                for( int iCol=0; iCol < pAcum->GetNumCols(); iCol++ ) {
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    switch ( pTable->GetTableType() ) {
                    case CTableDef::Ctab_Freq :
                        {
                        /* RHF COM INIC Oct 24, 2001
                            csValue.Format( "%.*s", CTableAcum::GetCodeSize(pTable->GetCellSize(), 0), pValue);
                            memcpy((csprochar*)&dValue, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 0), sizeof(double));

                            std::wcout << "<" << iRow << "," << iCol << "," << iLayer << "> <" << csValue << "> = " << dValue << "\n";
                            RHF COM END Oct 24, 2001 */

                            // Show cell new format
                            // RHF INIC Oct 24, 2001
                            ASSERT(false);
#ifdef REMOVED_WITH_FREQ_REFACTOR_IN_CSPRO76
                            int     iOffSet = 0;
                            short   iVSetNum;
                            byte    cKind;
                            byte    cFlag;
                            double  aCounter[_FreqCellNumCounters];
                            TCHAR    pszCode[_FreqMaxCodeSize+2];

                            FreqCell  FreqCell;
                            FreqCell.SetData( pValue, pTable->GetCellSize() );
                            //FreqCell.GetData( &iVSetNum, &cKind, &cFlag, aCounter, (csprochar*)pszCode );
                            FreqCell.GetData( &iVSetNum, &cKind, &cFlag, aCounter, (TCHAR*)pszCode );

                            csMsg.Format( _T("(%3d) %02d, %3d, %d, %8ls, %8ls, %8ls, %8ls, %8ls, %8ls, (%ls)"), iRow, iVSetNum,
                                cKind, cFlag,
                                (LPCTSTR)alpha_value(aCounter[0]),
                                (LPCTSTR)alpha_value(aCounter[1]),
                                (LPCTSTR)alpha_value(aCounter[2]),
                                (LPCTSTR)alpha_value(aCounter[3]),
                                (LPCTSTR)alpha_value(aCounter[4]),
                                (LPCTSTR)alpha_value(aCounter[5]),
                                pszCode );
                            std::wcout << _CSTR_ csMsg << _T("\n");
                            // RHF END Oct 24, 2001
#endif
                        }
                        break;
                    case CTableDef::Ctab_FreqDisjoint1Dim :
                        csValue.Format( _T("%.*s"), CTableAcum::GetCodeSize(pTable->GetCellSize(), 1), pValue);
                        memcpy((char*)&dValue, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 1), sizeof(double));
                        memcpy((char*)&iValue1, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 1) + sizeof(double), sizeof(int));

                        std::wcout << _T("<") << iRow << _T(",") << iCol << _T(",") << iLayer << _T("> <") << _CSTR_ csValue << _T("> = ") << dValue << _T(" Dims <") << iValue1 << _T(">\n");

                        break;
                        break;
                    case CTableDef::Ctab_FreqDisjoint2Dim :
                        csValue.Format( _T("%.*s"), CTableAcum::GetCodeSize(pTable->GetCellSize(), 2), pValue);
                        memcpy((char*)&dValue, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 2), sizeof(double));
                        memcpy((char*)&iValue1, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 2) + sizeof(double), sizeof(int));
                        memcpy((char*)&iValue2, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 2) + sizeof(double) + sizeof(int), sizeof(int));

                        std::wcout << _T("<") << iRow << _T(",") << iCol << _T(",") << iLayer << _T("> <") << _CSTR_ csValue << _T("> = ") << dValue << _T(" Dims <") << iValue1 << _T(",") << iValue2 << _T(">\n");

                        break;
                        break;
                    case CTableDef::Ctab_FreqDisjoint3Dim :
                        csValue.Format( _T("%.*s"), CTableAcum::GetCodeSize(pTable->GetCellSize(), 3), pValue);
                        memcpy((char*)&dValue, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 3), sizeof(double));
                        memcpy((char*)&iValue1, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 3) + sizeof(double), sizeof(int));
                        memcpy((char*)&iValue2, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 3) + sizeof(double) + sizeof(int), sizeof(int));
                        memcpy((char*)&iValue3, pValue + CTableAcum::GetCodeSize(pTable->GetCellSize(), 3) + sizeof(double) + sizeof(int)*2, sizeof(int));

                        std::wcout << _T("<") << iRow << _T(",") << iCol << _T(",") << iLayer << _T("> <") << _CSTR_ csValue << _T("> = ") << dValue << _T(" Dims <") << iValue1 << _T(",") << iValue2 << _T(",") << iValue3 << _T(">\n");

                        break;
                        // OTHER DOUBLE
                    case CTableDef::Ctab_SMean: //Savy for SMEAN
                        std::wcout << _T("This is a SMean Accumulator \n");
                        break;
                    case CTableDef::Ctab_STable: //Savy for STable
                        break;

                    default:
                        memcpy( (byte*)&dDefaultValue, pDefaultValue, pTable->GetCellSize() );
                        memcpy((byte*)&dValue, pValue, pTable->GetCellSize());
                        //csMsg.Format( "%3d,%3d,%3d=%6.2f", iRow, iCol, iLayer, dValue );
                        if( dValue == dDefaultValue )
                            csValue = _T("------");
                        else if( dValue <= 1.0e50 )
                            csValue.Format( _T("%6.2f"), dValue );
                        else
                            csValue = _T("******");

                        csMsg = csMsg + ( (iCol==0) ? _T("") : _T("  ") ) + csValue;
                        std::wcout << _T("<") << iCol << _T(",") << iRow << _T(",") << iLayer << _T("> = ") << _CSTR_ csValue << _T("\n"); // rcl, Nov 2004
                        break;
                    }
                    //std::wcout << csMsg <<  _T("\n");
                }
            }
        }

        delete pSlice;
        }
        std::wcout << _T("       END SHOW SLICE\n");

        std::wcout << _T("END TBD FILE INFO\n");
}
// --------------------------------
// Method: ShowSMeanOrSTbl
// --------------------------------
void CTbdFile::ShowSMeanOrSTbl() {
    CBreakItem cbItem;
    CTableAcum* pAcum;
    CTbdSlice* pSlice;
    CTbdTable* pTable;

    std::wcout << _T("BEGIN TBD FILE INFO\n------------\n");
    std::wcout << _T("File Name ") << _CSTR_ m_csFileName << _T("\n\n");

    std::wcout << _T("Breaks Info\n");
    std::wcout << _T("Num breaks: ") << m_aBreakItem.GetSize() <<_T("\n\n");
    for ( int i = 0; i < m_aBreakItem.GetSize(); i++ ) {
        cbItem = *m_aBreakItem.GetAt(i);
        std::wcout << _T("Break Name: ") << _CSTR_ cbItem.GetName() << _T(" ");
        std::wcout << _T("Break Len : ") << cbItem.GetLen() << _T("\n");
    }

    std::wcout << _T("\n\nTable Info\n");
    std::wcout << _T("Num tables: ") << m_aTbdTable.GetSize() << _T("\n\n");

    CString csOut;
    for ( int i = 0; i < m_aTbdTable.GetSize(); i++ ) {
        pTable = m_aTbdTable.GetAt(i);

        csOut.Format( _T("Name=%-25ls, Type=%1d, CellSize=%3d, NumDim=%1d, Dim0=%3d, Dim1=%3d, Dim2=%3d, NumBreak=%d"),
            pTable->GetTableName().GetBuffer(0),
            pTable->GetTableType(),
            pTable->GetCellSize(),
            pTable->GetNumDims(),
            pTable->GetDimSize(0),
            pTable->GetNumDims() >= 2 ? pTable->GetDimSize(1) : 0,
            pTable->GetNumDims() >= 3 ? pTable->GetDimSize(2) : 0,
            pTable->GetNumBreak() );

        std::wcout << _CSTR_ csOut << _T("\n");
    }

    std::wcout << _T("       BEGIN SHOW SLICE\n");

    CString     csMsg;
    CString     csValue;
    CString     csFilePos;
    byte*       pValue;
    long        lSliceFilePos=0L;
    while ( (pSlice = GetNextSlice(lSliceFilePos)) != NULL ) {
        pTable = pSlice->GetTable();

        csMsg = pTable->GetTableName();

        csFilePos.Format( _T(" File Pos : %d (0x%X)"), pSlice->GetFilePos(), pSlice->GetFilePos() );

        csMsg = csMsg + csFilePos;
        if( pTable->GetNumBreak() >= 1 ) {
            CString  csKey;

            csKey = _CSTR_ pSlice->GetBreakKey()+ sizeof(short);

            csMsg = csMsg + _T(" Break = ") + _T("(") + csKey + _T(")");
        }

        std::wcout << _CSTR_ csMsg << _T("\n");
        //continue; // First Get the break keys . Remove this continue once yoy are done checking this
        pAcum = pSlice->GetAcum();
        byte*   pDefaultValue = pAcum->GetDefaultValue();

// RHF INIC Jan 31, 2003
        CString         csTitle;
        CStringArray    aLines;
        int             iNumDec=2;

        //Dump will not work because it is type casting it to double fix this if needed
#ifdef _VARIANCES_DUNP
        pAcum->Dump( aLines, csTitle, iNumDec, true );
#endif
        for( int i=0; i < aLines.GetSize(); i++ ) {
            std::wcout << _CSTR_ aLines.ElementAt(i);
        }
        if( false )
// RHF END Jan 31, 2003

        for( int iLayer=0; iLayer < pAcum->GetNumLayers(); iLayer++ ) {
            if( pAcum->GetNumLayers() > 1 ) {
                csMsg.Format( _T("---> Layer %d"), iLayer );
                std::wcout << _CSTR_ csMsg << _T("\n");
            }

            for( int iRow=0; iRow < pAcum->GetNumRows(); iRow++ ) {
                csMsg = _T("");
                for( int iCol=0; iCol < pAcum->GetNumCols(); iCol++ ) {
                    pValue = pAcum->GetValue( iRow, iCol, iLayer );

                    switch ( pTable->GetTableType() ) {
                    case CTableDef::Ctab_SMean: //Savy for SMEAN
                        std::wcout << _T("This is a SMean Accumulator \n");
                        break;
                    case CTableDef::Ctab_STable: //Savy for STable
                        break;

                    default:
                        break;
                    }
                    //std::wcout << csMsg <<  _T("\n");
                }
            }
        }

        delete pSlice;
        }
        std::wcout << _T("       END SHOW SLICE\n");

        std::wcout << _T("END TBD FILE INFO\n");
}

short CTbdFile::GetTableNum( const csprochar* pszKey, int iSlotSize ) {
    short   sTableNum;

    if( iSlotSize == sizeof(char) )
        sTableNum = pszKey[0];
    else if( iSlotSize == sizeof(short) ) {
        memcpy( &sTableNum, pszKey, sizeof(short) );

        sTableNum = (-sTableNum-1) / 2;

        if( sTableNum < 0 )
            sTableNum =  32768 + sTableNum; // (16384+sTableNum)+16384;
    }
    else
        ASSERT(0);

    return sTableNum;
}

// Save a short number (transformed) without 0x00 byte
// sTableNum between 1 and 32000
int CTbdFile::SetTableNum( csprochar* pszKey, short sTableNum, int iSlotSize ) {
    if( iSlotSize == sizeof(char)  ) {
        ASSERT( sTableNum >= 1 && sTableNum <= 255 );
        pszKey[0] = (char)sTableNum;
    }
    else if( iSlotSize == sizeof(short) ) {
        ASSERT( sTableNum >= 1 && sTableNum <= 32000 );
        sTableNum = -(sTableNum * 2 + 1); // Impar

        memcpy( pszKey, &sTableNum, sizeof(short) );

    }
    else
        ASSERT(0);

    return iSlotSize;
}


int CTbdFile::GetBreakKeyLen() {
    return m_iBreakLen;
}

// size of file in bytes (only valid when open for read)
long CTbdFile::GetFileSize() const
{
    return m_iFileSize;
}
