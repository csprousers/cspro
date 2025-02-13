//---------------------------------------------------------------------------
//
//  CalcIFaz: Extensions for CsCalc module
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <engine/calcifaz.h>
#include <engine/Ctab.h>
#include <engine/IntDrive.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCalcIFaz::CCalcIFaz() {
    //C_SetBatchMode( CRUNAPL_CSCALC );
}

// For the application
int CCalcIFaz::C_GetAppCtabs( CArray <CTAB*, CTAB*>& aCtabs ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );

    for( CTAB* pCtab : m_engineData->crosstabs )
        aCtabs.Add( pCtab );

    return aCtabs.GetSize();
}

// For Each TBD
bool CCalcIFaz::C_OpenInputTbd( CString csInputTbdName ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );
    bool bRet;

    bRet = ((CCalcDriver*)m_pBatchDriverBase)->OpenInputTbd( csInputTbdName );

    if( !bRet ) {
        issaerror( MessageType::Error, 589, (LPCTSTR)csInputTbdName ); // Cannot open TBD file %s
    }

    return bRet;
}

void CCalcIFaz::C_CloseInputTbd() {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );

    ((CCalcDriver*)m_pBatchDriverBase)->CloseInputTbd();
}


// aUsedCtabs contains table that are in TBD & APP.
// aUnUsedCtabs contains table that are only in APP (not in TBD)
// aTables contains tables that are only in TBD (not in APP)
int CCalcIFaz::C_GetTbdCtabs( CArray<CTAB*,CTAB*>& aUsedCtabs,
                               CArray<CTAB*,CTAB*>& aUnUsedCtabs,
                               CArray<CTbdTable*,CTbdTable*>& aTables  ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );

    CMap<CTAB*,CTAB*,int,int&> m_mapUsedCtabs;

    CTbdFile*   pTbdFile=((CCalcDriver*)m_pBatchDriverBase)->GetInputTbd();

    for( int iTable=0; iTable < pTbdFile->GetNumTables(); iTable++ ) {
        CTbdTable*  pTable=pTbdFile->GetTable(iTable);

        CString csTableName=pTable->GetTableName();

        int iCtab = m_pEngineArea->SymbolTableSearch(csTableName, { SymbolType::Crosstab });

        if( iCtab > 0 ) {
            CTAB* pCtab = XPT(iCtab);
            aUsedCtabs.Add( pCtab );
            m_mapUsedCtabs.SetAt( pCtab, iCtab );
        }
        else {
            aTables.Add( pTable );
        }
    }

    for( CTAB* pCtab : m_engineData->crosstabs )
    {
        int iDummy;

        if( m_mapUsedCtabs.Lookup( pCtab, iDummy ) != TRUE )
            aUnUsedCtabs.Add( pCtab );
    }

    return aUsedCtabs.GetSize() + aUnUsedCtabs.GetSize() + aTables.GetSize();
}

int CCalcIFaz::C_GetTbdBreakKeys( CStringArray& aBreakKey, CUIntArray& aBreakNumKeys ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );

    CTbdFile*   pTbdFile=((CCalcDriver*)m_pBatchDriverBase)->GetInputTbd();
    CTbiFile*   pTbiFile=((CCalcDriver*)m_pBatchDriverBase)->GetInputTbi();

    if( pTbdFile->GetBreakKeyLen() <= 0 )
        return 0;

    pTbiFile->Locate(CTbiFile::First);

    const TCHAR* pKey;
    const TCHAR* pszKey;
    long    lFilePos;
    int     iTableNum;
    int     iNumBreak=0;
    CMap< CString, LPCTSTR, int, int& > mapKeys; // RHF Apr 17, 2003

    while( pTbiFile->Locate(CTbiFile::Next) )  {
        pKey = pTbiFile->GetCurrentReg();
        lFilePos = pTbiFile->GetCurrentOffset()-1;

        iTableNum =  CTbdFile::GetTableNum(pKey, sizeof(short));
        pszKey = pKey + sizeof(short)/sizeof(TCHAR); //Savy for unicode 02/02/2012

        // RHF INIT Aug 30, 2005
        ASSERT( pTbiFile->GetRegLen() >= sizeof(short) );
                 //Savy Jan 18 2008 commented this code truncates the break key incorrectly and
                 //does not add all the keys it in case of area breaks
       // pszKey[ pTbiFile->GetRegLen()-sizeof(short) ] = 0;
        // RHF INIT Aug 30, 2005

        // RHF INIC Apr 17, 2003
        if( iTableNum <= 0 || iTableNum > pTbdFile->GetNumTables() ) {
            issaerror( MessageType::Error, 692, (LPCTSTR)pTbiFile->GetFileName(), iTableNum ); // Cannot open TBD file %s
            continue;
        }
        // RHF END Apr 17, 2003

        /* RHF COM INIC Apr 17, 2003
        if( iTableNum == 1 ) {
            CString csKey(pszKey);
            aBreakKey.Add( csKey );
        }
        RHF COM END Apr 17, 2003 */

        // RHF INIC Apr 17, 2003
        //CString csKey(pszKey);
        //add full key - GHM's idx mods have changed the index creation.
        CString csKey(pKey); //write key with the table num included

        // Add if not in the list
        if( mapKeys.Lookup( csKey, iNumBreak ) != TRUE ) {
            CTbdTable*  pTable=pTbdFile->GetTable(iTableNum-1);

            iNumBreak = pTable->GetNumBreak();
            mapKeys.SetAt( csKey, iNumBreak );
            aBreakKey.Add( csKey );
            aBreakNumKeys.Add( iNumBreak );
        }
        // RHF END Apr 17, 2003
    }

    return aBreakKey.GetSize();
}

// Between TBD, XTS & APP. IF Tbd has less tables than the APP tables
// show a warning (show error if it not a subset).
bool CCalcIFaz::C_CheckIntegrity( CStringArray& csErrorMsg ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );

    //CTbdFile*   pTbdFile=((CCalcDriver*)m_pBatchDriverBase)->GetInputTbd();
    ASSERT(0);

    return false;
}

//Load the slice 'csCurrentBreakKey' for the table specified in aUsedCtabs
// For NoBreak tables, the unique slice is loaded (no message issued)
bool CCalcIFaz::C_LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );
    bool    bRet = ((CCalcDriver*)m_pBatchDriverBase)->LoadBreak( csCurrentBreakKey, iBreakKeyNum, aUsedCtabs, iCurrentBreak, iNumBreaks );

    return bRet;
}


void CCalcIFaz::C_SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray* aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs ) {
    ASSERT( m_pBatchDriverBase->GetBatchMode() == CRUNAPL_CSCALC );
    ((CCalcDriver*)m_pBatchDriverBase)->SetRunTimeBreakKeys( aBreakKeys, aBreakNumKeys, aUsedCtabs );
}

