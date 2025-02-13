//---------------------------------------------------------------------------
//
// BREAK.cpp      : break manager
//
//---------------------------------------------------------------------------

#include <io.h>

#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "Engine.h"
#include <zToolsO/Tools.h>
#include <zUtilO/SimpleDbMap.h>
#include "Ctab.h"
#include "Batdrv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


bool CTbd::breakinit( const TCHAR* pszTbdName ) {
    if( !m_pBatchDriverBase->GetNumCtabToWrite() )
        return true;

    if( m_pTableIndex != NULL )
        delete m_pTableIndex;
    m_pTableIndex = NULL;

    m_pTableIndex = new SimpleDbMap();

    m_csTbdFileName = pszTbdName;

    tbd_init( pszTbdName );                   // create TbTable and open TBD file

    int     iBreakIdLen = 0;

    for( int i = 0; i < Breaknvars; i++ )
        iBreakIdLen += Breaklvar[i];
    iBreakIdLen += 1; // \0 delimiter  GetTableIdLen();

    m_pBreakidnext = (csprochar*) calloc( 2, iBreakIdLen*sizeof(TCHAR) );
    if( m_pBreakidnext == NULL )
        issaerror( MessageType::Abort, 586 );

    m_pBreakidcurr = m_pBreakidnext + iBreakIdLen;
    *m_pBreakidcurr = *m_pBreakidnext = 0;

    CString csTbiName = PortableFunctions::PathRemoveFileExtension<CString>(pszTbdName) + _T(".") + GetTbiExtension();
    PortableFunctions::FileDelete( csTbiName );

    return m_pTableIndex->Open(CS2WS(csTbiName), { { _T("TBI"), SimpleDbMap::ValueType::Long } });
}

// RHF INIC Oct 22, 2002
void CTbd::FlushAll( const TCHAR* pszBreakId, int iCurrentBreakKeyNum ) {
    ASSERT( ((CBatchDriverBase*)m_pEngineDriver)->GetBatchMode() == CRUNAPL_CSCALC );

    //Save all tables
    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        // RHF INIC Apr 17, 2003
        if( pCtab->GetNumBreaks() != iCurrentBreakKeyNum )
            continue;
        // RHF END Apr 17, 2003

        // With Break
        if( pCtab->m_uEnviron[0] & ct_BREAK ) {
            breaksave( pszBreakId, pCtab );
        }
        // Without Break
        else {
            breaksave( _T(""), pCtab );
        }
    }
}
// RHF END Oct 22, 2002

void CTbd::breakend( void ) {
    ASSERT( Issamod == ModuleType::Batch );
    if( !m_pBatchDriverBase->GetNumCtabToWrite() )
        return;

    if( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC ) {
        FlushAll(   ((CCalcDriver*)m_pBatchDriverBase)->GetCurrentBreakKey(),
                    ((CCalcDriver*)m_pBatchDriverBase)->GetCurrentBreakKeyNum() );

        return;
    }

    //Save tables without break
    *m_pBreakidcurr = 0;

    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        if( !(pCtab->m_uEnviron[0] & ct_BREAK) )
            breaksave( m_pBreakidcurr, pCtab );
    }


    breakclose();
}

void CTbd::breakclose() {
    if( m_pTableIndex != NULL )
        delete m_pTableIndex;
    m_pTableIndex = NULL;

    tbd_MakeFinalTbd();

    if( !m_pBatchDriverBase->GetNumCtabToWrite() )
        tbd_Delete( m_csTbdFileName );

    if( m_pBreakidnext != NULL )
        free( m_pBreakidnext );
    m_pBreakidnext = m_pBreakidcurr = NULL;
}

void CTbd::breakcheckid() {
    int     i;

    ASSERT( Issamod == ModuleType::Batch );
    CBatchDriverBase*   pBatchDriverBase=(CBatchDriverBase*)m_pEngineDriver;

    breakmakeid( m_pBreakidnext );

    if( _tcscmp( m_pBreakidnext, m_pBreakidcurr ) != 0 ) {
        if( pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC ) {
            ;
        }
        else {
            if( *m_pBreakidcurr ) {
                for( CTAB* pCtab : m_engineData->crosstabs )
                {
                    if( pCtab->m_uEnviron[0] & ct_BREAK ) {
                        // RHF INIC Apr 16, 2003
                        if( pCtab->GetNumBreaks() > 0 && pCtab->GetNumBreaks() < Breaknvars ) {
                            if( _tmemcmp( m_pBreakidnext, m_pBreakidcurr, breaklen(pCtab->GetNumBreaks()) ) != 0 ) {
                                breaksave( m_pBreakidcurr, pCtab );
                                breakload( m_pBreakidnext, pCtab );
                            }
                        }
                        // RHF END Apr 16, 2003
                        else {
                            breaksave( m_pBreakidcurr, pCtab );
                            breakload( m_pBreakidnext, pCtab );
                        }
                    }
                }
            }
        }

        _tcscpy( m_pBreakidcurr, m_pBreakidnext );
        for( i = 0; i < Breaknvars; i++ ) {
            int     iSymVar = Breakvars[i];

            VARX* pVarX = VPX(iSymVar);

            // HINT - break' vars are always true Sing-class (non-array), so call to 'svarvalue below is OK
            m_iBreakValue[i] = (long) m_pIntDriver->svarvalue( pVarX );
        }
    }
}


void CTbd::breakmakeid( csprochar* p ) {
    for( int i = 0; i < Breaknvars; i++ ) {
        int     iSymVar = Breakvars[i];
        VART*   pVarT = VPT( iSymVar );

        if( pVarT->IsNumeric() )
            m_pEngineDriver->prepvar( pVarT, NO_VISUAL_VALUE );          // victor May 30, 00
        _tmemset( p, BLANK, Breaklvar[i] );


        p += ( Breaklvar[i] - pVarT->GetLength() );

        // HINT - break' vars are always true Sing-class (non-array)
        csprochar*   pVarAsciiAddr = m_pIntDriver->GetVarAsciiAddr( pVarT ); // victor Jul 10, 00

        _tmemcpy( p, pVarAsciiAddr, pVarT->GetLength() );
        // BUREAU Aug 26, 2005
        if( pVarT->IsNumeric()){
           int iLen=0;
           while(iLen < pVarT->GetLength()){
               if(*p == BLANK){
                   *p = _T('0');
               }
               p++;
               iLen++;
           }
        }
        // BUREAU Aug 26, 2005
        else{
            p += pVarT->GetLength();
        }
    }
    *p = 0;
}


void CTbd::breakload( csprochar* pszBreakId, CTAB* pCtab ) {
    if( pCtab->GetAcumArea() == NULL )
        return;

    csprochar    pszKey[BREAKMAXIDLEN+1];

    //TableIdLen return length in characters. SetTableNum needs the number of bytes it is supposed to write. -Savy 01/31/2012 for unicode changes
    if( IsNewTbd() )
        CTbdFile::SetTableNum( pszKey, pCtab->GetTableNumber() + 1, GetTableIdLen()*sizeof(TCHAR) );
    else
        CTbdFile::SetTableNum( pszKey, pCtab->GetContainerIndex() + 1, GetTableIdLen()*sizeof(TCHAR) );
    _tcscpy( pszKey + GetTableIdLen(), pszBreakId );


    // RHF INIC Apr 16, 2003
    if( pCtab->GetNumBreaks() > 0 && pCtab->GetNumBreaks() < Breaknvars ) {
        int     iCtabBreakLen=breaklen(pCtab->GetNumBreaks());
        int     iTotalBreakLen=breaklen(Breaknvars);

        _tmemset( pszKey + GetTableIdLen() + iCtabBreakLen, _T(' '), iTotalBreakLen-iCtabBreakLen);

        ASSERT( _tcslen(pszBreakId) == iTotalBreakLen );
        ASSERT( *(pszKey + GetTableIdLen() + iTotalBreakLen) == 0 );
        *(pszKey + GetTableIdLen() + iTotalBreakLen) = 0;
    }
    // RHF END Apr 16, 2003


    UINT    uTblSize = pCtab->GetAcumSize();
    long    lFilePos = breakfpos( pszKey );

    if( lFilePos < 0 )
        memset( pCtab->GetAcumArea(), 0, uTblSize );
    else {
        if( IsNewTbd() ) {
            ASSERT(m_pTbdFile != NULL );

            int     iTbdFile=m_pTbdFile->GetFd();

            CTbdSlice* pTbdSlice =  pCtab->GetTbdSlice();
            ASSERT( pTbdSlice != NULL );

            ASSERT(iTbdFile >= 0 );
            if( !pTbdSlice->Load( iTbdFile, lFilePos ) )
                issaerror( MessageType::Warning, MGF::OpenMessage, _T("Can't load the slice") );
        }
        else {
            _lseek( m_iTbdFile, lFilePos + sizeof( BREAK_ID ), 0 );
            _read( m_iTbdFile, pCtab->GetAcumArea(), uTblSize );
        }
    }
}

// RHF INIC Apr 16, 2003
int CTbd::breaklen( int iNumBreakVarsPrefix ) {
    ASSERT( m_pEngineArea != NULL );
    ASSERT( iNumBreakVarsPrefix <= Breaknvars );

    int     iLen=0;
    for( int i=0; i < iNumBreakVarsPrefix; i++ ) {
        iLen += Breaklvar[i];
    }

    return iLen;
}
// RHF END Apr 16, 2003

void CTbd::breaksave( const TCHAR* pszBreakId, CTAB* pCtab ) {
    csprochar    pszKey[BREAKMAXIDLEN+1];
    int     i;
    BREAK_ID BreakId;

    if( pCtab->GetAcumArea() == NULL )
        return;

    if( IsNewTbd() )
        BreakId.ctnum = pCtab->GetTableNumber() + 1;
    else
        BreakId.ctnum = tbd_tabindex( WS2CS(pCtab->GetName()) ) + 1;

    for( i = 0; i < MAXBREAKVARS; i++ )
        BreakId.breakid[i] = -1l;

    // RHF INIC Apr 16, 2003
    int     iNumBreakVars;

    if( pCtab->GetNumBreaks() > 0 )
        iNumBreakVars = pCtab->GetNumBreaks();
    else
        iNumBreakVars = Breaknvars;
    // RHF END Apr 16, 2003

    for( i = 0; i < iNumBreakVars; i++ ) {
        if( *pszBreakId != 0 )
            BreakId.breakid[i] = m_iBreakValue[i];
    }

    //TableIdLen return length in characters. SetTableNum needs the number of bytes it is supposed to write. -Savy 01/31/2012 for unicode changes
    CTbdFile::SetTableNum( pszKey, BreakId.ctnum, GetTableIdLen()*sizeof(TCHAR) );
    _tcscpy( pszKey + GetTableIdLen(), pszBreakId );

    // RHF INIC Apr 16, 2003
    if( pCtab->GetNumBreaks() > 0 && pCtab->GetNumBreaks() < Breaknvars && *pszBreakId != 0 ) {
        int     iCtabBreakLen=breaklen(pCtab->GetNumBreaks());
        int     iTotalBreakLen=breaklen(Breaknvars);

        _tmemset( pszKey + GetTableIdLen() + iCtabBreakLen, _T(' '), iTotalBreakLen-iCtabBreakLen);
        /* RHF CHECK why extra junk characters are  coming in .uncomment this code once the fix is in
        ASSERT( strlen(pszBreakId) == iTotalBreakLen );
        ASSERT( *(pszKey + GetTableIdLen() + iTotalBreakLen) == 0 );*/
        *(pszKey + GetTableIdLen() + iTotalBreakLen) = 0;
    }
    // RHF END Apr 16, 2003

    long    lFilePos = breakfpos( pszKey );
    bool    bNewBreak = ( lFilePos < 0 );

    if( IsNewTbd() ) {
        ASSERT(m_pTbdFile != NULL );

        int     iTbdFile=m_pTbdFile->GetFd();

        if( bNewBreak )
            lFilePos = _lseek( iTbdFile, 0l, 2 );
        else
            _lseek( iTbdFile, lFilePos, 0 );

        if( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC ) // RHF Oct 25, 2002.
            pCtab->DoStatistics(); // RHF Aug 14, 2002

        CTbdSlice* pTbdSlice =  pCtab->GetTbdSlice();
        ASSERT( pTbdSlice != NULL );

        /*
        TBD_SLICE_HDR           tSHdr;

        tSHdr.iTotalLenght = sizeof(TBD_SLICE_HDR) + pCtab->GetNumCells() * pCtab->GetCellSize() + pTbdSlice->GetBreakKeyLen();
        tSHdr.iTableNumber = pCtab->GetTableNumber();
        tSHdr.cSliceStatus = 0;
        tSHdr.cOtherInfo = 0;

        pTbdSlice->SetSliceHdr(&tSHdr);
        */

        pTbdSlice->SetBreakKey( (LPTSTR)&pszKey[0], pTbdSlice->GetBreakKeyLen() );

        ASSERT(iTbdFile >= 0 );

        if( !pTbdSlice->Save( iTbdFile, lFilePos ) )
            issaerror( MessageType::Warning, MGF::OpenMessage, _T("Can't save the slice") );

    }
    else {
        if( bNewBreak )
            lFilePos = _lseek( m_iTbdFile, 0l, 2 );
        else
            _lseek( m_iTbdFile, lFilePos, 0 );

        UINT    uTblSize = pCtab->GetAcumSize();

        if( _write( m_iTbdFile, (csprochar *) &BreakId, sizeof(BREAK_ID) ) != sizeof(BREAK_ID) )
            issaerror( MessageType::Abort, 14010 );

        if( _write( m_iTbdFile, pCtab->GetAcumArea(), uTblSize ) != uTblSize )
            issaerror( MessageType::Abort, 14010 );
    }

    if( bNewBreak ) {
        if( m_pTableIndex->Exists(pszKey) || !m_pTableIndex->PutLong(pszKey,lFilePos + 1) )
            issaerror( MessageType::Abort, 4003, m_pTableIndex->GetDbFilename().c_str(), pszKey );
    }

}


long CTbd::breakfpos( csprochar* pszKey ) {
    long    lFilePos = m_pTableIndex->GetLong(pszKey).value_or(0);
    return( lFilePos - 1 );
}
