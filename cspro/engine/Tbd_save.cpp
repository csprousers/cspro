//---------------------------------------------------------------------------
//
// TBDSAVE.cpp        saves a TBD file
//
//---------------------------------------------------------------------------
#include <io.h>

#include "StandardSystemIncludes.h"
#include "Batdrv.h"
#include "Ctab.h"
#include "Engine.h"
#include "Tables.h"
#include "Tbd_save.h"
#include "RangeFunctions.h"
#include <zToolsO/Tools.h>
#include <zUtilO/SimpleDbMap.h>
#include <fcntl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

static    const csprochar    *std_tParam = _T("NNNNNNNYYYYYY999 0D  1 00  1 00  1 00       1YNNCYYNY132 60 320000");

#define slot_type   int
typedef struct
{
    slot_type     m_iNodeType;             // node type: Variable or Operator
    slot_type     m_iNumCells;             // # of cells in associated subtree
    struct
    {
        slot_type m_iInfo1;
        slot_type m_iInfo2;
    } m_NodeInfo;
} OLD_TBD_CTNODE;

typedef struct
{
    slot_type     m_iRangeLow;
    slot_type     m_iRangeHigh;
} OLD_TBD_CTRANGE;




CTbd::CTbd()
{
    m_pEngineDriver = NULL;
    m_pBatchDriverBase = NULL;
    m_pEngineArea = NULL;
    m_engineData = nullptr;
    m_pEngineSettings = NULL;

    m_iMaxNumTables = 0;
    m_iNumTables = 0;
    m_iNumVars = 0;

    m_aVar = NULL;
    m_aBreakVar = NULL;
    m_aTbdTable = NULL;

    m_pAuxCtNodebase = NULL;
    m_iAuxCtNodenext = 0;
    m_iCtabTemp = -1;

    m_csTbdFileName = _T("");
    m_pBreakidnext=NULL;
    m_pBreakidcurr=NULL;

    memset( m_iBreakValue, 0, MAXBREAKVARS*sizeof(long) );

    // TBD and TBI files (used in TBD_SAVE & BREAK)
    m_pTableIndex = NULL;
    m_pTbdFile = NULL;

    SetNewTbd( false );
}

CTbd::~CTbd()
{
    if( m_pTableIndex != NULL )
        delete m_pTableIndex;
    m_pTableIndex = NULL;

    if( m_pTbdFile != NULL )
        delete m_pTbdFile;
    m_pTbdFile = NULL;
}



void CTbd::SetBatchDriver( CBatchDriverBase* pBatchDriverBase )
{
    m_pBatchDriverBase = pBatchDriverBase;
    m_pEngineDriver = pBatchDriverBase;
    m_pEngineArea = m_pEngineDriver->getEngineAreaPtr();
    m_engineData = &m_pEngineArea->GetEngineData();
    m_pEngineSettings = &(m_pEngineDriver->m_EngineSettings);
    m_pIntDriver = m_pEngineDriver->m_pIntDriver.get();
}


const Logic::SymbolTable& CTbd::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


void CTbd::tbd_init( const TCHAR* pszTbdName ) {   // alloc/fill TbTable; open TBD file
    CString csFileName = PortableFunctions::PathRemoveFileExtension<CString>(pszTbdName) + _T(".") + GetTbdExtension();
    CTAB* pCtab;


    if( IsNewTbd() ) {
        if( m_pTbdFile != NULL )
            delete m_pTbdFile;

        csprochar    cTbdVersion=1;
        m_pTbdFile = new CTbdFile(csFileName, cTbdVersion);

        ASSERT( m_pTbdFile );

        csprochar    cBreakOtherInfo=0;
        // Add Break vars
        for( int iBreakVar = 0; iBreakVar < Breaknvars; iBreakVar++ ) {
            int     iLen = Breaklvar[iBreakVar];
            CString csName = WS2CS(NPT(Breakvars[iBreakVar])->GetName());

            CBreakItem* pBreakItem= new CBreakItem( csName, iLen, cBreakOtherInfo );

            if( !m_pTbdFile->AddBreak(pBreakItem) )
                issaerror( MessageType::Abort, 672 );
        }

                // RHF INIC Aug 11, 2004
                //Add break vars to the TBD when they were not defined in the application break by command.
                if( Breaknvars == 0 ) {
                        for(int iBreakVar = 0; iBreakVar < (int)m_pEngineArea->m_aCtabBreakId.size(); iBreakVar++ ) {
                                CBreakById&     rThisBreakId=m_pEngineArea->m_aCtabBreakId[iBreakVar];

                                int     iLen = rThisBreakId.m_iLen;
                                CString csName = WS2CS(NPT(rThisBreakId.m_iSymVar)->GetName());

                                CBreakItem* pBreakItem= new CBreakItem( csName, iLen, cBreakOtherInfo );

                                if( !m_pTbdFile->AddBreak(pBreakItem) )
                                        issaerror( MessageType::Abort, 672 );
                        }
                }
                // RHF END Aug 11, 2004*/

        // Add Tables
        int iTableNum=0;
        for( CTAB* pCtab : m_engineData->crosstabs )
        {
            if( pCtab->GetAcumArea() == NULL )
                continue;

            pCtab->SetTableNumber( iTableNum );
            iTableNum++;

            CString csName = WS2CS(pCtab->GetName());
            int     iNumDims=pCtab->GetNumDim();
            int     iTableType=pCtab->GetTableType();
            int     iCellSize=pCtab->GetCellSize();

            CString csNextTable=_T("");
            csprochar    cTableOtherInfo=0;

            // Do DimSize to pTable
            CArray<int,int> aDimSize;
            for( int iDim=0; iDim < iNumDims; iDim++ )
                aDimSize.Add( pCtab->GetTotDim(iDim) );

            // Fill the table
            CTbdTable* pTable = new CTbdTable( csName, csNextTable,  aDimSize, (CTableDef::ETableType)iTableType, iCellSize, cTableOtherInfo ); // TODO Talvez Mejor en CTAB.H
            // Add Break vars to pTable

            // RHF INIC Apr 17, 2003
            int     iNumBreaks=pCtab->GetNumBreaks();
            if( iNumBreaks == 0 && pCtab->m_uEnviron[0] & ct_BREAK )
                iNumBreaks = Breaknvars;
            // RHF END Apr 17, 2003

            // RHF COM Apr 17, 2003 int iNumBreaks = Breaknvars;

            for( int iBreakVar = 0; iBreakVar < iNumBreaks; iBreakVar++ ) {
                CBreakItem* pBreak =  m_pTbdFile->GetBreak(iBreakVar);
                pTable->AddBreak( pBreak );
            }
            //BEGIN SAVY 05/10/2005 for making breakkey in slices to be always totalbreaklen
            int  iTotalBreakLen=breaklen(Breaknvars);
            m_pTbdFile->SetBreakKeyLen(iTotalBreakLen);
            pTable->SetTbdFileBreakKeyLen(m_pTbdFile->GetBreakKeyLen());
            //End SAVY
            // Add the table to the TbdFile
            if( !m_pTbdFile->AddTable( pTable ) )
                issaerror( MessageType::Abort, 674 );

            // Create slice
            CTbdSlice* pTbdSlice=new CTbdSlice( pTable );
            CTableAcum* pAcum=new CTableAcum( pTable );
            pAcum->ShareArea( pCtab->m_pAcum );

            pTbdSlice->SetAcum( pAcum, true );
            TBD_SLICE_HDR           tSHdr;

            //GetBreakKeyLen return length in characters. total length needs the number of bytes -Savy 01/31/2012 for unicode changes
            tSHdr.iTotalLenght = sizeof(TBD_SLICE_HDR) + pCtab->GetNumCells() * pCtab->GetCellSize() + pTbdSlice->GetBreakKeyLen()*sizeof(TCHAR);
            tSHdr.iTableNumber = pCtab->GetTableNumber();
            tSHdr.cSliceStatus = 0;
            tSHdr.cOtherInfo = 0;

            pTbdSlice->SetSliceHdr(&tSHdr);

            pCtab->SetTbdSlice( pTbdSlice );
            m_pTbdFile->AddSlice( pTbdSlice, false ); // destructor delete the slice
        }

        //Creation
        bool     bCreate=true;
        if( !m_pTbdFile->Open( bCreate ) ) //Create an empty trailer and initialize some vars
            issaerror( MessageType::Abort, 589, csFileName.GetString() );

        m_pTbdFile->Close();

        if( !m_pTbdFile->DoOpen( bCreate ) ) // Don't create an empty trailer
            issaerror( MessageType::Abort, 589, csFileName.GetString() );
    }

    else {
        m_iTbdFile = _topen( csFileName, O_RDWR | O_RAW | O_TRUNC | O_CREAT, 0666 );
        if( m_iTbdFile < 0 )
            issaerror( MessageType::Abort, 589, csFileName.GetString() );

        m_iNumTables = 0;
        m_iMaxNumTables = m_engineData->crosstabs.size() + 1; /* RHF 7/6/96 */
        m_aTbdTable = (csprochar *) calloc( m_iMaxNumTables, (LNAME8+1)*sizeof(TCHAR));
        if( m_aTbdTable == NULL && m_iMaxNumTables > 0 )
            issaerror( MessageType::Abort, 1000, _T("m_aTbdTable") );

        for( CTAB* pCtab : m_engineData->crosstabs )
        {
            if( pCtab->GetAcumArea() == NULL )
                continue;

            tbd_newtable( pCtab->GetSymbolIndex() );    /* add table name to m_aTbdTable array */
        }
    }
}

// tbd_MakeFinalTbd: concatenacion de las tablas (m_iTbdFile) con header (CtabTmp)
void CTbd::tbd_MakeFinalTbd() {
    if( IsNewTbd() ) {
        //m_pTbdFile->WriteAllSlices( true );
        m_pTbdFile->WriteTrailer();
        m_pTbdFile->Close();
        return;
    }
    csprochar    eof = 0x1A;

    _lseek( m_iTbdFile, 0l, 2 );            /* Eof */
    write( m_iTbdFile, &eof, sizeof(csprochar) ); /* EOF virtual */

    /* Temporal Header */
    /*### tbd_Open ya debe haber llenado m_pEngineDriver->m_TmpCtabTmp con un nombre! */

    m_iCtabTemp = _topen( m_pEngineDriver->m_TmpCtabTmp, O_RDONLY | O_RAW, 0666 );

    if( m_iCtabTemp < 0 )                   /* cannot open */
        issaerror( MessageType::Abort, 2001, m_pEngineDriver->m_TmpCtabTmp.GetString() );

    tbd_catheader( m_iCtabTemp, m_iTbdFile );

    free( m_aTbdTable );
    _close( m_iCtabTemp );                   /* remove( "$$CTAB$$.TMP" ); */
    PortableFunctions::FileDelete( m_pEngineDriver->m_TmpCtabTmp );
    _close( m_iTbdFile );
    m_iTbdFile = -1;
}

void CTbd::tbd_Delete( CString csName ) {
    CString csFileName = PortableFunctions::PathRemoveFileExtension<CString>(csName) + _T(".") + GetTbdExtension();
    PortableFunctions::FileDelete(csFileName);

    csFileName = PortableFunctions::PathRemoveFileExtension<CString>(csName) + _T(".") + GetTbiExtension();
    PortableFunctions::FileDelete(csFileName);
}

// RHF INIC Dec 19, 2001
int CTbd::tbd_tabindex( const csprochar* pszTableName ) { // index of "tname" in array m_aTbdTable
    ASSERT( !IsNewTbd() );

    int     iLen=_tcslen(pszTableName);
    for( int i = 0; i < m_iNumTables; i++ ) {
        // strlen(pszTableName)+1 in order to considerate \0.
        if( _tcsncicmp( ( m_aTbdTable + LNAME8 * i ), pszTableName, std::min( iLen+1, LNAME8 ) ) == 0 )
            return( i );
    }

    return -1;
}
// RHF END Dec 19, 2001

/* RHF COM INIC Dec 19, 2001
int CTbd::tbd_tabindex( csprochar* pszTableName ) { // index of "tname" in array m_aTbdTable
    ASSERT( !IsNewTbd() );

    for( int i = 0; i < m_iNumTables; i++ )
        if( _memicmp( ( m_aTbdTable + LNAME8 * i ), pszTableName, min( strlen(pszTableName), LNAME8 ) ) == 0 )
            return( i );

        return -1;
}
RHF COM END Dec 19, 2001 */

void CTbd::tbd_WriteTrailer() { // write the TBD file to disk
    ASSERT( !IsNewTbd() );

    tbd_Open();

    m_pAuxCtNodebase = NULL;

    m_pAuxCtNodebase = (int *) calloc( CtNodenext, sizeof(int) );
    if( m_pAuxCtNodebase == NULL && CtNodenext > 0 )
        issaerror( MessageType::Abort, 1000, _T("CTAB") );

    // RHF INIC Jul 13, 2001
    m_iAuxCtNodenext = 0;

    for( CTAB* ct : m_engineData->crosstabs )
    {
        // RHF INIC Jul 31, 2001
        // This table can't be printed by exprtab
        if( ct->GetRunTimeVersion() >= 3 && (ct->m_uEnviron[0] | ct_PRINT) ) {
            issaerror( MessageType::Warning, 619, ct->GetName().c_str() );
            ct->m_uEnviron[0] &= ~ct_PRINT;
            continue; // RHF Sep 10, 2002
        }
        // RHF END Jul 31, 2001

        for( int iDim = 0; iDim < 3; iDim++ ) {
            if( ct->GetNodeExpr(iDim) >= 0 )
                ct->SetNodeExpr( tbd_copytree( ct->GetNodeExpr(iDim) ), iDim );
        }
    }
    // RHF END Jul 13, 2001

    m_aVar  = (TBVAR *) calloc( TBDMAXVARS, sizeof(TBVAR) );
    if( m_aVar == NULL && TBDMAXVARS > 0 )
        issaerror( MessageType::Abort, 1000, _T("m_aVar") );

    m_aBreakVar = (TBBVAR *) calloc( MAXBREAKVARS, sizeof(TBBVAR) );
    if( m_aBreakVar == NULL && MAXBREAKVARS > 0 )
        issaerror( MessageType::Abort, 1000, _T("m_aBreakVar") );

    tbd_DoArrays();
    tbd_DoWriteTrailer();

    free( (csprochar *) m_pAuxCtNodebase );
    free( (csprochar *) m_aVar );
    free( (csprochar *) m_aBreakVar );
    tbd_Close();
}

// local
// tbd_DoArrays: make array of vars (m_aVar), array with break-vars (m_aBreakVar)
//               and other array with the tables name (m_aTbdTable)
void CTbd::tbd_DoArrays() {
    ASSERT( !IsNewTbd() );

    m_pBatchDriverBase->SetHasAnyTable( false );
    m_iNumVars = 0;

    for( CTAB* ct : m_engineData->crosstabs )
    {
        if( ct->GetAcumArea() == NULL )
            continue;

        m_pBatchDriverBase->SetHasAnyTable( true ); // some table generated to TBD file

        // add normal vars to m_aVar & modify the m_pAuxCtNodebase array w/index
        for( int k = 0; k < ct->GetNumDim(); k++ )
            tbd_addvar( ct->GetNodeExpr(k) );

        // Mean/SMean/HotDeck
        if( ct->GetTableType() != CTableDef::Ctab_Crosstab && ct->GetTableType() != CTableDef::Ctab_STable )  /* RHF 26/9/96 STABLE */ {
            for( int k = 0; k < 2 && ct->GetDepSymVar(k) >= 0; k++ )
                tbd_newvar( ct->GetDepSymVar(k) );
        }
    }

    for( int k = 0; k < Breaknvars; k++ )
    {
        ( m_aBreakVar + k )->len = Breaklvar[k];
        /* Add the break vars to m_aVar array */
        ( m_aBreakVar + k )->varnum = tbd_newvar( Breakvars[k] );
    }
}

// local
// open a file header
void CTbd::tbd_Open() {
    ASSERT( !IsNewTbd() );

    // some ICFI apps were failing because a previous $$CTAB$$.TMP file was locked, so create a unique file name
    m_pEngineDriver->m_TmpCtabTmp = WS2CS(GetUniqueTempFilename(_T("$$CTAB$$.TMP")));

    m_iCtabTemp = _topen( m_pEngineDriver->m_TmpCtabTmp, O_RDWR | O_RAW | O_TRUNC | O_CREAT, 0666 );

    if( m_iCtabTemp < 0 )
        issaerror( MessageType::Abort, 589, m_pEngineDriver->m_TmpCtabTmp.GetString() );
}

// local
void CTbd::tbd_Close() {  // Close TBD File
    ASSERT( !IsNewTbd() );

    if( m_iCtabTemp >= 0 )
        _close( m_iCtabTemp );
    m_iCtabTemp = -1;
}


// local
void CTbd::tbd_DoWriteTrailer() {
    ASSERT( !IsNewTbd() );

    _TUCHAR      bvars, tbltype;
    csprochar    cero = 0x00, tparam[256];
    short   i, k;
    short   shortnum;
    long    pinicoffset, pendoffset, pvoffset, ptoffset,
        nolabels = -1, posaux;
    csprochar    numlen[256];

    pinicoffset = _tell( m_iCtabTemp );      // current position (inic of header)

    // Offset Unknown
    write( m_iCtabTemp, BLANKS, sizeof(long)); //offset is type long

    // Global Parameters
    write( m_iCtabTemp, G_PARAM, 39*sizeof(TCHAR) );

    // Len of array m_pAuxCtNodebase
    //  k = m_iAuxCtNodenext * sizeof(short);                     RHF 6/9/94
    // TRUCO: se agrega a la cola de m_iAuxCtNodenext             RHF 6/9/94
    //        un area para guardar los decimales                  RHF 6/9/94
    //        en los porcentajes de cada tabla                    RHF 6/9/94
    k = ( m_iAuxCtNodenext + m_engineData->crosstabs.size() + 1 ) * sizeof(short);         // RHF 6/9/94

    write( m_iCtabTemp, &k, sizeof(short) );

    // Array m_pAuxCtNodebase
    for( int i = 0; i < m_iAuxCtNodenext; i++ )
    {
        shortnum = (short) *(m_pAuxCtNodebase + i);
        write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );
    }

    // Indicador de continuacion - no choca
    // con ningun valor de m_pAuxCtNodebase
    shortnum = -1;
    write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

    for( CTAB* ct : m_engineData->crosstabs )
    {
        short sPercent=ct->GetPercentDecs();
        write( m_iCtabTemp, (csprochar *) &sPercent, sizeof(short) );
    }

    // Number of vars
    shortnum = m_iNumVars;
    write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

    // Names and Unknown offset of vars
    pvoffset = _tell( m_iCtabTemp ) + LNAME8*sizeof(TCHAR);     // Pos. of first offset of m_aVar
    for( i = 0; i < m_iNumVars; i++ )
    {
        write( m_iCtabTemp, (m_aVar + i)->varname, LNAME8*sizeof(TCHAR) ); // Name of var
        write( m_iCtabTemp, BLANKS, sizeof(long));    // Offset Unknown offset for TABVAR is type long
    }

    // Number of break vars
    bvars = Breaknvars;
    write( m_iCtabTemp, (csprochar *) &bvars, sizeof(csprochar) );

    // Numbers and len of break vars
    for( i = 0; i < Breaknvars; i++ )
    {
        // index in m_aVar and lenght of break var i
        shortnum = (m_aBreakVar + i)->varnum;
        write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );
        write( m_iCtabTemp, (csprochar *) &(m_aBreakVar + i)->len, sizeof(csprochar) );
    }

    // number of tables
    shortnum = m_iNumTables;
    write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

    // Names and Unknown offset of tables
    ptoffset = _tell( m_iCtabTemp ) + LNAME8*sizeof(TCHAR); // Pos. of first offset of m_aTbdTable
    for( i = 0; i < m_iNumTables; i++ )
    {
        write( m_iCtabTemp, m_aTbdTable + LNAME8 * i, LNAME8*sizeof(TCHAR) ); // Name of table
        write( m_iCtabTemp, BLANKS, sizeof(long) );          // Offset Unknown
    }

    // Labels
    for( i = 0; i < m_iNumVars; i++ )
    {
        posaux = _tell( m_iCtabTemp );

        std::vector<TCHAR> label_data = m_pEngineArea->lab_load( ( m_aVar + i )->SYMTidx );

        if( label_data.empty() ) // This var has no label
        {
            _lseek( m_iCtabTemp, pvoffset, 0 );
            write( m_iCtabTemp, (csprochar *) &nolabels, sizeof(long) );
            _lseek( m_iCtabTemp, posaux, 0 );
        }
        else
        {
            _lseek( m_iCtabTemp, pvoffset, 0 );
            write( m_iCtabTemp, (csprochar *) &posaux, sizeof(long) ); // Offset
            _lseek( m_iCtabTemp, posaux, 0 );
            write( m_iCtabTemp, label_data.data(), label_data.size()*sizeof(TCHAR) );  // Labels
            write( m_iCtabTemp, &cero, sizeof(csprochar) );
        }
        //pvoffset += 12;                 // Next offset of var
        pvoffset += LNAME8*sizeof(TCHAR) + sizeof(long) ;                 // Next offset of var
    }

    // Tables' specifications
    for( CTAB* ct : m_engineData->crosstabs )
    {
        _tcscpy( tparam, std_tParam );   // reloads standard parameters

        if( ct->GetAcumArea() == NULL )
            continue;

        posaux = _tell( m_iCtabTemp );

        _lseek( m_iCtabTemp, ptoffset, 0 );
        write( m_iCtabTemp, (csprochar *) &posaux, sizeof(long) ); // Offset table

        _lseek( m_iCtabTemp, posaux, 0 );

        // Tipo de la tabla
        tbltype = ct->GetTableType();
        write( m_iCtabTemp, (csprochar *) &tbltype, sizeof(csprochar) );

        // Len of cells
        short siAcumType = ct->GetAcumType();
        write( m_iCtabTemp, &siAcumType, sizeof(siAcumType) );

        // Numero de decimales
        short siNumDec = ct->GetNumDec();
        write( m_iCtabTemp, &siNumDec, sizeof(siNumDec) );

        // depSYMTvars and m_iDepVarOcc
        for( k = 0; k < 2; k++ )
        {
            // Index relative to the m_aVar array
            shortnum = ct->GetDepSymVar(k);
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

            csprochar   iVarOcc= ct->GetDepVarOcc(k);
            write( m_iCtabTemp, &iVarOcc, sizeof(csprochar) );
        }

        // Table Parameters
        if( ct->OptionActivated(ct_ROWTOT) )
            tparam[0] = _T('Y');
        if( ct->OptionActivated(ct_COLTOT) )
            tparam[1] = _T('Y');
        if( ct->OptionActivated(ct_LAYTOT) )
            tparam[2] = _T('Y');
        if( ct->OptionActivated(ct_CHISQUARE) )
            tparam[3] = _T('Y');
        if( ct->OptionActivated(ct_ROWPCT) )
            tparam[4] = _T('Y');
        if( ct->OptionActivated(ct_COLPCT) )
            tparam[5] = _T('Y');
        if( ct->OptionActivated(ct_LAYPCT) )
            tparam[6] = _T('Y');

        if( ct->m_uEnviron[0] & ct_FREQ )
            tparam[7] = _T('N');
        if( ct->OptionActivated(ct_ROWZERO) )
            tparam[8] = _T('N');
        if( ct->OptionActivated(ct_COLZERO) )
            tparam[9] = _T('N');
        if( ct->OptionActivated(ct_LAYZERO) )
            tparam[10] = _T('N');
        if( ct->OptionActivated(ct_MISSING) )
            tparam[11] = _T('N');
        if( ct->OptionActivated(ct_DEFAULT) || ct->OptionActivated(ct_NOTAPPL) || ct->OptionActivated(ct_REFUSED) )
            tparam[12] = _T('N');

        // Cell and Decimals
        // sprintf( tparam + 16, "%02d", ct->m_uEnviron[1] );
        _stprintf( tparam + 16, _T("%2d"), ct->m_uEnviron[1] );
        tparam[18] = (TCHAR)ct->GetNumDec() + _T('0');

        if( !( ct->m_uEnviron[0] & ct_PRINT ) )
            tparam[45] = _T('N');

        _stprintf( numlen, _T("%3d"), ct->GetPageWidth() );
        _tmemcpy( tparam + 53, numlen, 3 );

        _stprintf( numlen, _T("%3d"), ct->GetPageLen() );
        _tmemcpy( tparam + 56 , numlen, 3 );

        _stprintf( numlen, _T("%3d"), ct->GetStubWidth() );
        _tmemcpy( tparam + 59, numlen, 3 );

        write( m_iCtabTemp, tparam, _tcslen( std_tParam )* sizeof(TCHAR) );

        // environment
        write( m_iCtabTemp, (csprochar *) &ct->m_uEnviron, 4 * sizeof(csprochar) );

        // Head of trees
        for( k = 0; k < 3; k++ )
        {
            shortnum = ct->GetNodeExpr(k);
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );
        }

        // Title
        csprochar    ceroa = 0x0A;

        if( ct->GetTitleLen() > 0 ) {           // There is title
            // RHF INIC Aug 08, 2001
            shortnum = ct->GetTitleLen();
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

            for( k = 0; k < ct->GetTitle().GetSize(); k++ ) {
                CString& csTitleLine=ct->GetTitle().ElementAt(k);
                if( csTitleLine.GetLength() > 0 )
                    write( m_iCtabTemp, csTitleLine.GetString(), csTitleLine.GetLength()*sizeof(TCHAR) );
                write( m_iCtabTemp, (csprochar *) &ceroa, sizeof(csprochar) );
            }
            // RHF END Aug 08, 2001

        }
        else {                            // no title ( GetTitleLen() == 0 )
            shortnum = ct->GetTitleLen();
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );
        }

        if( ct->GetStubLen() > 0 ) {           // There is stub title
            // RHF INIC Aug 08, 2001
            shortnum = ct->GetStubLen();
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );

            for( k = 0; k < ct->GetStubTitle().GetSize(); k++ ) {
                CString& csStubTitleLine=ct->GetStubTitle().ElementAt(k);
                if( csStubTitleLine.GetLength() > 0 )
                    write( m_iCtabTemp, csStubTitleLine.GetString(), csStubTitleLine.GetLength()*sizeof(TCHAR) );
                write( m_iCtabTemp, (csprochar *) &ceroa, sizeof(csprochar) );
            }
            // RHF END Aug 08, 2001
        }
        else {                            // no stub title ( GetStubLen() == 0 )
            shortnum = ct->GetStubLen();
            write( m_iCtabTemp, (csprochar *) &shortnum, sizeof(short) );
        }

       // ptoffset += 12;   // Next offset of tables
        ptoffset += LNAME8*sizeof(TCHAR) + sizeof(long) ;                 // Next offset of var
      }

      // real EOF
      write( m_iCtabTemp, &pinicoffset, sizeof(long) );
      pendoffset = _tell( m_iCtabTemp );       // End of header

      _lseek( m_iCtabTemp, pinicoffset, 0 );
      write( m_iCtabTemp, &pendoffset, sizeof(long) );
}

static int convertDoubleToInt( double rValue )
{
    double dValue = rValue;
    int iFinalValue = 0;
    if( dValue >= NUMBER_OF_DATA_CELLS )
    {
        // Compare against VAL_CTxxxx constants,
        // if match found -> convert to int "mirror" value

#define CASE(x) if( areEqual(dValue,VAL_##x) ) iFinalValue = x;
#define OR else
        CASE(CTMISSING) OR CASE(CTDEFAULT) OR CASE(CTNOTAPPL) OR
            CASE(CTUNDEFINED) OR CASE(CTTOTAL);
    }
    else
        iFinalValue = (int) rValue;
    return iFinalValue;
}

// local
// RHF INIC Jul 13, 2001
int CTbd::tbd_copytree( int iOffset ) {
    ASSERT( !IsNewTbd() );

    CTNODE*         pSourceNode;
    OLD_TBD_CTNODE* pTargetNode;
    int             iPos = -1;

    pSourceNode = (CTNODE *) ( CtNodebase + iOffset );
    pTargetNode = (OLD_TBD_CTNODE *) ( m_pAuxCtNodebase + m_iAuxCtNodenext );
    iPos = m_iAuxCtNodenext;

    if( pSourceNode->isVarNode() )
    {
        m_iAuxCtNodenext += sizeof(OLD_TBD_CTNODE)/sizeof(slot_type);

        pTargetNode->m_iNodeType    = pSourceNode->m_iSymbol;
        pTargetNode->m_iNumCells    = pSourceNode->m_iNumCells;
        pTargetNode->m_iCtOcc       = pSourceNode->m_iCtOcc;
        pTargetNode->m_iCtNumRanges = pSourceNode->m_iCtNumRanges;
        ASSERT( pTargetNode->m_iCtNumRanges >= 1 );

        int              iStartRange, iEndRange;
        CTRANGE*         pSourceRange;
        OLD_TBD_CTRANGE* pTargetRange;

        /*
        // Create as 1 full-range
        pTargetRange = (OLD_TBD_CTRANGE *) ( m_pAuxCtNodebase + m_iAuxCtNodenext );
        m_iAuxCtNodenext += sizeof(OLD_TBD_CTRANGE) / sizeof(slot_type);
        pTargetRange->m_iRangeLow = 1;
        pTargetRange->m_iRangeHigh = pTargetNode->m_iNumCells;
        pTargetNode->m_iCtNumRanges = 1;

        break; //////
        */

        iStartRange = iOffset + CTNODE_SLOTS;
        iEndRange = iStartRange + CTRANGE_SLOTS * pSourceNode->m_iCtNumRanges;
        for( int i = iStartRange; i < iEndRange; i += CTRANGE_SLOTS ) { // Variable Mapping
            pSourceRange = (CTRANGE *) ( CtNodebase + i );
            pTargetRange = (OLD_TBD_CTRANGE *) ( m_pAuxCtNodebase + m_iAuxCtNodenext );

            if( pSourceRange->m_iRangeCollapsed >= 2 ) {// Only considerated the first range
                pTargetNode->m_iCtNumRanges--;
                ASSERT( pTargetNode->m_iCtNumRanges >= 1 );
                continue;
            }

            m_iAuxCtNodenext += sizeof(OLD_TBD_CTRANGE) / sizeof(slot_type);

            pTargetRange->m_iRangeLow = convertDoubleToInt(pSourceRange->m_iRangeLow);

            if( pSourceRange->m_iRangeCollapsed == 1 ) // low=high
                pTargetRange->m_iRangeHigh = pTargetRange->m_iRangeLow;
            else
                pTargetRange->m_iRangeHigh = convertDoubleToInt(pSourceRange->m_iRangeHigh);
        }
    }
    else
    {
        ASSERT( pSourceNode->isOperNode() );
        int         iCtLeft, iCtRight;

        m_iAuxCtNodenext += sizeof(OLD_TBD_CTNODE)/sizeof(slot_type);

        iCtLeft  = tbd_copytree( pSourceNode->m_iCtLeft );
        iCtRight = tbd_copytree( pSourceNode->m_iCtRight );

        pTargetNode->m_iNodeType    = pSourceNode->getOperator();
        pTargetNode->m_iNumCells    = pSourceNode->m_iNumCells;
        pTargetNode->m_iCtLeft      = iCtLeft;
        pTargetNode->m_iCtRight     = iCtRight;
    }

    return iPos;
}
// RHF END Jul 13, 2001

// local
// tbd_addvar: add vars to the array of variables (m_aVar) from sub-tree
//             Ctabbase+desp and Modify the m_pAuxCtNodebase array with the new index
void CTbd::tbd_addvar( int desp ) {
    ASSERT( !IsNewTbd() );

    if( desp < 0 ) return; // RHF Sep 10, 2002

    OLD_CTNODE* pnode = (OLD_CTNODE *) ( m_pAuxCtNodebase + desp );

    switch( pnode->m_iNodeType )
    {
    case -TOKADDOP:
    case -TOKMULOP:
        tbd_addvar( pnode->m_iCtLeft );
        tbd_addvar( pnode->m_iCtRight );
        break;

    default      : /* Var */
        pnode->m_iNodeType = tbd_newvar( pnode->m_iNodeType );  /* replace SYMTvar */
        break;
    }
}

// local
// tbd_newvar: add variables to the array of variables (m_aVar)
int CTbd::tbd_newvar( int symidx ) {
    ASSERT( !IsNewTbd() );

    if( m_iNumVars >= TBDMAXVARS )
        issaerror( MessageType::Abort, 672 );

    if( symidx <= 0 ) return 0; // RHF Sep 10, 2002

    ASSERT( is_alpha( NPT(symidx)->GetName().front() ) );

    int i;
    for( i = 0; i < m_iNumVars; i++ ) {
        if( symidx == (m_aVar + i)->SYMTidx )
            break;
    }

    if( i == m_iNumVars ) {                    // add the var to array m_aVar
        _tcsnccpy( (m_aVar + i)->varname, NPT(symidx)->GetName().c_str(), LNAME8 ); // rcl, Jun 2005
        (m_aVar + i)->SYMTidx = symidx;
        m_iNumVars++;
    }

    return( i );
}

// local
// tbd_newtable: add tables to the array of tables (m_aTbdTable)
// RHF INIC Dec 19, 2001
int CTbd::tbd_newtable( int symidx ) {
    ASSERT( !IsNewTbd() );

    if( m_iNumTables >= m_iMaxNumTables )
        issaerror( MessageType::Abort, 674 );

    CString csTableName = WS2CS(NPT(symidx)->GetName());
    int     iLen=csTableName.GetLength();

    int i;
    for( i = 0; i < m_iNumTables; i++ ) {
        // ilen+1 in order to considerate \0.
        if( _tcsncicmp( csTableName, ( m_aTbdTable + LNAME8*i ), std::min(iLen+1,LNAME8) ) == 0 )
            break;
    }

    if( i == m_iNumTables ) { // ADD the New table in array m_aTbdTable
        _tcsnccpy( ( m_aTbdTable + LNAME8 *i ), csTableName, LNAME8 );
        m_iNumTables++;
    }

    return( i );
}
// RHF END Dec 19, 2001


// local
void CTbd::tbd_catheader( int f1, int f2 ) {   // concats header of f1 to file f2
    ASSERT( !IsNewTbd() );

    long    pinic, pend, sizeheader;

    pinic = _lseek( f2, 0l, 2 );         /* Eof */
    sizeheader = _lseek( f1, 0l, 2 );    /* size of $$CTAB$$.TMP file */

    _lseek( f1, 0l, 0 );                 /* begin of file */

    tbd_fcopy( f1, f2, sizeheader );

    _lseek( f2, -4l, 2 );
    write( f2, (csprochar *) &pinic, sizeof(long) );
    pend = _lseek( f2, 0L, 2 ); /* EOF */

    _lseek( f2, pinic, 0 );
    write( f2, (csprochar *) &pend, sizeof(long) );
}

// local
void CTbd::tbd_fcopy( int f1, int f2, long len ) {      // copy from f1 to f2
    ASSERT( !IsNewTbd() );
    csprochar    *readbuf;
    int     faltan;

    if( len <= 0 )
        return;

    readbuf = (csprochar *) calloc( LEN_BLOCK, 1 );
    if( readbuf == NULL )
        issaerror( MessageType::Abort, 1000, _T("READBUF") );

    while( len > 0 ) {
        faltan = ( len > LEN_BLOCK ) ? LEN_BLOCK : (int) len;

        _read( f1, readbuf, faltan );
        write( f2, readbuf, faltan );

        len -= LEN_BLOCK;
    }
    free( readbuf );
}
